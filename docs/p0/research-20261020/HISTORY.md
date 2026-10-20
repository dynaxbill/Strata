# P0 research history and retained failures

All dates are 2026. No row retroactively changes an earlier gate.

| Date/stage | Finding | Disposition |
| --- | --- | --- |
| 10-17/18 original implementation | Dual private anonymous socket ROM, immutable PLE owner, GPU cache filled before ROM registration; live ArenaExpertSource and persistent embedding covered | Opt-in experimental source at public 9f09b7c |
| 10-18 host/CPU checks | 288 sampled fixed CPU expert checks bitwise; two host audits with 49542 sampled addresses each, not a full PFN census | Bounded pass; raw bit21/negative-control contradiction retained |
| 10-18 isolated original performance | B0 26.823/26.731; P0 30.825 ms/token p95, +15.12% vs bracket mean | Original 3% gate FAIL; this older source did not have the later slot VERIFY factor |
| 10-19 strict later C2 | 125/144 outside fixed nine-vector B0 envelope, one new Top-1 | FAIL retained, no widening |
| 10-19 first causal C2 chain | T3/T4 changes same expert batch NT1/NT2; MT_MIN2 switches ggml/AVX2 at L15/E255 IQ4_NL down; first output maxabs 2.60770320892334e-8, then amplification | Actual trace and CPU witness reproduce both sides; not a ROM-byte error |
| 10-19/20 refusal/backend investigation | Adaptive residency changes CPU/GPU arithmetic; exact L0/E131 witness reproduces both outputs with equal weights/input | maxabs .0033283382654190063; not isolated to a single FMA or quantization step |
| 10-20 earlier MT1/GPU1 qualification | C2 0/144 outside that predeclared envelope; full functional C3 and historical C1 pass | This batch's bounded observations remain valid history |
| 10-20 earlier four-client benchmark | P0 27.969 vs B0 26.861/26.955; later inspection finds P0 VERIFY1, B0 no VERIFY | Original FAIL retained; tensor-capture OFF did not mean verifier OFF |
| 10-20 actual 512K | B0/P0 GPU1 each exact recall at three depths, usage prompt_tokens 524088 | Separate max_context524288/VERIFY-ON profile; not all-task quality or OFF qualification |
| 10-20 single-factor ON/OFF/ON/OFF | ON28.576/28.609; OFF25.926/25.949; B0 26.860/26.836 | Latest OFF C4 PASS; same work/config; verifies later diagnostic overhead only |
| 10-20 repeat C2 | Both ON/OFF 8/144 outside old envelope, 0 new Top-1; paired 144/144 exact | Latest C2 FAIL; all eight first-repeat Chinese prefixes; no relabelling as B0 variation |
| 10-20 C3 functional/target | Suite PASS, target head a89b710d... exact, but GEN vs old BGEN3 | Original refusal not reconstructed with identical native protocol/pre-state |
| 10-20 publication | Public source overlay, raw hashes, sanitized reports, all C4 timing rows, original fixed comparator and CPU witness code | Research archive only, no new model run/build/service change |

The original 30.825 ms/token failure cannot be retrospectively explained solely by a flag introduced later. The source overlay captures subsequent slot-copy reuse and diagnostics; this publication does not isolate the performance contribution of every intermediate change.

Historical calibration from five earlier B0 starts is a distinct experiment. It is not added to the later frozen nine-vector envelope and cannot turn its failures into passes. Interrupted/failed runs remain separate from valid measurements.

The original hypervisor fault records are retained privately. This work neither clears them nor establishes the cause of that fault.

## Later 2026-10-20 follow-up

The original first40-head match and Chinese8-row FAIL now have actual trace/strict micro/isolated E449 causality. A new memory-only spill candidate passes the same original fixed9 envelope and cleanC4 gate; whole-VM swap I/O still fails. See[the follow-up](FOLLOWUP.md). No old gate is rewritten and no production promotion occurs.
