> Historical design specification; requirements are not completion claims. See VALIDATION.md for measured results.

# P0 — 雙 Socket 唯讀非 PLE 權重雙副本、PLE 單份：可驗證的記憶體拓撲

> **狀態：待實作（SPEC，不代表 test VM 已完成）**
> **基準：test VM 現有 Strata v0.1.44.1，對應 upstream tag commit `296b59d2f73c8f04a805df29c2fa97655adace78`。**
> **優先級：Blocking / P1–P3 的資料配置前置條件**
> **範圍：test VM Ubuntu guest + PVE 9.2 / QEMU host；本文件只定義待交付變更，不執行變更。**

## 1. 背景與問題

test VM 使用雙路 AMD EPYC 7663、RTX 3090 24 GiB ×2；PVE host 為 NPS4（理論上八個 host NUMA domains），客體 NUMA 編排、vCPU affinity、GPU-to-socket 對應必須實際讀取，不得以 NUMA ID 或 CUDA ID 推斷實體位置。先前 STREAM 觀測：雙路約 265.54 GB/s，Socket0 132.16、Socket1 133.20 GB/s；這是**既有歷史基線**，不是新架構的量測結果。

**必須修正的現況：**「非 PLE 唯讀權重雙副本、PLE 單份」尚未真正完成。單純將同一個 `mmap` 映射兩次、`numactl` 啟兩個 reader、或在 guest 內觀察到兩個指標，都**不能證明** host 有兩組互不共用的物理頁。Strata 目前有 expert mmap/補集 resident copy/GPU expert cache/PLE I/O 等機制，但**不是**滿足本 SPEC 的實體雙副本實作。

Strata v0.1.44.1 與此需求相關的實際程式：

- `src/core/expert_source.cpp` + `include/strata/core/expert_source.hpp`：`FileExpertSource`、`make_cache_complement_plan`、resident budget、mmap/file tier、pinning，含 NUMA-aware 的保守空閒記憶體估計 `numa_pin_free`；該估計不等於真正 placement。
- `src/platform/memory.cpp` + `include/strata/platform/memory.hpp`：`mlock` / `lock_resident_ranges`、readahead、I/O counters。
- `src/kernels/ngram.cpp` + `include/strata/kernels/ngram.hpp`：`PleTable`、16 rows/token、`Direct`/`Mmap`、`--ple-io ram` 的鎖頁模式。
- `src/core/weights.cpp`：dense GPU weight loader；**GPU-only 的 dense 權重不應為了「非 PLE」這個名稱無條件複製到兩顆 CPU**。
- `docs/MULTI_GPU.md`：layer split、stage-resident experts、prompt lend，必須保留。

## 2. 精確定義（禁止混淆）

| 名稱 | 本專案定義 | 是否複製 |
|---|---|---|
| **CPU-visible ROM（非 PLE）** | **所有在 Host RAM 中需要保留的非 PLE 不可變權重**之 manifest；初期以 CPU expert kernel 實際讀取的 expert blobs 作主要 workload，並逐項盤點 embeddings／shared weights／dense 等是否存在 CPU 消費者，禁止未盤點就宣稱完整覆蓋 | **兩套獨立匿名實體頁**，Socket0 / Socket1 各一份 |
| GPU-resident experts / dense weights | GPU VRAM 中供各自 stage 運算的 tensor / cache | 繼續遵照 v0.1.44.1 layer split / cache；**不計入雙 CPU ROM** |
| **PLE table** | 一份模型級 immutable `per_layer_token_embd.weight`，來源 GGUF tensor | **一個邏輯 owner、一份 backing store/映射或 direct reader**；不得偷偷為第二個 Socket 建立全表副本 |
| PLE per-request state | `ple_prev`、history、token window、每 slot 的 scratch | **每個 session 獨立**，不能因「PLE 單份」而共用序列狀態 |
| PLE row cache、DMA scratch | 有上限的衍生短期資料 | 可各執行者持有暫存，但**不得演變成第二份全表**；總量有 telemetry |

**兩種容量配置需明確選擇，不得口頭混用：**

1. **P0-FULL（正確性對照）：** 在每個 Socket 各自建立完整、連續可定位的**盤點清單中的全部 Host-resident non-PLE immutable weights**（含 CPU expert image）；逐項校驗 `(layer, expert)` 與任何其他 host-consumed tensor 均能獨立讀到相同內容。適合有足夠 RAM 的功能驗證；所需 bytes 由當前實際模型 manifest 決定，不寫死 34/47/54 GiB。
2. **P0-COMPLEMENT（正式容量最佳化候選）：** 只複製「未被該 GPU stage cache 覆蓋但 CPU 會讀取」的**穩定補集**；adaptive cache 交換、prompt lending、stage split 更新時，必須具有原子版本化的來源回退。若任一 expert fallback 會跨 Socket 或被重新映射而未經驗證，退回 P0-FULL。此模式另立測試，**不能以 P0-FULL 過關代表它過關**。

