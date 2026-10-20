# Follow-up: exact C2 cause and bounded PCIe spill candidate

Measured on 2026-10-20 on the same two-RTX3090 CUDA SM86 test guest. Historical specification: dual AMD EPYC7663,64 guest vCPUs; exact guest/driver inventory was not newly detected for this publication. Private original v0.1.44.1 B0/P0 identities and clean candidate hashes are recorded. Existing active observer/limits were retained; no driver, clock, kernel or global security changes. The current production version is only a restoration target, not the tested engine version.

## Passed

- The first changed Chinese head has a direct single-expert causal witness. With the same actual activation/IDs/router weights/shared, only L44/E449 backend changes. Reserving its original PCIe quota keeps E342 unchanged and reproduces the entire new248320-value head bitwise. CPU/GPU exact micro and self-repeat error0, correct GPU weight readback0. CPUq8_K/q8_0 versus GPUq8_1 are existing paths; this does not isolate one FMA or quantization operation.
- Same-old and same-new schedule B0/P0 each41 complete heads and3888 captured tensors are bitwise; B0 itself reproduces the same schedule-induced head fork. The original125/144+1 and8/144 failures are unchanged.
- The new opt-in memory-only spill candidate passes the **original unchanged nine-vector envelope**,0/144 outside/0 newTop1,144 genuine original B0 matches in each of B0/P0/observer cases. Same-profile B0/P0 and observer ON/OFF each144/144 bitwise, maxabs0. New B0 samples are not added to the baseline.
- Original34704 plan/run records show CPU expert jobs0, max PCIe groups23,16 actual records above16. Two physical GPUs pass264 bounded-copy Graph/canary/alias checks. Original93 math/kernel objects remain bitwise for both private builds.
- Clean C4,18 rounds/two excluded/N64 per arm: B0 26.879/26.838, P0 26.020 ms/token. Original+3% point gate passes against both brackets; same request/answer/cache distributions. This is p95 of request-average decode, not streaming ITL, and is not a contemporaneous spill-ON/OFF speedup experiment.

## Failed or incomplete

- Whole-VM zero-swap I/O fails: swap-in7166/out7254 kernel counters in the monitored window. Model/native/cgroup swap and OOM remain0. The existing256MiB observer cgroup later has about439MiB swap despite ample VM memory;20 seconds without a new P0 workload reproduces observer paging. Its internal allocation cause is not established and no shared service was changed. This failure is retained, not hidden by the numerical or latency pass.
- Conservative node0 MemFree reserve is not met. MemFree is not MemAvailable; complete node headroom and post-workload all-span ROM rehash remain unclosed.
- Original C3 badhead lacks complete original slot/prestate, and old/new GEN/BGEN protocols differed. Earlier fixed-GEN same-class refusal/backend experiments remain valid in their bounded scope; they are not exact restoration of the original BGEN3 failure.

## Not run

- Final spill-profile normal C3/C1/512K/concurrent full-head/long-soak and HIP/SYCL. Earlier profile results are not transferred.
- Complete default-OFF model byte-equivalence for this new memory prototype, public-branch native rebuild, upstream review, merge or production promotion.

## Evidence

- [Current matrix](VALIDATION-LATEST.md), [E449 report](reports/20261020-e449-isolated.zh-TW.md), [spill report](reports/20261020-pcie-spill.zh-TW.md).
- [Causal witness](data/e449-causal.json), [independent ordered combine](data/e449-ordered-combine.json), [fixed-envelope results](data/spill-c2.json), [all216 C4 requests](data/spill-c4-requests.json), [C4 summary](data/spill-c4.json), [resources](data/spill-resources.json).
- [Private archive digest ledger](data/evidence-ledger-followup.json), [build provenance](data/spill-build-provenance.json), [experimental source artifact](experimental/pcie-spill/README.md), [exact expert helper](source/e449_expert_witness.cpp).

Full logits, activations, weights, addresses, private source/runtime backups and administrative controllers remain private. These are sanitized metadata and sources, not a public container image or complete raw numerical reproduction dataset. Historical files and failures remain preserved. **NOT_PROMOTED.**
