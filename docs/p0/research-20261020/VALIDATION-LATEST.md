# 當前候選驗證矩陣

整體NOT_PROMOTED；原FAIL不覆寫、原九向量包絡及容差不變。

| 項目 | 狀態 | 有界證據 |
|---|---|---|
| E449首差／因果／exact micro | PASS | d3ee…：只換backend且保留名額重現完整新head，兩路各trace/self0及GPU權重0 |
| 原math/kernel物件 | PASS | 新B0/P0各93物件前後SHA相同，新增3個memory/test物件分列 |
| Dual-GPU Graph/canary/overflow alias | PASS | 264組、兩張RTX3090，無模型 |
| 新B0/P0 C2對原9向量包絡 | PASS | 各0/144超界、0新Top1、全部匹配舊真向量 |
| 同profile B0/P0 head | PASS | 144/144 bitwise0 |
| POOL observer head gate | PASS | 144/144 bitwise0；34704 actual dispatch jobsCPU0、maxPCIE23 |
| Clean C4原3% p95 gate | PASS | B0 26.879/26.838，P0 26.020，N64/arm、同工 |
| Native/cgroup swap及OOM | PASS觀察 | 監測均0，非全VM宣稱 |
| 全VM zero-swap IO | FAIL | pswpin7166/out7254；原run保留 |
| Dashboard cgroup資源 | FAIL／待授權診斷 | 256MiB cap、約438MiB swap；無P0測試20秒paging複現，未改服務 |
| NUMA容量完整閉合 | 未完成 | node0保守MemFree reserve未滿足；不等同MemAvailable不足 |
| 本新候選post-workload所有ROM span重hash | NOT_RUN | 不以startup0或歷史PFN樣本代替 |
| 原default125/144+1、後續8/144及舊C4 | 原FAIL保留 | 不是這次候選PASS回寫成歷史PASS |
| C3原BGEN3 badhead完整state復原 | NOT_SATISFIED | 既有受控GEN128同類拒答／E131根因與本項分開 |
| 新spill正常C3／C1／512K／並發head／soak | NOT_RUN | 不繼承舊VERIFY-ON資格 |
| HIP/SYCL | NOT_RUN | 本copy實驗CUDA-only，未請上游review |
| Production promotion／merge | 未執行 | 現行v0.20只作還原目標，未部署新候選 |
