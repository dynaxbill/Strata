# P0 host backend runbook

Keep application changes and VM memory-backend changes as separate review units. Capture the original VM configuration, running QEMU command line, QMP memory backends, kernel/driver versions, CPU affinity, GPU passthrough map and application identities before any change. Treat backups, environment, API keys and physical addresses as private evidence.

For the tested PVE layout, persist `allow-ksm: 0` for the target VM only. Do not disable global KSM or change other VMs. This is not a live hotplug field: PVE can keep it pending until the VM stops. Inspect pending/current configuration separately. Drain application requests under the existing ownership locks, stop its owned GPU processes and verify the GPU lease is released, then request a graceful VM shutdown. Do not force-stop an unresponsive VM as an automatic fallback.

While stopped, inspect the generated cold-start command and require `mem-merge=off`. Start the VM and independently inspect running QMP `query-memdev`: each relevant RAM backend must report `merge=false`. A generated command alone is not proof of the running backend. Compare all unrelated VM configuration fields with the backup.

After restart, read the actual guest kernel, driver, GPU PCI identities and vCPU affinity again. A previously installed kernel can become active at reboot; keep subsequent B0/P0 comparisons in this same environment. Verify the original B0 image, binary hash, configuration, concurrency/context, API authentication and a fresh nonce request before proceeding.

If qualification fails after a new backend has started, restore the original scoped setting and perform a second controlled cold restart. Restoring only the configuration file does not restore the already-running backend. Preserve the failed receipts and verify application recovery again. Do not overwrite unrelated concurrent VM changes by blindly replacing an entire configuration file.

For host physical-page evidence, translate seeded samples from every expert and boundary through guest pagemap, QEMU gpa2hva and QEMU-process pagemap. Require nonzero visible PFNs, no swap, stable translations, zero identity intersection between mirrors, and physical host nodes in each socket's allowed set. Hash all live guest ROM spans separately and scan whole-range guest NUMA placement; sampling alone does not prove content coverage.

Corroborate RAM backend `merge=false` with whole RAM-VMA accounting: no mergeable VmFlags, KSM zero, no shared resident accounting, and all resident bytes anonymous/private. Record kpagecount with the kernel's THP averaged-count limitation. On the measured host kernel7.0.6-2-pve, raw bit21 was also set on fresh unique MADV_UNMERGEABLE negative controls with mapcount1; retain that raw contradiction and do not use it alone to infer deduplication. Record the sampling method, scale, visibility failures and limitations explicitly.

This runbook does not install kernels, alter drivers/P2P, change global NUMA/THP/KSM, clear file caches or promote an application candidate. The private report determines which gates passed and which remain UNVERIFIED.

