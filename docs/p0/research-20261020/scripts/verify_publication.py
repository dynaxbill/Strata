"""Offline public manifest verification only; never a model/quality acceptance gate."""
import hashlib,json,pathlib
R=pathlib.Path(__file__).resolve().parents[1];m=json.loads((R/'PUBLICATION-MANIFEST.json').read_text(encoding='utf-8'))
for item in m['files']:
 n=pathlib.PurePosixPath(item['path']);assert not n.is_absolute() and '..' not in n.parts
 p=R/n;assert p.is_file() and not p.is_symlink();b=p.read_bytes();assert len(b)==item['bytes'] and hashlib.sha256(b).hexdigest()==item['sha256'],str(n)
assert m['numerical_status']=='FAIL_8_OF_144_EACH_ON_OFF' and m['release_status']=='NOT_PROMOTED'
print(json.dumps({'status':'PUBLICATION_FILES_SHA256_VERIFIED','files':len(m['files']),'release_status':m['release_status'],'source_image_selected_files':430}))
