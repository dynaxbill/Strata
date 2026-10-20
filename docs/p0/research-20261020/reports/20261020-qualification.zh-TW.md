> Historical sanitized export. Source SHA256: `20250208c6b7d64635eced6a6625052788fd85582f823c1979d756f79f2e3472`. Original failures and measurement scope are retained. Receipt links refer to private artifacts, not publicly downloadable raw data.

# P0＋MT1／GPU1 本輪驗證報告

**後續CPU查核註記（2026-10-20 C4單因子新任務）：** 原封存effective-env確認P0 C4實際SLOT_COPY_VERIFY=1，B0未啟用；本報告「capture OFF」只指tensor trace，未涵蓋checkpoint/KV verifier。下列27.969ms/token及原FAIL保留；尚不能把全部退化歸於ROM本體。人類已授權在全新strata-p0-c4-singlefactor-20261020固定COPY_REUSE=1、只切VERIFY作因果驗證，該任務結果另報，不改寫這份原始Gate。

更新時間：2026-10-20T10:03:53+08:00（台灣時間）。E131 exact 與18個邏輯階段已執行並完成證據核驗；**P0仍不放行／不promotion，因C4 p95超過原3%門檻。** 原default C2 125/144超包絡＋1新Top1，以及舊C3／PVE故障記錄不改判。MT1指CPU dispatch的STRATA_IQ_MT_MIN=1，與正式服務MTP分開。 實驗仍使用原d99 B0／956 P0（Strata v0.1.44.1）；v0.20只作正式服務還原目標。本輪為既定CUDA驗證，HIP／SYCL不在這18階段內，維持NOT_RUN。

## 本輪結論

| 項目 | 結果 |
|---|---|
| E131實際首差重放 | CPU/GPU各自trace及self-repeat誤差0，GPU權重bytes exact，既有數學archives逐位未變；兩路彼此maxabs .0033283382654190063 |
| 新GPU1政策C2 | 0/144超預先固定9向量包絡，0新Top1；零追加容差。原default FAIL保留 |
| C1單請求p95 | P0 11.982 ms/token，B0/GPU1前12.848、後12.845；相對-6.74%／-6.72%，原Gate PASS |
| C4四請求p95 | P0 27.969 ms/token，B0/GPU1前26.861、後26.955；相對+4.12%／+3.76%，原Gate FAIL |
| 完整C3 | B0/GPU1 PASS；P0/GPU1 PASS |
| 512K B0/GPU1 | PASS_512K_CLASS_EXACT_RECALL；3個已記錄深度 |
| 512K P0/GPU1 | PASS_512K_CLASS_EXACT_RECALL；3個已記錄深度 |

先前C2首差與本輪C3須分開：舊C2由draft T3/T4使同CPU expert批次NT1/NT2，預設MT_MIN=2切換ggml／AVX2；首個會傳播的差異是L15/E255 IQ4_NL down，maxabs 2.6077e-8，後續放大。MT_MIN=1固定既有dispatch後新對照通過；原FAIL不改。詳先前C2因果報告 (private receipt: `REPORT.md`)。

## C3根因與對策證據

本輪首個有效tensor分歧為pos4122、0-based L0/E131/rank3/row0。同activation、router IDs/weights與正確GGUF bytes下，CPU使用q8_K activation／q8_0 hidden、GPU使用q8_1；GU權重iq3_xxs、down iq4_nl。2560/2560 expert元素不同，maxabs .0033283382654190063、RMS .0003100228662314996。獨立CPU/GPU micro各自逐位重現原trace，支持固定backend算術差異，再由adaptive residency改換路徑、後續層與GDN/KV狀態放大的機制。此重放未分別消融量化與算術順序，不能把根因縮成單一FMA或單一量化步驟。

原受控.55 repeat0→2有相同有效prompt prestate、352個membership變動，E131由CPU移到VRAM而產生上述分歧；GPU1也有352個變動，但所比較tensor/state/head相同。新正常SFX完整C3兩者均完成history20、API9、一份生成gcd程式的8個執行測例，以及canonical輸入稽核。

原拒答位置的實際4129 IDs SHA均為7998199aa7affc766a57d64c48d507c38540252455d86b6886be1efb9a5483b1；兩者header均為BGEN slot3／128／top_p=.9／top_k30／seed1337，完整248320-FP32 heads均a89b710d19f6875881f038dda2750cecf6427d268f6b11d5501ffba39e2332f1，**maxabs=RMS=0、Top1=50、margin=.44562339782714844**，匹配先前GPU1診斷。這是本輪固定題組與已量測狀態的結果，不保證未測輸入；原badhead6a...的原始完整前態未捕捉，未宣稱逐位重建該次失敗。

## 延遲與可用範圍

測試保留Graph、prefill、prefix、PLE、routing與ordered combine。正常SFX、capture OFF；4K prompt、256 output、cache_n=4089。每次18輪排除前2輪：C1 N16、C4 N64。p95是每請求平均decode ms/token的nearest-rank，並非串流ITL；3%放行門檻不變，bootstrap僅供診斷。

