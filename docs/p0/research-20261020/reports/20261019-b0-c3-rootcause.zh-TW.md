> Historical sanitized export. Source SHA256: `be7c1b54583ce1b2683a67d1e26dd560ae703347305f3b5717a1784a6bacadf7`. Original failures and measurement scope are retained. Receipt links refer to private artifacts, not publicly downloadable raw data.

# 原始 B0 C3 拒答、p95 與 Strata PR 關聯調查

2026-10-19 12:32，Asia/Taipei。**本輪已完成，原f20服務與API已恢復並驗證；P0維持NOT_PROMOTED。** 原始B0拒答的adaptive／混合backend機制已重現，GPU-only候選通過本輪C3與短decode p95點估計，尚非production資格。

## 已確認的結果

原始 B0（P0 OFF）的 benign 檢索請求可重現拒答。固定同一 suffix OFF、native input、二進位、容量，adaptive ON→OFF→ON 會依序得到「第三次拒答→三次答對→同一第三次拒答」。兩組 ON 的三份全詞彙 FP32 logits 各自逐位相同，不是只比較文字。私有觀測版也逐位重現原始結果；完整狀態快照／GPU 同步沒有掩蓋此案例。

此fixture的可重現機制是 **adaptive expert residency 改變 CPU／GPU 算術分工，再經 GDN／KV 狀態及多層計算放大，改變 greedy 選擇**。使用既有 `--pcie-frac 1`，將 missed experts 也交給 GPU 後，仍觀察到 316 個 resident membership 變動、總 resident slots 保持18,676；但 GDN／PLE／KV／indexer 狀態與三份完整 logits 全部逐位相同，固定請求3/3答對。這是此 fixture 的反事實證據；不代表所有模型／請求或 P0 C2 已通過。

## 原始二進位與固定請求

- B0 image：`d99ba0e864c490aecf6c1355abf45c66183e3623495214c6cd3190f5c640ecfd`。
- Native SHA256：`4ea6a2fa3361ec22d764944a570dd76ec03106295687e17b343798a18e106ff4`。
- Native input：4,129 tokens，SHA256 `7998199aa7affc766a57d64c48d507c38540252455d86b6886be1efb9a5483b1`。
- 預期輸出：`SLOT-0-RVN-101`。相同 benign payload、temperature0；保留完整 SSE、native token/protocol ledger、248,320個 finite FP32 logits。
- 單 native、雙3090 layer split24、batch4配置；cache9988/8688、loan2020/1832、chunk8192、ring96。Suffix OFF 時明確 spec6/MTPcap4 保持原本有效 verifier allocation6；沒有開啟 MTP。
- 歷史預處理為固定2輪C4與封存 numbers/story/explanation/code 請求，不能宣稱每次 concurrent scheduling 都與舊失敗完全相同。

## 原始 B0 九案例

| 案例 | 結果 | 解讀 |
|---|---|---|
| 01 original history20 | PASS | history、API9、coding8與input audit |
| 02 cold target×3 | 3/3答對 | 首 logits 隨後續歷史變動 |
| 03 serial prehistory×3 | 3/3答對 | 陰性試驗，不是修復 |
| 04 suffix OFF，同有效容量 | **第三次拒答** | suffix／BGEN 並非必要條件 |
| 05 adaptive OFF，原 suffix | 3/3答對，head bitwise一致 | 固定 residency 消除此 fixture，但效能有代價 |
| 06 original serial AFTER | 3/3答對 | ON 時 head 再次隨歷史變動 |
| 07 original history20 AFTER | **FAIL_RETAINED** | code 重複率.31998≥.30；在 recall 前停止，不能算拒答試驗 |
| 08 suffix OFF＋adaptive OFF | 3/3答對，head bitwise一致 | 與04/09匹配的反事實 |
| 09 suffix OFF＋adaptive ON AFTER | **同一第三次拒答** | 三份完整head逐位等同04 |

04／09 top1依序50、50、40，margin .0628604889、.3832683563、.0767173767。08三次均top1=50、margin .1199684143。good static與bad head差異maxabs1.05028725、RMS.19116063、p99.9 .68038302，並非可以用固定1e-4容差略過的最終差異。

實際拒答開頭為「I cannot fulfill the request to reply only with the code, as the provided text contains repetitive filler content…」。首個 native GPU head 已選到拒答 token；不是HTTP wrapper將正確文字換成拒答。

機讀證明：causal-on-off-on.json (private receipt: `causal-on-off-on.json`)。

## 內部狀態與觀測界線

觀測版只在原始 verify.cpp 加 opt-in 資料擷取，未修改 kernel、計算順序或 routing。STATE0不做整體狀態同步，STATE1同步後擷取完整 owned GDN／PLE／indexer／有效prefix KV。兩種模式的 ON／OFF full heads 各自逐位一致；每個案例六個stage windows均完整，未達1GiB上限。

