# Reproduce the public CPU checks

These steps read local files. They do not start a model, contact a service or alter production.

## Public archive verification and C4 re-computation

Use Python3.10+ from this directory:

```sh
python scripts/verify_publication.py
python scripts/recompute_c4.py
```

The first command checks each published file's SHA256 against the manifest. The second preserves the original nearest-rank p95, first-two-round exclusion and +3% gate against both B0 brackets. It checks all432 rows, measured output256/cache4089, four-client shape and P0 input/answer/cache distributions. It intentionally does not turn the latest C2 FAIL into a pass.

## Historical source reconstruction

In a separate research checkout at public P0 head `9f09b7c3755f8048be812d484e7917bd03bd2c1a`, apply the overlay described in source/README.md. Do not apply it to current upstream main or a production checkout.

The publication preparer ran git apply --check and reconstructed all430 selected source hashes of image `95601c2...`. No new native/backend build or model run was performed for publication. CUDA SM86 has historical image-level execution evidence; HIP/SYCL remain unbuilt/unrun for this candidate. Compiler/toolkit, external build assets, model data and the entire container are not reconstructed solely by a source patch.

## Full C2 comparison requires private raw data

The original comparator is scripts/compare_c2_fixed_envelope.py (NumPy2.3.5 was used historically). It expects the original three B0 directories named61_C2_B0_GPU1_1,63_C2_B0_GPU1_2,64_C2_B0_GPU1_3, each with three vectors per prefix, and a candidate directory alias62_C2_P0_GPU1. Use a separate read-only extraction/working copy for the candidate107/108 data; do not rewrite an archived manifest or add vectors to B0.

```sh
python scripts/compare_c2_fixed_envelope.py --root /path/to/private-working-copy --output comparison.json
```

Expected latest ON and OFF result: FAIL_OBSERVED_C2_ENVELOPE, 8/144 outside, 0 new Top-1. Direct ON/OFF pairing is144/144 bitwise. The public metadata lacks the full float arrays and cannot reproduce these numerical calculations alone.

The C2 expert255 and C3 expert131 C++ witness sources require the matching archived native math libraries, own licensed model pack/GGUF, and exact captured inputs. No private input arrays, prebuilt libraries or weights are distributed here. The source comments/CLI identify their input shapes; this is archival reproducibility material, not a portable newly tested build command.

## Scope of historical work

Do not replay a lone chinese:0 prompt as if it preserved the original first40 requests, or replace a BGEN slot request with GEN and call the pre-state identical. New causal runs need their own fixed plan, raw evidence and complete restoration contract. No such new run is authorized by executing the public offline scripts.
