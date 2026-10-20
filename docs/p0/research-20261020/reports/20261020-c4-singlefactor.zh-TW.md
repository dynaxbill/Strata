> Historical sanitized export. Source SHA256: `f077c611c45a686f011699c27a577e7ed3bf97c249674eecbd1c52a614557296`. Original failures and measurement scope are retained. Receipt links refer to private artifacts, not publicly downloadable raw data.

# C4 單因子驗證報告

2026-10-20，epoch `20261020T034300Z-b97a3bd4`。九個預先凍結案例已一次完成，未重跑 GPU。**本輪 C4 p95 退化可由 slot verifier 的額外工作重現；關閉 VERIFY 後改善約 9.3%，兩次都通過原 3% 效能門檻。但 C2 對原包絡仍各有 8/144 超界，整體維持不放行。**

現行 v0.20 production/API 已還原並通過獨立 authenticated nonce、runtime/native hashes、GPU/lease、dashboard 與 boot 核驗。沒有 production promotion。

## 單一因子與根因證據

上一輪封存 ZIP `12fb304ea78e8ad98933831b72dd8ec01aee9122d8b3ce85ac2306e55cf7ac46` 的有效 env 證實：P0 C4 啟用 `STRATA_SLOT_COPY_VERIFY=1`，B0 未啟用。先前「capture OFF」只涵蓋 tensor/slot trace，未涵蓋 checkpoint/KV verifier。原 27.969 ms/token、相對 B0 +4.12%/+3.76% 及原 FAIL 均保留；原比較混入診斷成本，無法單獨量化純 ROM 成本。

本輪四個 P0 cold start 只切換 `STRATA_SLOT_COPY_VERIFY=1/0`；`COPY_REUSE=1`、`STRATA_IQ_MT_MIN=1`、GPU1 policy（`--pcie-frac 1`）、ROM 雙副本與 NUMA roles、cache、Graph、prefill、prefix、PLE、routing、ordered combine 全部固定。實際 config/env/image/cache/harness invariants 通過；沒有編譯或改 kernel。

實際 immutable P0 image `95601c2…` 內的 `generate.cpp` SHA `0a6238aa7287058f672b6112a9044de167f283f741f1a0c0f762360f5635beb4`：9945–9953 行額外保存並比對 checkpoint；9985–9988、10047–10050 行呼叫 KV verify。實際 `conversation_snapshot.cpp` SHA `60dbc4b18d60bf87c8526173f528d17b79c9197f7cc8816be92807900ec2f719`，257–298 行逐 64 KiB 回讀與 memcmp，並對 authoritative payload 逐 byte 計算 FNV hash，也核對 resident K/V。這些工作位於 serial admission/copy 路徑；正常 save/restore 與既有同步呼叫未刪除。

因此有來源碼與 ON/OFF/ON/OFF 重複實驗共同支持的根因是 **診斷 verifier 混入效能路徑**。本輪沒有再把 readback、hash 與 checkpoint 的耗時各自拆分，不能宣稱其中一個子步驟獨占全部成本，也不推論硬體／driver 故障。

## C4：通過原效能門檻

同 4K prompt／256 output、四併發；每次 18 輪，依原規則排除前兩輪，每次 N=64。decode 指標是「每請求平均 decode ms/token」的 nearest-rank p95，**不是逐 token streaming ITL p95**。

| 順序／配置 | decode p95 ms/token | TTFT p95 秒 | E2E p95 秒 |
| --- | ---: | ---: | ---: |
| B0 前測 | 26.860 | 1.0162 | 7.3123 |
| P0 VERIFY ON1 | 28.576 | 1.4000 | 7.8537 |
| P0 VERIFY OFF1 | 25.926 | 0.7748 | 6.9537 |
| P0 VERIFY ON2 | 28.609 | 1.3781 | 7.8726 |
| P0 VERIFY OFF2 | 25.949 | 0.7618 | 6.9772 |
| B0 後測 | 26.836 | 0.9873 | 7.2930 |

