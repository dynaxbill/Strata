> Historical sanitized export. Source SHA256: `f052fd18d9046529170dbd144a71a7839fd63f2280fd226891cff70afb7b3053`. Original failures and measurement scope are retained. Receipt links refer to private artifacts, not publicly downloadable raw data.

# P0 C2 首差根因報告

2026-10-19，台灣時間 15:43。**首次新增 C2 分歧已用實際 activation／expert witness 複現並定位；原 C2 仍 FAIL，P0 未放行。**

根因是原有 CPU expert 對批次大小的算術路徑依賴：draft 寬度 T3/T4 改變同一 expert 的 CPU 批次 NT，預設門檻 `STRATA_IQ_MT_MIN=2` 因而切換單列 ggml 與多列 AVX2 路徑。此次最早會傳播的 expert 輸出差異發生在 **0-based layer 15、expert 255、committed row 0、rank 8**，其實際格式是 **IQ2_S gate/up、IQ4_NL down**。模型／檔案稱為 IQ3_S，並不表示每層每個 projection 都是 IQ3_S。

本輪證明了首次分歧的計算鏈。原始執行未記錄 EMA cost 向量，因此「當次 ROM 耗時變動如何精確越過 DraftPolicy 選擇門檻」不能事後完整重建；但固定 T 的雙向干預已精確重現原始兩側完整 head，排除了僅憑時間相關性歸因。

## 因果證據

| 位置／對照 | 結果 |
|---|---|
| 原始第 12 請求 `math:4` | first head／回覆相同；B0 用 T4、P0 用 T3 |
| 同 T4：B0 對 P0 | 首 13 個完整 248320-F32 heads 全部逐位相同 |
| 同 T3：B0 對 P0 | 首 13 個完整 heads 全部逐位相同 |
| B0 與 P0 各自只切換 T4/T3 | 首差都在下一請求 `math:8`，精確重播封存兩側 SHA；maxabs `0.13855111598968506`、RMS `0.027005825103313848` |
| 四組 trace ON/OFF | 各 13 份完整 head、proposal/context 逐位／精確一致；兩 stage window 與六份 post snapshots 完整 |
| compiled P0_OFF T4 | 與 B0 T4 全部 13 份 head 逐位相同（首輪收集） |

T4 的 `math:8` SHA 為 `dc0144e9f9d42cf0e2928b2902c8e5e816bdf8ccbf5f4fdd18f2c134c2eb99c8`；T3 為 `f39fa84a2467378832d2588113dea6dd8313c5b70ff9ccb0752b8a99a9046ff2`，正是原始 B0/P0 首差。原 P0 的本輪自然重播有一次走到 T4 結果，該新資料沒有加入原 B0 包絡，也沒有覆寫原 FAIL。

## 第一個有效內部分歧

`math:2` 後與 `math:4` verify 前，兩側 owned GDN、PLE、有效 KV、residency 均相同。窗口中 layer 0–15 的共同 committed rows，其專用 GPU activation／router weights 相同；layer 15 的 CPU HOST activation 也與專用 GPU copy 全部逐位相同，因此不是未發布或 stale HOST 資料。

在 layer 15，T3 只有 row 0/rank 8 路由到 CPU expert 255；T4 額外的 rejected row 3/rank 4 也路由到它。於是同一 expert 的 CPU 批次由 **NT1 變 NT2**，共同 row 0 的權重、activation、router weight、expert ID 與 CPU backend 都未改變。

| 可觀測量 | 最早差異 |
|---|---|
| CPU expert 255 輸出 | layer 15，maxabs `2.60770320892334e-08`，RMS `4.115364374331116e-09` |
| 下一層 GPU activation | layer 16，maxabs `2.980232238769531e-07` |
| GPU router weights | layer 16，maxabs `5.960464477539063e-08` |
| 共同 row 的 expert 身份／rank | layer 27 才首次變化 |
| backend kind | layer 37 才首次變化，且當時 expert 身份已不同 |

Post `math:4` 已有有效 GDN 差異；global rounds 都是 24，兩側 pending 都有 96 對，但內容、usage 與部分 residency 不同。這些 adaptive cache 差異確實存在，**但不能解釋比它更早的 layer 15 同 CPU expert 算術首差**。本例也不能直接套用先前 C3 的 CPU/GPU 量化分流根因。

KV 比對只使用有效 live-token 座標；排除了頁面尾列、未使用 indexer tail、block_pos scratch、GPU-owned HOST expert parts。完整機讀資料：internal-comparisons (private receipt: `internal-comparisons.json`)、firstdiff inspection (private receipt: `firstdiff-inspection.json`)、CPU input validity (private receipt: `CPU-input-validity.json`)。

## 實際 CPU witness：確認會傳播的差異來自 down 分支