**Scope gate：** 在 Coding 之前，由 `weights-scope-manifest.json` 逐 tensor 列出「CPU 會讀 / 僅在 GPU 計算 / 原始磁碟 backing / PLE」，並確認使用者所說的「非 PLE 全部權重」是否也包含 GPU-only host staging；若要求最嚴格的所有非 PLE host bytes 雙份，P0-FULL 必須照清單實作，**不得默默縮成 CPU experts**。P0 先完成完整雙副本證據，再獨立加入補集版本；不強求 GPU-only VRAM tensors 在 CPU 再存兩份，除非嚴格範圍明確要求。

## 3. 需求與不變條件

- **R0.1 資源盤點先於任何修改。** 保存 VM 設定、有效 kernel/CUDA/driver、二進位與 image 的 SHA256、模型 GGUF/pack 與 profile 的 checksum、完整啟動 argv/environment；確認已部署二進位對應 v0.1.44.1（tag 不代表 VM binary 必相同）。
- **R0.2 NUMA 與 GPU 事實表。** 讀取 host `lscpu -e=CPU,NODE,SOCKET,CORE`, `numactl -H`, `lstopo`, `nvidia-smi topo -m`；guest 同樣讀取 NUMA、`Cpus_allowed_list` / `Mems_allowed_list`、CUDA bus ID。用 PCI bus→host node→guest allowed cpuset 的**實際**交集建立 `PlacementManifest`；不假定 Guest 直接顯露 NPS4 八節點。
- **R0.3 兩個 ROM 身分獨立。** 不允許兩個 pointer 指向同一 `mmap` file-backed page cache；使用 `MAP_PRIVATE | MAP_ANONYMOUS`（或經驗證確實為私有的 allocator），由指定 node 的 reader/copy worker 填入並在填充結束後 `mprotect(PROT_READ)`。兩份內容雜湊一致、資料頁不同、允許不同虛擬位址；需保存物件生命週期與對齊資訊。
- **R0.4 NPS placement。** 對應 Socket 的四個 host NPS domain 是**候選分區**，在 VM guest 看不到相同 NUMA 時，不得硬指定無效 node；可先 socket-local coarse placement，之後由 P1 按 `(layer, expert)` 的 owner domain 在兩個 Socket 內分片。使用 cpuset、`mbind(MPOL_BIND)` + page-touch；失敗時 fail-closed，不能默默變成 remote pages。
- **R0.5 防止 KSM 去重。** 優先確認 test VM 使用的 `memory-backend-*` RAM objects 實際屬性 `merge=false`，並用 host RAM backend/KSM 頁統計與物理映射稽核。不要直接切全 host `ksm=0` 影響其他 VM；`merge=false` 屬 VM backend 的設計修改，是否可 live `qom-set` 需先確認 QEMU 版本與 property 是否可改，**不可假設運行中修改必然生效或重啟後保留**。必要時循受控停機/重啟流程套用已核准的 VM 設定。
- **R0.6 讀寫分離。** ROM 一旦 `READY` 後沒有任何寫入路徑；`mprotect(PROT_READ)` 是否與現有 `cudaHostRegister`/pinned DMA 相容**必須實測**，不相容就使用另一個有界的可寫 pinned staging，不能移除唯讀保護假裝通過。GPU adaptive evict 不得寫入唯讀 ROM，必須放在獨立可寫 cache/staging。禁止以 `MADV_UNMERGEABLE` 在 guest 當作 host `merge=false` 的證據。
- **R0.7 PLE 單實例。** 只建立一個全域 immutable PLE `TableOwner`，由 reader 提供 `issue()/collect()`；`--ple-io direct|mmap|ram` 需以當前模型、NVMe 與 4 併發讀取模式 A/B 測，不擅改 upstream default。`ram` 模式全表 mlock 可能造成 swap 或內存壓力，嚴禁未通過容量門檻即啟用。PLE session history 永遠 per slot。
- **R0.8 生命週期與失敗語義。** `DISCOVER → PLAN → ALLOCATE → COPY → HASH → PROTECT → VERIFY → READY`；任一步失敗時銷毀候選並回復單副本舊路徑，不發布半初始化的 pointer。`SHUTDOWN` 等待所有 readers/DMA 完成，停止依賴後才 `munlock/munmap`。
- **R0.9 不修改生產基準。** 新功能 feature-gated；B0 原本 flags 與數值輸出均可完整還原；禁止同時改量化、MTP、KV、GPU cache、PCIe/P2P 驅動。

## 4. 目標資料流與物件契約

