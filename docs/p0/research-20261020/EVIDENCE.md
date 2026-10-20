# Evidence, provenance and public data

## Public material

- data/c4-requests.json: all18 rounds of six arms, 432 request rows; warmup rows explicitly marked rather than removed. Timings in ms/token or seconds as named; fingerprints are synthetic request/answer SHA256.
- data/c4-analysis.json: original bounded repeated-start analysis and diagnostic bootstrap.
- data/c2-ON-fixed-envelope.json, data/c2-OFF-fixed-envelope.json and data/c2-singlefactor.json: complete comparison metadata and ON/OFF pairing, with original numeric formulas and fixed envelope unchanged.
- data/c2-prior-head-firstdiff.json: first observable latest head change, not an internal tensor root cause.
- data/c3-target.json: actual GEN/BGEN headers, input/head hashes and explicit NOT_SATISFIED protocol replay.
- data/e131-exact-witness.json and source/*witness.cpp: actual independent witness results and source. Weight/activation input arrays are not included.
- source/tested-image-overlay.patch and source/image-source-manifest.json: exact selected historical image source delta and 430 file hashes.
- reports/: sanitized historical narratives, including failures. Original private receipt links are rendered as private references, not broken public download claims.
- data/evidence-ledger.json: private archive SHA256, counts and recorded independent-verification receipt status.
- data/provenance.json and PUBLICATION-MANIFEST.json: pinned sources and public bytes.

## Private raw evidence

The latest raw archive has 2101 files, 313488333 bytes, SHA256:

`fe4009ea788265abeb3523f064a0571a22269584bd17bfda1454058b87ab7495`

Its ZIP SHA, every member CRC/size/SHA and path/duplicate checks passed. The raw archive is retained privately. The local post-analysis sidecar has separate SHA `55d7b20998d955a92bd5cb091a6cbcdd969f966a498017330cbc992a10db8bfc`; it includes later failed analyzer attempts and corrections and is not represented as part of the older raw ZIP.

Other archive digests/counts are in the ledger. Published fingerprints do not let an external reader independently recompute full-logit numerical gates without the private vectors. The C4 timing gate is independently recomputable from the public request rows. Public summaries distinguish this limitation from raw CRC/SHA verification.

Not uploaded: model/GGUF/expert weights, activation dumps, full raw logits, full environments/runtime backups, API/SSH credentials, private addresses/home paths, boot identities, virtual addresses/PFNs and private service-control scripts. Sanitized reports preserve numeric findings and failed gates. GitHub owner names and already public upstream links remain.

Publication performs no new GPU experiment, backend build, production change or hardware/driver action. Historical source/build/test evidence is not CI for the new documentation commit.
