# Selected historical image source overlay

Base public P0 head: `9f09b7c3755f8048be812d484e7917bd03bd2c1a` (upstream v0.1.44.1 base `296b59d2f73c8f04a805df29c2fa97655adace78`).

Source image: `sha256:95601c2cc323d3e84a8b7cef03f594aa7c682b5ac9ff430d0eba161dba4a9f49`. This is a private locally built image digest, not a public pullable container.

The overlay contains11 selected changed/new files, including slot trace/logit headers, copy/reuse/checkpoint/KV diagnostic paths, ROM instrumentation and the historically used serving-wrapper repair. The wrapper dependency is also tracked separately in upstream PR2886; it is not mislabelled as a new P0 arithmetic fix.

```sh
git checkout --detach 9f09b7c3755f8048be812d484e7917bd03bd2c1a
git apply --check /path/to/tested-image-overlay.patch
git apply /path/to/tested-image-overlay.patch
```

Run only in a disposable research checkout. The preparer reconstructed every one of430 selected src/include/serve/CMake text file hashes in image-source-manifest.json. Other container content, binary reproducibility and backend portability are separate matters. Native root files of the publication branch are not overwritten by adding this archival patch.

This source capture used a network-disabled, read-only CPU Python entrypoint with NVIDIA_VISIBLE_DEVICES=void and no GPU device request. It did not load a model, compile, change any service or run a GPU kernel.

COPY_REUSE and VERIFY are separate switches. Latest C4 fixes REUSE1 and toggles VERIFY1/0. VERIFY1 additionally re-saves/compares checkpoints and reads KV in64KiB pieces, memcmp/hash and resident checks on the admission path. The normal copy/restore remains. VERIFY0 is a measured low-latency candidate here, not permission to waive C2/C3.

The two standalone witness C++ sources are historical exact tests. Their raw model weights/activation inputs are private; read REPRODUCE.md before treating them as runnable examples.
