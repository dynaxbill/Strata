"""Offline public manifest verification only; never a model/quality acceptance gate."""
import hashlib,json,pathlib
R=pathlib.Path(__file__).resolve().parents[1];m=json.loads((R/'PUBLICATION-MANIFEST.json').read_text(encoding='utf-8'))
for item in m['files']:
 n=pathlib.PurePosixPath(item['path']);assert not n.is_absolute() and '..' not in n.parts
 p=R/n;assert p.is_file() and not p.is_symlink();b=p.read_bytes();assert len(b)==item['bytes'] and hashlib.sha256(b).hexdigest()==item['sha256'],str(n)
assert m['historical_numerical_status']=='FAIL_8_OF_144_EACH_ON_OFF' and m['release_status']=='NOT_PROMOTED'
assert m['numerical_status']=='PASS_CURRENT_SPILL_ORIGINAL_FIXED9_HISTORICAL_FAILS_RETAINED'
c2=json.loads((R/'data/spill-c2.json').read_text(encoding='utf-8'));assert c2['fixed_B0_vectors']==9 and c2['extra_tolerance']==0
for label in ['136_C2_B0_spill_ON','137_C2_P0_spill_ON','138_C2_P0_spill_POOL_observer']:
 row=c2['results'][label];assert row['candidate_rows_outside_envelope']==0 and row['candidate_rows_new_top1']==0 and row['candidate_matches_genuine_B0']==144
assert all(row['full_heads']==row['bitwise']==144 and row['maxabs']==0 for row in c2['pairs'].values())
resource=json.loads((R/'data/spill-resources.json').read_text(encoding='utf-8'));assert any(resource['whole_VM_swap_IO_monitored_span'].values()),'Preserve recorded zero-swap FAIL'
assert json.loads((R/'data/e449-causal.json').read_text(encoding='utf-8'))['entire_target_head_causal_reproduction']

print(json.dumps({'status':'PUBLICATION_FILES_SHA256_VERIFIED','files':len(m['files']),'release_status':m['release_status'],'source_image_selected_files':430}))
