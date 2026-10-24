# Foundation: current v0.2.0-beta + scheme A

This private-fork integration reference preserves the official native source and scheme A from PR #2886 (child iterator cleanup and caller-owned DONE statistics). The official selected-replica callback remains. The native serving fix is separate from GPU numerical kernels.

## Scope

The foundation references upstream3d1fb48090e8f2cefc22d7b5845fb7f5b71f33fa and the deployed immutable image8a42a4011ff601db842265c17ac9b6c41aceace17a4a4973de681e1e3542d533 (native0.1.45.3). The candidate references upstream7ddc17a38fe7f31637e895ecfba58c2cd43d65ec (native0.1.48). Scheme A replicas.py SHA256 is6484e67ab1ed65a30b115645941b33a9f3845f98b1705ddad55629d130449696. These references do not move fork main or the existing upstream PR.

## Deployment policy

Replicas: two independent one-GPU engines, two native slots each; workers27 per engine, context262144, original RT MTP/batch-MTP, KVint8/resident32768, batch-groups1. Explicit conversation cache16384MiB/4entries per engine stays. Preserve sampling and fit_max_tokens:false. The foundation remains visionoff; CPU vision is the candidate target only. No model weights, keys, private runtime/host files or raw user conversations are in this reference.

## Validation

Passed: existing foundation hardware qualification is historical and identified by its original image; current new-release validation is recorded separately after actual commands.

Failed: none asserted here; see the timestamped preparation report for retained actual failures.

Not run: candidate GPU/model/performance/promotion at creation. CPU tests do not imply GPU acceptance. DFlash2 serving is gated off by upstream; an MTP fallback is never labelled DFlash2. No upstream-native code modification, other-backend qualification or universal numerical parity is claimed.

## Rollback

Use the immutable foundation source tag and the separately retained deployment image/config backup. First acquire exclusive maintenance control and drain the serving endpoint, then restore the matching image, launcher, run config and environment as one generation. Verify hashes, loaded262144/four native slots, nonce, MTP and authenticated API before returning traffic. Do not restore a key from this repository: credentials remain in private deployment storage. Do not delete the foundation image while this rollback is required.