OFF1 相對 B0 前／後為 **−3.477%／−3.391%**；OFF2 為 **−3.392%／−3.305%**，兩次皆同時通過原 +3% 上限。B0 前後漂移約 −0.089%。四個 ON/OFF 交叉比較改善 **9.193%–9.378%**，兩個 OFF 都優於兩個 ON。

P0 四次的 input、answer SHA 分布一致；全部 cache_n=4089、output=256，實際 peak_running=peak_decoding_slots=4、各次 native PID 穩定。分 cold start／round 的 hierarchical bootstrap（5,000 次、seed=1337）診斷區間為 **−9.452% 至 −9.015%**；每組只有兩個 cold start，區間不取代原 gate，也不代表所有負載的保證。

原始資料與重算：C4-singlefactor-bench.json (private receipt: `C4-singlefactor-bench.json`)。

## C2：VERIFY 沒有新增差異，但原包絡仍 FAIL

原比較器與既有三個 B0 starts／每 prefix 九向量完全不變；48 prefix × 3 repeats × 248320 logits，零追加容差、零新增包絡向量。這是 C4 capacity 下的序列重放，沒有宣稱完整四併發 head 等價。

| 比較 | 結果 |
| --- | --- |
| ON 對原固定 B0 包絡 | **FAIL：8/144 超界，0 新 Top-1；136 個匹配真實 B0 向量** |
| OFF 對原固定 B0 包絡 | **FAIL：8/144 超界，0 新 Top-1；136 個匹配真實 B0 向量** |
| ON/OFF 同輸入、同 repeat 直接配對 | **144/144 bitwise；max abs=0、RMS=0、0 Top-1 改變** |

八個超界都在 repeat=0 的 `chinese:0,1,2,4,8,16,31,57`。最大包絡超出量 **0.4286017417907715**；對第一個 B0 最大差 **0.46791887283325195**。Top-1 未變不能抵銷數值 FAIL。

與先前已通過那一批 P0 ON 比較，本輪有效 env 逐 byte 相同、runtime config 相同，144 個 input hashes 與順序相同，仍有這八個 head 改變。最早可觀測分歧在 **第 41 個請求（zero-based 40），repeat0 `P0-C2:chinese:0`**：input SHA `ba4da394d43bd05e57619a842d4c5d1169164b88161eb7a0b9a9b00745597271`，舊 head `b01977f8…`、本輪 `10db6537…`。

這證明本輪關閉 VERIFY 沒有新增已觀測的 C2 差異；**尚不能判定八個共同超界的內部根因，亦未證明這批新向量在 B0 重現**。本輪效能 study 沒有捕獲逐層 activation／實際 pending/residency／GDN/KV／backend expert plan，不能把最早 head 差異冒稱第一個內部 tensor 差異。下一個定位目標應保留前 40 請求，追蹤第 41 請求的有效狀態；本輪沒有擴充 GPU 案例或重跑到 PASS。

原 default P0 的 **125/144＋1 新 Top-1 FAIL** 與先前 GPU1 批次的歷史結果都保留。資料：C2-singlefactor.json (private receipt: `C2-singlefactor.json`)、C2-prior-P0-firsthead11.json (private receipt: `C2-prior-P0-firsthead11.json`)。

## C3：功能通過，原拒答的同 protocol 重放未成立

OFF 的 C4 precondition、history20、API9、生成 gcd 的八個執行案例及 canonical native input audit 全部 rc=0，suite PASS。本輪未觀察到這組功能題目的拒答。

目標 SLOT-0 仍是 4129 個相同 token IDs，input SHA `7998199a…`；full-head 為 `a89b710d…`，與先前 B0 逐位相同，max abs/RMS=0，Top-1=50、margin≈0.4456234。但本輪實際 header 是 **`GEN 128 …`**，歷史 B0 是 **`BGEN 3 128 …`**。因此 full-head 相同只作診斷；**「相同 protocol／完整前序狀態的拒答重放」為 NOT_SATISFIED**，原 bad head `6a0c8f…` 與拒答問題未被本輪精確重現或結案。

