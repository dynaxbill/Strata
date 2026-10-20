# P0 dual-socket ROM research — 2026-10-20

**Research archive, NOT_PROMOTED.** The latest follow-up establishes the Chinese C2 first cause with an isolated E449 backend ablation and exact CPU/GPU witnesses. A new bounded PCIe-spill candidate passes the unchanged original C2 envelope and clean C4+3% gate, but whole-VM zero-swap fails and final C3/512K/soak qualifications remain incomplete.

最新：C2首差因果與新候選數值／C4通過；原FAIL保留，全VM資源gate仍FAIL，不promotion。請先讀[完整新結果與限制](FOLLOWUP.md)和[當前矩陣](VALIDATION-LATEST.md)。下列舊資料均保留為其原配置的歷史證據。

## Read the results

- [Current validation matrix](VALIDATION-LATEST.md) and [historical matrix](VALIDATION.md): Passed, Failed, Not run and remaining work.
- [Research history](HISTORY.md): separates historical configurations and all failed gates.
- [Numerical requirements](NUMERICAL-PARITY.md): zero ROM/fixed-CPU error, fixed B0 envelope, margins and functional checks.
- [Earlier VERIFY ON/OFF report, Traditional Chinese](reports/20261020-c4-singlefactor.zh-TW.md).
- [C2 first internal difference](reports/20261019-c2-firstdiff.zh-TW.md).
- [C3 exact CPU/GPU witness](reports/20261020-e131-exact.zh-TW.md).
- [Earlier qualification and actual 512K recall](reports/20261020-qualification.zh-TW.md).
- [B0 refusal investigation](reports/20261019-b0-c3-rootcause.zh-TW.md).
- [Historical design specification](SPEC.zh-TW.md), which is not a completion claim.

## What is published

The original opt-in implementation remains at the existing engine paths from public P0 head `9f09b7c3755f8048be812d484e7917bd03bd2c1a`, based on upstream v0.1.44.1 `296b59d2f73c8f04a805df29c2fa97655adace78`. This publication adds research files; it does not silently replace that implementation with a newer engine.

The later tested image has 11 changed selected source files. Its [archived overlay](source/README.md) applies to the pinned public P0 source, reconstructing all 430 selected source hashes from that image. It includes diagnostic slot-copy changes and the historically used serving-wrapper dependency. Applying this overlay is a separate research action, not a claim that the newly published branch head was built or model-tested.

Public data includes all 432 C4 request records (including the 48 predeclared excluded warmup requests), C2 comparison metadata, full-head fingerprints, CPU witness sources and private-archive digests. Model weights, expert tensors, activation dumps, full raw logits, private runtime/control scripts, credentials, host addresses and physical mappings are excluded. [Evidence and data availability](EVIDENCE.md).

## Latest C4 single-factor result

Same machine and immutable images, four clients, 4K prompt class, 256 generated tokens; 18 rounds with the first two excluded, 64 measured requests per cold start. `COPY_REUSE=1`, MT1, GPU1 policy, ROM, cache plan, Graph, prefill, prefix, PLE and ordered combine stayed fixed.

| Arm | Decode p95 ms/token | TTFT p95 seconds | E2E p95 seconds |
| --- | ---: | ---: | ---: |
| B0 before | 26.860 | 1.0162 | 7.3123 |
| P0 VERIFY ON1 | 28.576 | 1.4000 | 7.8537 |
| P0 VERIFY OFF1 | 25.926 | 0.7748 | 6.9537 |
| P0 VERIFY ON2 | 28.609 | 1.3781 | 7.8726 |
| P0 VERIFY OFF2 | 25.949 | 0.7618 | 6.9772 |
| B0 after | 26.836 | 0.9873 | 7.2930 |

Both OFF starts pass the original +3% limit against both fresh B0 brackets. Every OFF/ON comparison improves p95 by 9.193–9.378%, with identical input/answer/cache distributions among the four P0 starts. This p95 is over each request's average decode ms/token, **not streaming inter-token latency**. The original older 30.825 and 27.969 failures remain historical failures.

## Hardware and software scope

- Two NVIDIA RTX 3090 24 GiB cards, layer-split CUDA SM86; the historical specification names two AMD EPYC 7663 CPUs and socket-local NUMA roles.
- Historical inventory: Ubuntu x86_64, Linux 7.0.0-34-generic, pre-existing NVIDIA driver 610.43.02. The driver's reported CUDA API compatibility is not a compiler/toolkit version.
- Model: Qwen3.8-Flash-Next GSQ-RCO IQ3_S; individual experts can have mixed formats.
- Test configuration: max context 262144; int8 KV, resident 32768; workers 47; groups 1; expert slots 9988/8688, prompt loans 2020/1832, chunk 8192, ring 96; MTP spec4/min-p0.5.
- MT1 means `STRATA_IQ_MT_MIN=1`, not a change to MTP. GPU1 is the study's `--pcie-frac 1` policy label, not a claim that every workload has no CPU work.
- The original dashboard remained active for the latest study. No new hardware/driver/clock/power change was part of this publication.
- Exact installed RAM, CPU affinity map, storage, power/PCIe-link capture and compiler flags are not re-exported in this package; no cross-machine performance claim follows.

[Reproduce the public CPU checks](REPRODUCE.md). Original production and authenticated API restoration passed; the previously authorized production was restored without promotion or rollback to an obsolete version. Publication did not run a model, compile a backend or change any service.