| 場景 | B0/GPU1前 | P0/GPU1 | B0/GPU1後 |
|---|---:|---:|---:|
| C1 decode p95 ms/token | 12.848 | 11.982 | 12.845 |
| C1 TTFT p95 ms | 150.880 | 114.844 | 149.634 |
| C1 E2E p95 s | 3.426 | 3.168 | 3.423 |
| C4 decode p95 ms/token | 26.861 | 27.969 | 26.955 |
| C4 TTFT p95 ms | 980.101 | 1279.389 | 987.505 |
| C4 E2E p95 s | 7.278 | 7.649 | 7.291 |

GPU1對策在本輪C1有延遲改善，C4仍退化。C4回答hash與cache reuse分布相同，模型native／容器cgroup採樣swap為0；量測期GPU時脈／溫度另有原始記錄。這些只能縮小診斷範圍，**尚未取得本輪C4退化的單因子因果證明**。後續可聚焦C4 admission、host staging與copy-reuse路徑；本輪未追加GPU調參或重跑C4直到PASS。

## 512K實際檢索

| 場景 | needle深度 | 實際usage prompt_tokens | 結果 |
|---|---:|---:|---|
| B0/GPU1 | 10.00% | 524088 | PASS |
| B0/GPU1 | 49.99% | 524088 | PASS |
| B0/GPU1 | 89.99% | 524088 | PASS |
| P0/GPU1 | 10.00% | 524088 | PASS |
| P0/GPU1 | 49.99% | 524088 | PASS |
| P0/GPU1 | 89.99% | 524088 | PASS |

三份payload與原manifest的原生模板IDs均已在CPU逐一核對524088與原指紋，GPU另檢查實際usage及完整預期識別字。512K為獨立capacity profile（max_context524288）；包含原prefix reuse，不宣稱全任務512K品質或C4長decoder容量，也未將正式服務改為此profile。

## 失敗保全與修正界線

原18窗口的C1 shape assertion錯將solo path當作batch slot。原server明載solo在outside_slots；55筆實際running/generating/outside_slots1樣本與18輪單請求非重疊證明後，才以新有限窗口要求正向solo證據。原失敗保留，數值與p95門檻未改。

我製作第一個接續封包時漏帶nested fixtures-512：B0在HTTP前缺檔，P0於載入中受控停止。第二次CPU前置又誤用uint32-byte指紋比較原逗號分隔ID指紋；它在production drain前攔下、未占GPU。兩次錯誤均獨立封存。依原generator格式把三原payload／IDs／SHA逐一exact驗證後，才執行最後兩個原512K案例；沒有更換題目、預期答案、原指紋或容差。

C2與C4未重跑；C1/C3與512K取自各自新收據。18個邏輯階段逐一列來源SHA於analysis/combined-qualification06.json，原失敗ZIP不改寫。CPU分析器對partial身份缺欄位、header list/BGEN假設的錯誤亦留有原source與失敗收據，未因此重跑模型。

資源觀察分階段保留。原C2/C4及有效C1/C3窗口採樣native VmSwap與container swap均0；已中止的R2 P0長測載入曾見container cgroup swap 73744384 bytes，native VmSwap仍0。最後實際長測的資源結果見combined JSON；不以任何一段0宣稱整VM或全窗口zero-swap。

最後實際512K窗口的採樣：container cgroup swap峰值0 bytes，native VmSwap峰值0 KiB，OOM／OOM-kill counter最大值0／0。這些為模型／容器範圍，不能推成整VM zero-swap。

## 還原與封存

測後獨立確認現行v0.20 image sha256:<PRIVATE_RESTORATION_IMAGE>，live native/server與immutable image相同，9個production runtime及23個API runtime hashes吻合，兩張GPU與lease歸正式服務、外來controller/ExecStopPost已結束、foreign控制鎖無持有人。authenticated nonce：[redacted-authenticated-nonce]。Dashboard active、原unit/resource/hash與PVE helper條件保持；PVE/VM boot身份未變。依使用者授權還原現行v0.20，沒有退回舊f20。

| 證據包 | 檔數 | ZIP SHA256 |
|---|---:|---|
| E131 exact | 73 | 6d25ce8e69eb4064b954b73c8806004e8d46daacefe272ad3836349d48630326 |
| 原18窗口（含FAIL） | 2045 | 12fb304ea78e8ad98933831b72dd8ec01aee9122d8b3ce85ac2306e55cf7ac46 |
| C1/C3接續（含缺檔／中止） | 869 | 31484db44124a4e2744202e1d14365443c7213be07b33033e050f912c64c7b0a |
| CPU指紋前置FAIL | 78 | 6c9f119508b31d9b615006a0226dde2c66f00b55a98509a7a7feb85d993665b0 |
| 原512K兩案例 | 135 | 7710d8fa62a14c15c86a331980e19f0d56d3a856e2d945681c649e1e226a053d |

以上每包均已獨立核驗ZIP SHA、每檔CRC及manifest SHA。舊PVE故障事件與原始數值／拒答失敗未清除、未改判。Heartbeat維持PAUSED；未改driver、硬體或全域安全設定，未promotion。