使用實際 layer 15/expert 255 權重、row 0 activation 與 T4 額外 row 3 activation，呼叫原有 compiled CPU functions，沒有修改 math archives，沒有 GPU 裝置或 CUDA 呼叫。預設 NT1、NT2、反向重複與替換額外列值均有收據。

- NT1 輸出 **逐位等於實際 T3 row 0**；NT2 輸出 **逐位等於實際 T4 row 0**，額外 row 3 的輸出也逐位相等。
- 同一 NT 自身重複逐位一致；將額外列改成 row 0 副本，不改變共同 row 0，排除跨列數值污染。
- input quant bytes 相同。Gate/up hidden FP32 曾相差 `5.960464477539063e-08`，但 **hidden quant bytes 仍完全相同**。
- 在相同 hidden quant bytes 與權重下，IQ4_NL down 的 NT1 ggml／NT2 AVX2 分支輸出仍不同。因此本次會傳播的 expert 輸出差異已定位到 down 路徑切換。

| 私有 CPU witness 模式 | NT1 對 NT2 | 等同哪側原輸出 |
|---|---|---|
| 原預設 `MT_MIN=2` | 不相同，精確重現實際 trace | NT1=T3、NT2=T4 |
| `MT_MIN=1` | 所有擷取階段逐位相同 | T4／原多列路徑 |
| `MT_MIN=99`，本例固定 ggml | 所有擷取階段逐位相同 | T3／原單列路徑 |

證據：witness verification (private receipt: `witness-verification.json`)、build identity (private receipt: `build-identity.json`)。這是本次 C2 的 expert 255，沒有拿舊 C3 的 expert 193 代替。

## 高效、低延遲對策與界限

優先方向是讓 CPU expert 的算術路徑與 NT 無關，保留多列解碼／權重重用、Graph、prefill、prefix、PLE 與 ordered combine。現成 `STRATA_IQ_MT_MIN=1` 在此次實際 witness 已達 NT1/NT2 **零誤差**，可作為後續完整模型驗證的候選；亦應覆蓋其他混合量化格式的 gate/up 和 down，避免只修掉第一個 expert。

`MT_MIN=99` 可作固定舊 ggml 算術的診斷參考。固定 draft 寬度可以減少耗時對選擇的影響，但本身沒有消除 NT-dependent kernel 問題。

**沒有把任何候選套用到 production。** 兩個固定路徑會選擇不同的既有單列／多列輸出；它們通過的是此 CPU witness，尚未通過完整 144-prefix C2、C3、512K、並發或 p95 重驗。本輪未測量候選的完整模型延遲，因此不宣稱加速比例；HIP/SYCL 亦未執行。

## 驗收、還原與封存

原比較器／封存資料不變：**125/144 超出事先固定的 9 向量 B0 包絡、1 個新 Top-1，原 C2 FAIL／NOT_PROMOTED。** 首 13-prefix 因果結果不能取代完整驗收，也未逐項證明其餘差異與後續新 Top-1 的完整因果鏈。

15:40 最終 fresh readback：原 immutable f20 production 與 API active、兩 GPU 程序與 production container 身份正確、GPU lease 由原服務 supervisor 持有、私有控制鎖已釋放、九個 runtime 檔案 SHA 不變；執行中的 native／server／replicas hashes 與 f20 image 逐位一致。Authenticated nonce `[redacted-authenticated-nonce]` 通過。CPU witness 前後 production container、GPU PIDs、runtime hashes 與 boot ID 全相同。見最終還原驗證 (private receipt: `final-production-readback.json`)。

| 證據包 | 檔案數 | SHA256 |
|---|---:|---|
| 首輪與 trace 失敗 | 284 | `dec215217719c1c246481f7bbe7d3661e589fb965f8017ada76d5d2500c81fbe` |
| R2 完整 GPU traces，1,971,616,645 bytes | 3875 | `e5e3c707da4519041dad20ede7e98640ed9fdcb38cd4d0c69d17895dee9d8506` |
| CPU witness／最終 readback v2 | 129 | `37577532ba46559270d9db275ed71c1a7bbbff4d8d9fa1ee1a9862be10ba3dbd` |

三包皆已下載，ZIP SHA、CRC、manifest 逐檔 SHA 全數驗證。原始 C2 與 PVE 故障包仍保留，未清除事件、未重啟主機、未改硬體／driver／全域安全設定。PVE 原故障原因仍未知。

首輪 trace 的觀測複製契約錯誤、收集腳本錯誤、CPU 證據包 manifest 命名衝突及一次本機 verifier 參數錯誤都保留於失敗清冊 (private receipt: `failure-ledger.json`)。修正各有新收據；沒有為取得模型 PASS 而重跑原測項。原始有重名 manifest 的 CPU ZIP 亦保留，正確 v2 只修封裝名稱，未重跑 witness。

結案判定：causal-verdict.json (private receipt: `causal-verdict.json`)。本任務完成的是「首次差異與根因定位」，沒有變更 production 或 P0 放行標準。
