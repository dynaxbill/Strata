# P0 C2 修正與 C4：數值／延遲通過，整體資源仍失敗

2026-10-20，雙RTX3090 24GiB、CUDA SM86、原v0.1.44.1 B0/P0、MT1/GPU1/VERIFY0、layer split24、batch4/pool47。現行v0.20僅為已授權的production還原目標。本輪七階段各一次，沒有重跑到PASS或改原九向量包絡／容差；舊125/144+1、8/144及C4/C3 FAIL全部保留。

**新hybrid PCIe spill候選：C2原固定包絡0/144超界、0新Top1，144/144匹配真正舊B0向量；同profile B0/P0及POOL觀測器ON/OFF各144/144完整head bitwise。C4原3%門檻通過。整個VM的zero-swap gate失敗，未promotion。**

## 根因與最小修正

前一輪已以E449實際CPU/GPU exact、原ordered-combine逐位重算及只保留其原配額的單expert消融，完整重現新Chinese head。鏈條是：前序draft排程→usage/residency→16個PCIe名額飽和→E449落CPU而非GPU→L44 expert/combined→L45 activation→完整logits。原ROM不需要出現位元損壞就能觸發此有界差異。

本修正保持實體16-buffer（G2各8）、cache9988/8688、loans2020/1832、chunk8192/ring96。只在opt-in STRATA_P0_PCIE_SPILL=1/mode2下提高logical plan上限至min(64,plan.cap)；新的獨立memory-copy/rebase物件只處理前per個miss，overflow保留原pinned GPUalias，交原GPU expert kernel及原ordered combine。原93個math/kernel物件各自逐位未變，沒有更換量化kernel、Graph、prefill/prefix/PLE或正常SFX。新source僅是私有CUDA研究候選，預設OFF的完整模型byte-equivalence及HIP/SYCL未另測，不要求上游review。

原fetch_blobs的cap只檢查正數，實際kernel讀完整*n；rebase也處理全*n。因此不能直接擴mode2 cap而沿用原無界copy。此次先在兩張GPU通過264個Graph/canary/alias檢查，涵蓋per8/16、16/256/4096-byte blobs、n0/1/7/8/9/15/16/17/23/60/64及兩pattern，再啟動模型。

## C2正式重算

原比較器SHA9749198a595a9ea1e942ff235462c535e63d3f6dbc1977abbcf0b8fe3e03608b、原baseline ZIP12fb304e…、原每prefix九向量及所有公式／門檻不變；新B0沒有加入基線。每案48prefix×3repeat、144個248320-F32完整finite heads，實際GEN2/top_k30/seed8020及inputSHA全核對。

| 新profile | 原包絡超界 | 新Top1 | 匹配原B0真向量 |
|---|---:|---:|---:|
| B0 spill ON | 0/144 | 0 | 144/144 |
| P0 spill ON | 0/144 | 0 | 144/144 |
| P0 spill ON＋既有POOL trace | 0/144 | 0 | 144/144 |

B0/P0直接逐位144/144、maxabs0；POOL ON/OFF亦144/144、maxabs0。既有dispatch的34704個publish/run配對完整：CPU expert jobs總0，實際最大23個PCIe groups，16個紀錄超過原16-group上限。這證明確實走到overflow，非只測未觸發分支；不是宣稱總CPU活動為0或所有未測profile皆無fallback。舊失敗run仍為FAIL，這是新候選的結果。

## Clean C4 p95

無POOL/tensor observer；每arm18輪排除前2，四請求、每請求256輸出、N64。三arm輸入、答案、cache分布相同、native未重啟。指標為每請求平均decode ms/token的nearest-rank p95，非streaming ITL。

| 指標 | B0 before | P0 | B0 after |
|---|---:|---:|---:|
| p95 ms/token | 26.879 | 26.020 | 26.838 |
| median ms/token | 25.542 | 25.144 | 25.4535 |
| TTFT p95 s | 1.0249525 | 0.7764657 | 0.9774088 |
| E2E p95 s | 7.3395452 | 7.0153762 | 7.2802030 |

P0相對前／後為-3.1958%／-3.0479%，原+3% gate PASS。只有各一次P0及兩次B0冷啟動；未宣告本輪spill-OFF夾測，不能把相較歷史OFF的差異當作單因子加速證明。也未覆寫歷史30.825/27.969或VERIFY ON/OFF的結果。

## 資源失敗及唯讀定位

556監測樣本、1060個native樣本：native VmSwap及model cgroup swap均0，model OOM/kill均0，無monitor error。但whole-VM GPU窗口pswpin7166、pswpout7254（kernel counters/pages）非0，zero-swap gate FAIL。P0 C4 phase為pswpin5/pswpout0；B0 C2及前／後C4也有swap-in。不能因model swap0就稱整VM零swap。

node0最低MemFree約3.94GiB，低於保守Free-only reserve18.86GiB；node1超過保守reserve。MemFree不是MemAvailable，不能據此斷言實際可回收容量不足；完整NUMA headroom及post-workload全ROM span重hash尚未重新關閉。

還原後VM MemAvailable250098240KiB。dashboard原MemoryMax256MiB/MemorySwapMax=infinity，其cgroup memory.current約239.5MiB、swap約438.6MiB、memory.max事件21514，OOM0；程序RSS約248.6MiB、VmSwap約210.2MiB。原PID與unit保持，沒有重啟或修改。其餘使用者scope亦有少量swap，不能把全部歷史IO一律歸它。

沒有新增P0工作負載的20秒唯讀見證：VM swap-in2867/out2485，dashboard CG in2633/out2485，max事件+38。這支持「共用observer在自身256MiB cgroup限制下paging」為當前資源失敗來源；尚未把內部分配器／特定cache歸為已證根因。server source、SQL及GPU reader已以全部字串值去識別化AST唯讀檢查，未碰peer source、unit、DB或swap設定。

一次原樣dashboard restart＋5分鐘觀察已形成private observer-operation contract並向使用者請求授權；尚未執行。這需要額外授權，因先前明示維持原dashboard/准入、不動peer檔案。不得自行停用observer、改MemoryMax/SwapMax或swapoff/drop_caches以湊PASS。若允許原樣重啟，也不會豁免這次失敗。

## 還原、保全與尚未完成

本unit inactive、MainPID/ControlPID0、Resultsuccess；現行已授權v0.20/API原profile、native/runtime hashes、GPU owners/lease、原active dashboard/PVE boot及authenticated nonce全部獨立核對。原PVE故障資料未清理。ZIP5ac6cc064b59607a8fb904c549b2d46529b0f5498baa91c0c36a99eb66b1ccd9，400836622 bytes、1324檔，整包與逐檔CRC/SHA已核驗。CPU分析及後續資源調查另保留，不改封存raw。

C2首差與此有界修正的數值／C4點gate已完成；整體P0仍NOT_PROMOTED。C3同類拒答機制已有E131等舊證據，但原BGEN3 badhead缺完整當時前態，未宣稱逐位復原；最終spill profile的正常C3、C1、512K、完整並發logits及長soak仍待執行。HIP/SYCL沒有實測資格。dashboard資源操作等待人類選擇，其間只作唯讀／本機CPU整理，不自動改共用服務或復啟heartbeat。
