# P0 FULL implementation and qualification

This opt-in Linux candidate mirrors every persistent host weight in the audited model scope. It allocates two private anonymous images, binds and fills each on its socket, verifies SHA256, locks and registers the images, protects them read-only, and checks all guest pages before publishing pointers. CPU jobs choose their worker socket's mirror; GPU cache routing and arithmetic retain their existing order. The persistent mapped embedding shares the ROM owner. Runtime reload and COMPLEMENT are rejected.

`STRATA_DS_ROM=full` requires `STRATA_NUMA_STRICT=1`, `STRATA_PLE_OWNER=single`, a writable `STRATA_DS_ROM_REPORT`, and externally verified `STRATA_DS_ROM_LAYER_SOCKET` roles for every layer. Defaults retain the original source. A failed candidate initialization reports `P0 FULL FALLBACK` and retains the old source; the qualification harness rejects such a fallback as a successful FULL run.

The immutable PLE table has one owner and independent request tickets. Session history and output buffers remain per session. This does not change the default PLE I/O mode. Direct, mmap, and locked RAM require separate capacity and parity measurements.

On the measured RTX3090 driver, read-only host registration is unsupported. The tested order is copy/hash/lock, register unpublished writable pages, `mprotect(PROT_READ)`, whole-range placement verification, then READY. Registration runs with temporary socket CPU affinity and memory policy, restoring both afterward. This order is required: registering the large images without both local controls moved pages between guest NUMA nodes.

A guest READY report does not establish physical mirror distinctness. Qualification additionally translates seeded samples from every expert through guest pagemap, QEMU gpa2hva and host pagemap; checks distinct stable PFNs and socket-local host nodes; and corroborates private RAM VMA accounting and backend `merge=false`. Host raw bit21 disagrees with fresh never-mergeable negative controls on the measured PVE kernel. It is retained as an interface limitation, rather than treated alone as evidence of deduplication. THP can average kpagecount values, so mapcount is corroboration only.

Shutdown drains device work and unregisters both images before releasing mappings. A failed drain retains mappings and reports HOLD. Candidate container qualification uses init and a bounded 180-second Docker stop timeout; final restoration uses the captured B0 launcher.

Status and measured results belong in the accompanying private P0 report. This implementation is not promoted: unchanged B0 repeats currently fail the strict same-seed greedy identity control. CUDA SM86 and bounded CPU components have been exercised; HIP and SYCL have not been built. A draft must not be presented as ready for review or production.