```text
                Config / Model manifest (SHA256)
                              |
                        PlacementPlan
             +----------------+----------------+
             |                                 |
    Socket 0 ROMImage A                 Socket 1 ROMImage B
    anonymous/read-only                 anonymous/read-only
    unique host PFNs                    unique host PFNs
    local work/NUMA domains             local work/NUMA domains
             +---------------+-----------------+
                             |
           ExpertView(layer, expert, socket, generation)
                             |
                       CPU expert pool

            PLE TableOwner [one model-wide table]
              | issue/read/prefetch/collect |
      Session0/1/2/3: private PLE history & temporary results
```

```cpp
struct RomImage {
    uint32_t socket_id;            // logical role, not a hard-coded Linux NUMA ID
    uint64_t generation;
    void* base;                    // private anonymous pages
    size_t bytes;
    std::vector<ExpertSpan> spans; // stable (layer, expert) -> offset/length
    Digest model_digest;
    bool read_only;
};

struct PlacementManifest {
    Digest engine, model, expert_layout, gpu_profile;
    std::vector<GuestNodeMap> guest_nodes;
    std::vector<HostNodeMap> host_nodes;
    std::vector<PciNodeMap> gpu_pci;
    uint64_t planned_bytes_per_socket, reserved_headroom_bytes;
    bool merge_disabled_verified;
};

const uint8_t* get_cpu_expert(int layer, int expert, int owning_socket,
                              uint64_t expected_generation);
```

**重要：** 總體 `get_cpu_expert()` 不能在 hot path 查檔案、加 mutex、建立新映射或分配記憶體。模型更新/VRAM cache 交換使用 generation+RCU 類型的引用壽命控管；初版可直接禁止 runtime reload。

## 5. PVE Host / Guest 操作約束

以下是**只讀盤點命令**（以實際權限與位置執行），不是一鍵改機腳本：

```bash
# PVE HOST
qm config 102
qm showcmd 102 --pretty
numactl --hardware
lscpu -e=CPU,NODE,SOCKET,CORE
nvidia-smi topo -m || true  # GPU passthrough 時 host 可能看不到卡
cat /sys/kernel/mm/ksm/pages_sharing /sys/kernel/mm/ksm/pages_shared

# test VM GUEST
uname -a; lscpu -e=CPU,NODE,SOCKET,CORE
numactl --hardware
nvidia-smi --query-gpu=index,pci.bus_id,memory.total --format=csv
numastat -p "$(pgrep -n -f 'strata.*engine')"  # 先人工核對 PID
```

- `qm showcmd` 展開的是**預計命令列**，不是獨立證明 running QEMU 物件的狀態；要加上 running QEMU command line / QMP 查核。
- Guest `move_pages(pid=0,nodes=NULL)`、`/proc/<pid>/numa_maps` 可驗證 guest NUMA location，**無法單獨驗證 PVE host 的不同實體 PFN**。
- Host 使用授權的 QEMU address translation / `pagemap` 或等效映射稽核將兩段 guest RAM 關聯到 host physical pages；若限權、KSM 或大頁導致無法取得可信證據，明確標記 `UNVERIFIED`，不得宣稱「實體雙副本完成」。
- HugeTLB / THP / pinned host pages 彼此有配額及鎖頁限制；先記錄再小規模 A/B。禁止全局 `drop_caches`、關閉其他 VM KSM 或改全機 NUMA balancing 作為初版必要條件。

## 6. 開發工作包與 PR 切分

| PR | 工作包 | 可交付輸出 |
|---|---|---|
| P0-A | Inventory + B0 capture | `baseline.json`, `host-topology.json`, `guest-topology.json`, model/engine SHA256, benchmark seeds |
| P0-B | `RomImage` 獨立分配/複製/唯讀保護 | `RomImage` allocator、owner + generation、兩份 hash verifier、fallback |
| P0-C | `PlacementManifest` + NUMA plan | topology probe、guest/host mapping、bind/touch／fail closed |
| P0-D | Host backend `merge=false` 受控設計 | PVE/QEMU 配置 diff、reload/restart 流程、host 實頁稽核紀錄 |
| P0-E | PLE `TableOwner` 單實例契約 | 單 table、per-slot state、`direct/mmap/ram` 實驗與 buffer budget |
| P0-F | E2E tests 與資源閘門 | correctness、OOM/rollback、long context smoke、報告 |

## 7. 測試矩陣

