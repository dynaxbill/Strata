# Private-image PCIe spill experiment (CUDA only)

This is a research artifact, not an engine-source change on this branch or a released feature. The root engine source remains the prior P0 implementation. Apply only to the exact later tested source snapshot described by the existing 11-file overlay and source hashes; do not assume that the public branch binary was rebuilt from it.

`STRATA_P0_PCIE_SPILL=1` opts into the mode2 experiment. Logical GPU plan capacity becomes min(64, plan capacity), while physical staging stays16 (8 per G2 group). The new transfer object copies/rebases only the physical prefix; overflow keeps its original pinned alias and uses the original GPU expert arithmetic. Never just raise the mode2 cap with the original unbounded fetch/rebase kernels.

The source patch changes only verifier memory scheduling and CMake, adding the transfer/test objects. Original93 math/kernel objects were checked bitwise unchanged for each private B0/P0 build. The two-GPU standalone test covers264 Graph/canary/alias checks. Same-profile C2 and clean C4 results are in the dated report; original resource/C2/C3 failures remain.

Untested: HIP/SYCL builds, a complete default-OFF model byte-equivalence run for this new prototype, final C1/512K/concurrent full-head/long-soak qualification and production. No upstream review requested. Model weights, private probe arrays, images and administrative controllers are not distributed here.
