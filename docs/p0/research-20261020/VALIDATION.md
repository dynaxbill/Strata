# Validation matrix

Latest disposition: **NOT_PROMOTED**. These are different gates; passing one does not waive another.

## Passed

| Check | Observed result and scope |
| --- | --- |
| P0 ROM bytes/read-only | Seven latest P0 loads: 24576 blobs bitwise identical, 24577 spans, no FULL fallback; physical host PFN evidence remains historical and sampled |
| Single-factor contract | P0 config/env/image/cache/harness fixed except VERIFY; COPY_REUSE stays 1 |
| Latest C4 p95 | OFF1 25.926, OFF2 25.949 ms/token; both pass +3% against B0 26.860/26.836 |
| Repeated verifier overhead | ON1 28.576, ON2 28.609; same work distributions; bounded diagnostic bootstrap interval -9.452% to -9.015%, not a replacement gate |
| ON/OFF direct C2 | 144/144 complete 248320-FP32 heads bitwise; maxabs/RMS 0; no changed Top-1 |
| C3 functional suite | Precondition, history20, API9, eight executable gcd cases and canonical input audit pass |
| Resources | 1135 latest samples: model-cgroup swap max 0, OOM/kill 0, no monitor errors; not a whole-VM zero-swap claim |
| Recovery | Authorized current production/API, immutable native/runtime identity, both GPU owners/lease, active original dashboard and boot continuity verified; no promotion |

## Failed or incomplete

| Gate | Result |
| --- | --- |
| Latest C2 ON vs original nine-vector B0 envelope | **FAIL: 8/144 outside; 0 new Top-1; 136 match genuine old B0 vectors** |
| Latest C2 OFF vs the same unchanged envelope | **FAIL: same 8/144; no new baseline vectors or tolerance** |
| Earlier default P0 C2 | **Unchanged FAIL: 125/144 outside and one new Top-1** |
| Earliest new latest head | Request 41, repeat0 chinese:0, after the same first 40 requests; env byte-identical and runtime config equal to earlier P0 ON |
| First internal cause of the latest eight rows | **NOT_CAPTURED**: pending/residency/GDN/KV/activation/backend witnesses absent in this performance study |
| Exact earlier C3 refusal replay | **NOT_SATISFIED**: latest native target is GEN128; historical B0 was BGEN slot3/128, despite identical input and full head |
| Historical C4 gates | 30.825 and later 27.969 ms/token remain separate failed configurations |
| Historical preparation/collection/observer failures | Retained, including trace-copy contract, omitted nested fixture, fingerprint schema, shape assertion and post-analysis header/parser mistakes |

The eight latest C2 rows are repeat0 chinese:0,1,2,4,8,16,31,57. ON/OFF exact equality rules out a new difference from the VERIFY toggle in the observed vectors, not a common P0 issue. No new B0 run has established these eight vectors as a B0 phenomenon.

## Not run for the latest OFF configuration

- Single-client C1, 512K, HIP, SYCL, long soak and full concurrent-C4 logit equivalence.
- Backend builds or GPU/model tests during this GitHub publication.
- A bit-identical rebuilt native binary from the archived source overlay.
- Production promotion, merge or upstream review request.

Earlier GPU1/VERIFY-ON qualification has bounded C1 and actual 512K recall passes. They are not transferred to the OFF configuration. A branch/file-format check is not a numerical or production acceptance gate.