- **P0-T01：** 來源校驗：兩份 ROM 的大小、scope manifest 每一筆非 PLE tensor、expert 數、每個 `(layer,expert)` 的 bytes/hash/offset 與原版來源相符；沒有遺漏既有 CPU 實際消費的其他非 PLE 權重。
- **P0-T02：** 權重唯讀：任何寫入候選都只能使**測試子行程**收到預期 fault，不能在生產服務內刻意寫入引起 crash。
- **P0-T03：** Guest NUMA placement：完整掃描 + 抽樣 `move_pages`，依配置 local node 範圍；記錄跨域與未知比率。
- **P0-T04（阻斷）：** Host backend `merge=false`、KSM/host PFN **distinctness**、同一 model 並非兩個 file-backed alias。抽樣地址與抽樣方法需列入 `pfn-evidence.json`。
- **P0-T05：** GPU pinned staging、adaptive cache swap、prompt lend、restart 均不寫 ROM，reference lifetime 無 UAF。
- **P0-T06：** PLE owner 不重複，16-row gather/hash、EOS/missing prev、`Direct/Mmap/RAM` parity；短上下文與冷/熱表各驗一次。
- **P0-T07：** 至少兩個獨立 sessions 交錯使用 PLE，`ple_prev`、history、KV 不交叉污染。
- **P0-T08：** 256K/512K 僅在同量化配置實際可載入且符合記憶體預算時跑 smoke+recall；不能把 `--max-context` 可設定當成 recall 正確。
- **P0-T09：** OOM、單 node RAM 不足、partial `mlock`、checksum mismatch、QMP 不支援、kill/restart 注入 → safe fallback；不能有 silent remote fallback。
- **P0-T10：** 同設定 B0 與 P0 對照，grep/記錄 swap、`pswpin/out`、NUMA migrations、major faults、VRAM、TTFT、decode、engine restart 次數。

## 8. 建議的阻斷式驗收標準（事前設定的 gate，不是效能承諾）

1. **功能：** P0-T01～T09 全通過；無讀到未初始化 ROM、UAF、競態、PLE state 污染、非法 SIGBUS/OOM kill。
2. **物理：** guest + host 兩層 placement 具有可重現證據；兩份 mirror 的 host page shared-identity 抽樣為 **0**，樣本規模與可見性限制明載；KSM merge 配置可重啟後保留。
3. **容量：** 每 Socket 峰值 RSS、locked bytes、MemAvailable 有明確安全餘量（初始提案：保留至少 `max(16 GiB, 每 Socket 可用 RAM 的 10%)`，實際以 test VM 常駐服務修正），無 swap、OOM；若不足不得啟用 FULL。
4. **正確性：** 保持相同 router/quant/VRAM/MTP 路線時，理論上只複製 bytes 而不更動算術；預設要求同種子 greedy 輸出與 per-expert float checks bitwise identity。不可直接聲稱不同 GPU tier/算術次序也會 bitwise。
5. **效能：** P0 目標是**記憶體正確性**；先要求 p95 decode 不劣於 B0 超過 3%（可調 A/B gate），不是承諾 P0 必加速。

## 9. 失敗與回滾

- Feature flags（**提議的新旗標，非現有 upstream**）：`STRATA_DS_ROM=off|full|complement`、`STRATA_PLE_OWNER=single`、`STRATA_NUMA_STRICT=0|1`；全部預設 off，確保 B0 原路徑不變。
- 任一實頁稽核／hash／RAM headroom 不符合：不啟動 P1；保留上一個 stable image，重新啟動 B0。若更改 QEMU backend，回滾需使用原始 `qm config` 快照及受控 VM restart；不要以熱更改當作可靠回滾。
- GPU/PVE host 的重啟配置變更與 Strata application 變更分開 PR；不合併後再查根因。

## 10. 完成定義與交接

交付 `P0/spec.md` + 原始碼 PR + `baseline.json` + `placement-manifest.json` + `pfn-evidence.json` + `ple-ownership-report.md` + `P0-test-report.md`。只有 `T04` 真正確認物理隔離，才能將 P0 標成 `DONE`，否則維持 `PARTIAL / UNVERIFIED`。

## 11. 來源（固定版本）

- Strata v0.1.44.1 正式版：<https://github.com/Niko1221/Strata/releases/tag/v0.1.44.1>
- 實作：<https://github.com/Niko1221/Strata/blob/v0.1.44.1/src/core/expert_source.cpp>、<https://github.com/Niko1221/Strata/blob/v0.1.44.1/include/strata/core/expert_source.hpp>、<https://github.com/Niko1221/Strata/blob/v0.1.44.1/src/kernels/ngram.cpp>
- v0.1.44.1 PLE 定義：<https://github.com/Niko1221/Strata/blob/v0.1.44.1/include/strata/kernels/ngram.hpp>
- Linux NUMA：<https://man7.org/linux/man-pages/man2/mbind.2.html>、<https://man7.org/linux/man-pages/man2/move_pages.2.html>
- QEMU memory-backend `merge`：<https://www.qemu.org/docs/master/system/qemu-manpage.html>

---
**依賴：** P0 先於 P1；P2 可先做 isolated kernel microbench，但不併入 P1/P3 服務路徑，直到 P0 通過。
**嚴格禁止：** 未量測就把 NUMA 假設當事實；未確認 host physical page 不共享就宣稱 ROM×2 已完成。