ON的good repeat0與bad repeat2，在最終T1/pos4128進入前已有狀態差異：GDN第0、1層逐位一致，第2層起不同；第3層KV差異只落在接近reuse邊界的少量尾列，並非整個既有prefix都不同。這排除了「最後一個 Token window 才開始分歧」的說法，但單靠狀態不同不能區分合法算術歷史與restore/copy錯誤。

GPU-only反事實保持316個resident membership變動，所有擷取的有效狀態卻逐位一致；96個paired layer windows中CPU entries確實0，並且三個全head SHA都是 `a89b710d19f6875881f038dda2750cecf6427d268f6b11d5501ffba39e2332f1`、top1=50、margin .4456233978。

觀測有效性已依原始source逐項校驗：`doorbell_publish_res` always發布IDs，但只有miss時更新activation，且不發布router weights。因此all-hit activation與weights舊值一律排除。`h_ymiss`的GPU rows是未更新緩衝區，不能當實際GPU輸出或保證為零；只比較plan-dst以外真正CPU rows。沒有用這些舊值假稱第0層故障。原始檔案完整保留。

分析：state pre-difference (private receipt: `prestate-good-vs-refusal.json`)、GPU-only counterfactual (private receipt: `uniform-gpu-counterfactual.json`)、head equivalence (private receipt: `head-observer-equivalence.json`)。

固定activation micro witness已完成精確pool dispatch驗證。實際layer1/expert193權重是IQ2_S／Q2_0，不可只看IQ3_S檔名推斷所有tensor格式。CPU gate/up input使用Q8_K，Q2_0 fast-down使用Strata planar ActQ（FP32 scales／half-away rounding），GPU使用Q8_1。第二版重放的CPU結果與真實trace row6 **bitwise相同、誤差0**；同activation／同GGUF bytes的GPU結果與CPU的2560維全部不同，maxabs **0.000986292026937008**、RMS **0.0002207569152784384**，各backend自身重算均bitwise一致。這直接證明此expert的backend算術不等價；結合residency反事實，可定位此fixture的混合backend歷史依賴機制，但不宣稱排除所有其他state/copy缺陷。

Activation SHA `382d23cdae9eee31b5cbc14c8a305523a8e0a78e7d0cdfba14271e267aa3511f`；weights SHA `9cf051e17fac23a80f00a865db8853f2b87441c852bb155b28fec99f986cf2f9`。第一版generic `native_down_rows`未走真實pool的Q2_0 fast-down path，重放CPU差3.06219e-5，完整保留並排除exact replay主張；第二版只調整witness呼叫既有dispatch，沒有修改kernel。

機讀驗證：trace-witness-verification.json (private receipt: `trace-witness-verification.json`)。原始CPU／GPU向量、相同權重blob與kernel log都在相同資料夾；本地已驗證封存所有487檔案的CRC／SHA256。

第二版micro執行成功後，Docker --rm在list→inspect之間自動移除容器，收尾函式拋錯。該窗口保留failure、已恢復f20並authenticated確認；不是GPU kernel執行失敗。後續獨立窗口只接續未跑的C3與clean timing；收尾修正僅在inspect失敗後，以新成功的container absence查詢＋新成功的GPU空集合確認清理，任何查詢失敗／同名外來容器／外來GPU都不當idle。五項CPU mocked contract檢查通過；沒有重跑已完成的模型結果。

## PR 關聯

使用者明確要求的 read-only subagent 已完成目前上游／fork三個PR稽核。未push、merge、留言或改PR。

