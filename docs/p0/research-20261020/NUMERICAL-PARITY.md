# Numerical acceptance and causal evidence

## C1: immutable ROM and fixed CPU kernels

ROM copies have zero tolerated byte difference. Fixed activation, expert ID, kernel path and arithmetic order require bitwise equal CPU outputs. Check memory, alignment, races and path identity before introducing a numerical tolerance. PFN sampling and read-only maps are separate evidence.

## C2: teacher-forced complete logits

Use fixed token histories and full 248320-value FP32 heads. Preserve the originally fixed per-prefix B0 vectors; no new vectors or nonzero tolerance may be added after seeing a failure. The published comparator is byte-identical to the archived original numerical comparator and checks empirical elementwise min/max plus Top-1 membership.

Initial diagnostic atol/rtol values from the design are not final release gates. Inspect B0 self-variation and report maximum absolute error, RMS, p99.9, margins, new Top-1 and genuine-vector matches. Text equality, top-k equality and functional success cannot replace C2.

For a fixed greedy decision, if every logit error is bounded by epsilon, margin > 2*epsilon is sufficient to preserve argmax at that position. It does not guarantee future untested autoregressive generations. Zero observed new Top-1 does not waive an envelope failure.

Latest ON/OFF each fails 8/144 against the same original nine-vector envelope, although their direct complete vectors are 144/144 bitwise identical. Earlier GPU1 0/144 and original default 125/144+1 are different recorded batches; all remain visible.

## First causal C2 difference already established

The earlier first control difference was request12 math:4, T4 vs T3; the next math:8 first complete head differs. Same T gives bitwise B0/P0 heads for the first13 requests. Bidirectional T intervention reproduces both archived heads.

At L15/E255, common committed row0 has equal valid activation/router/weights/backend. Extra rejected row3 changes CPU NT1 to NT2; MT_MIN2 switches the CPU path. The first output difference is 2.60770320892334e-8. Hidden quant bytes are equal; the propagating difference remains in IQ4_NL down. MT_MIN1 and MT_MIN99 each remove this witness's NT dependence while choosing different existing arithmetic results. This is not proof for every format or later Top-1.

The latest eight Chinese heads are a new unresolved observation. Earliest is request41 chinese:0 repeat0. Its first internal state/tensor difference was not captured here; neither the earlier L15/E255 explanation nor B0 nondeterminism is automatically assigned to it.

## C3: functionality and actual native protocol

Keep coding, mathematics, tools, structured JSON, history/session isolation and long-context checks separate from logits. Preserve actual token IDs, header and complete pre-state. The latest target has the same4129 IDs and head as historical B0, but is GEN128 rather than BGEN3/128. Functional PASS and zero head difference are diagnostic only for the original refusal's exact replay.

The L0/E131 exact witness separately establishes equal weights/activation and repeatable CPU/GPU output difference: CPU q8_K/q8_0 vs GPU q8_1, mixed iq3_xxs/iq4_nl, maxabs .0033283382654190063. This supports backend-path sensitivity without requiring ROM corruption, but does not isolate one quantization stage/FMA or reproduce the original bad head's entire state.