第一版事後分析器錯誤預設舊 BGEN slot3 而觸發 assertion；原腳本、失敗 log 保留。新分析器只記錄實際 headers，明列 protocol 不同與 NOT_SATISFIED，不將修正當成 gate 放寬。另保留 UTF-8 讀檔誤用 cp950，以及 CPU 輔助查詢的一次括號語法錯誤；均未改 raw 或重跑 GPU。資料：C3-target12.json (private receipt: `C3-target12.json`)、analyzer-correction12.json (private receipt: `analyzer-correction12.json`)。

## 資源、還原與證據

九案例全部完成，1,135 次資源採樣無 monitor error；model cgroup swap 最大 0 bytes，OOM／OOM-kill counters 為 0。七次 P0 載入皆驗證 24576 expert blobs bitwise identical、24577 spans 與唯讀 images；沒有 ROM fallback。這些數據不等同整台 VM 零 swap。

2026-10-20 **12:25:53 台灣時間**，獨立還原核驗通過：現行 v0.20 image `<PRIVATE_RESTORATION_IMAGE>`、原 runtime/API hashes、live native/server 對 immutable image、兩 GPU 原服務 ownership／lease、authenticated nonce `[redacted-authenticated-nonce]`。controller/ExecStopPost 均為 0、inactive/success；dashboard active、原 unit SHA `c57b742e…` 與 resource limits 不變，PVE/VM boot 未變。最後的唯讀 continuity 與 dashboard 收據另存本機 analysis。

原始 ZIP：`p0-c4-singlefactor-evidence-20261020T034300Z-b97a3bd4.zip`，**313,488,333 bytes／2,101 檔**，SHA256 **`fe4009ea788265abeb3523f064a0571a22269584bd17bfda1454058b87ab7495`**。下載後整 ZIP SHA、每個 member 的 CRC、大小與 SHA 全部通過；manifest 無重複或越界路徑。原始與失敗證據沒有刪除或改寫。見 LOCAL-VERIFICATION.json (private receipt: `LOCAL-VERIFICATION.json`)。

封存後本機報告、比較結果、原失敗分析器與修正收據另封存為 `strata-p0-c4-singlefactor-20261020-analysis-sidecar11.zip`，配有逐檔 SHA manifest 與獨立 CRC/SHA 核驗收據；這些後續分析不冒稱已包含在原始 GPU ZIP。12:31:33／12:31:35 最後 continuity/dashboard 核驗均 PASS，收據見 post-analysis-status11.json (private receipt: `post-analysis-status11.json`)、post-analysis-pve11.json (private receipt: `post-analysis-pve11.json`)。

凍結 package SHA `ea9efb881db65a8c0f9c6eeb57e0ecb6e03048b1620dfb73b33aaedb09cf30cb`（52 檔）；原 P0 image `95601c2…`／native `101c20db…`、原 B0 image `d99ba0e8…`／native `4ea6a2fa…`。完整來源、設定、方法與唯一案例順序見 FROZEN-PLAN.md (private receipt: `FROZEN-PLAN.md`)。

## 對策與驗證界線

低延遲對策是在效能測量路徑使用 **`STRATA_SLOT_COPY_VERIFY=0`**，保留 `COPY_REUSE=1` 與原正常複製流程；需要逐 byte 狀態驗證時，在獨立正確性案例啟用 VERIFY。這個對策已消除本輪 C4 的 +3% 退化，沒有透過修改浮點容差、ROM Gate 或新增 B0 向量達成。

**整體仍 HOLD／NOT_PROMOTED：C2 8/144 FAIL，C3 同 protocol 拒答重放尚未成立。** 本輪沒有驗證 OFF 的單併發 C1、512K 或 HIP/SYCL，不繼承先前其他配置的 PASS。不修改 production profile、硬體、driver 或全域安全設定；保留 PVE 故障紀錄，沒有新增 subagent／peer 訊息，heartbeat 維持 PAUSED。