| PR | 本次關聯 |
|---|---|
| [上游#2886](https://github.com/Niko1221/Strata/pull/2886) | 修HTTP replica pipe／STOP-DONE ownership；repair A已存在拒答B0 image；本次config實際建StrataEngine，未走ReplicaEngine。無直接關聯證據 |
| [fork#1](https://github.com/dynaxbill/Strata/pull/1) | ROM、CPU pointer、PLE owner與cache identity有機制交集，但P0 OFF原始B0也失敗；不是必要成因或已驗證修補 |
| [fork#2](https://github.com/dynaxbill/Strata/pull/2) | 僅host backend稽核文件，不改token計算 |

三個PR目前均未merge；沒有maintainer根因認定可引用。使用者issue#2138是adaptive-overlap效能問題，`STRATA_ADAPT_OVERLAP=0`不等於adaptive swaps OFF。完整精確head／source分支／SHA證據見PR稽核報告 (private receipt: `REPORT.md`)。

## p95 與低延遲對策

舊p95退化已另有獨立重現：完整`/proc/PID/numa_maps`觀測持有mmap_lock約1.05s，阻塞native mmap/munmap約1s。相同native的heavy／OFF／heavy p95為30.387／27.011／31.306ms。低干擾觀測及ROM啟動配置案例B0／P0／B0為26.712／25.973／26.762ms；不得覆寫舊+15.12%失敗，也不能用此結果豁免數值gate。

針對B0拒答的立即候選是既有`--pcie-frac 1`，讓expert cache只影響資料位置，減少因CPU／GPU切換引入的算術變動。不是關閉CUDA Graph、prefix、prefill、PLE，亦不是禁止拒答Token。另一個診斷方法adaptive OFF在story案例約42.7t/s，相對ON約69–70t/s，不能假定它是高效的release fix。

原始d99二進位、capture OFF、same-capacity的GPU-only與B0 A/B/A短量測及完整C3已完成。每phase32個request samples，只能作診斷點估計，不能代表所有長上下文／冷cache／高併發。第一個before phase碰到CPU witness編譯負載，已排除效能驗收並保留原資料；最終使用獨立續跑窗口07/08/09無編譯干擾的clean A/B/A，不挑選成功結果。

原始d99＋`--pcie-frac 1`、原suffix設定的完整C3 **PASS**，包含precondition、history20、input audit、API9、coding8，unknown native inputs0。獨立GPU-only bench03 p95=26.961ms、B0 bench04=26.826ms（約+0.50%），仍保留作為另一組點估計，不只引用較快的clean結果。沒有重跑已完成的C3或微測試。

06原始d99二進位、suffixOFF／有效verifier6與相同封存prehistory／native input，GPU-only **3/3精確回覆SLOT-0-RVN-101**、unknown native inputs0；三份完整logits均finite且逐位等同觀測版GPU-only結果。這補上不使用私有observer的反事實驗證。

最終clean A/B/A（每phase32requests、排除前2warmup rounds）：

| 指標 | B0 before | GPU-only | B0 after |
|---|---:|---:|---:|
| Decode p95 ms/token | 26.853 | **26.811** | 26.930 |
| E2E TTFT p95 秒 | 1.0234 | **1.0295** | .9643 |
| E2E request p95 秒 | 7.3133 | **7.3087** | 7.2748 |
| Median round aggregate t/s | 140.74 | **140.34** | 141.54 |

GPU-only decode p95相對兩個B0為−0.16%／−0.44%，通過事先3% **decode點估計**限制（spec第155行指decode，不能當TTFT門檻）。TTFT相對前後B0平均約+3.59%，E2E p95約+0.20%；另一組03／04的TTFT為1.0451／.9769秒。故候選消除此拒答且保住短decode延遲，**不宣稱全面降低首Token或所有尾延遲**。

三組實際原始native SHA相同，config差異只有`--pcie-frac 1`，cache容量一致，每phase native PID未重啟，timing時first-logit／frontend trace關閉。367份light samples中model cgroup swap／OOM counter最大0、監測錯誤0，但全VM仍有swap且各phase不等（07 IN/OUT790/538、08 1913/1667、09 11239/12018 pages）；**沒有正式整窗零swap或嚴格隔離的效能資格**。32樣本與背景差異限制了統計推論。

機讀結果：latency-comparison.json (private receipt: `latency-comparison.json`)、identity／C3／resources qualification (private receipt: `qualification.json`)。

可實作的短期對策是將GPU-only設為獨立B1 profile候選，保留既有Graph／prefill／prefix／PLE／routing與cache容量，讓VRAM residency和PCIe傳輸只影響存取成本。正式採用前仍須以新profile自身的B0-style envelope、完整品質與長上下文／冷cache負載驗收；本輪沒有推廣production。若後續仍需CPU分攤，中期工作是對齊CPU／GPU activation quantization及逐階段expert算術，或固定logical backend使adaptive搬移不改變數值路徑；這是獨立kernel／policy工作，不屬於P0唯讀權重擺放的放行條件豁免。

## P0、512K與故障狀態

P0 resumed round已完成：固定CPU288 bitwise PASS；C3 reuse+slot-byte verification PASS；真實524088 input tokens的512K B0/P0三深度檢索6/6 PASS。但 **C2 strict FAIL125/144outside、1newtop1，P0維持NOT_PROMOTED**。512K超出GGUF metadata262144，只是本題組單請求檢索成功，尚非512K C4／完整品質資格。HIP／SYCL完整build/device測試NOT_RUN，本機沒有相關toolchain/device；CUDA不能替代。詳見P0測試報告 (private receipt: `REPORT.md`)。

PVE非預期電源／重啟根因仍未知；SEL、kernel、時間校正證據與舊失敗ZIP均保留，未清除事件、修改全域kernel／driver／NUMA／安全設定。最終已fresh核對原f20 image、九個runtime hashes不變、boot不變、兩個GPU native PIDs皆屬production container；API實際回覆nonce **B0-C3-FINAL-RESTORED-73d70aecc5de487f**。所有診斷controller已結束，GPU目前由原production使用，並非idle。沒有production promotion。

恢復證據：restoration-verified.json (private receipt: `restoration-verified.json`)。所有失敗與原始證據已本地CRC／逐檔SHA256驗證：initialB0 archive809files、trace五個phase ZIP與61file control、remedy失敗窗口487files、續跑窗口713files。續跑ZIP `rc19-remedy2-evidence.zip` SHA256 `4b07a35c4fc05d3ca68de2ef1b2b294868363dbd5232bb66213ba11437ad3637`；未攜出API key／private full runtime backup。
