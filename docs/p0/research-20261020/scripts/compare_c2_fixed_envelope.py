"""Predeclared nine-vector B0 self-envelope; never widen it after seeing P0."""
import argparse,hashlib,itertools,json,pathlib
import numpy as np
P=pathlib.Path
def load(phase):
 m=json.loads((phase/'c2-data/manifest.json').read_text());assert m['status']=='COLLECTED_FINITE_FULL_VOCAB'
 out={}
 for r in m['rows']:
  p=phase/'c2-data'/r['row'];assert hashlib.sha256(p.read_bytes()).hexdigest()==r['logits_sha256']
  x=np.fromfile(p,dtype='<f4');assert x.size==248320 and np.isfinite(x).all()
  out.setdefault(r['key'],[]).append((r,x))
 return m,out
def stats(x,y):
 d=np.abs(y.astype(np.float64)-x.astype(np.float64));i=int(np.argmax(x));top=np.partition(x,-2)[-2:];margin=float(top[-1])-float(top[-2]);eps=float(d.max())
 return {'maxabs':eps,'rms':float(np.sqrt(np.mean(d*d))),'p99_9':float(np.quantile(d,.999)),
  'baseline_top1':i,'candidate_top1':int(np.argmax(y)),'top1_changed':i!=int(np.argmax(y)),
  'baseline_margin':margin,'margin_gt_2epsilon':margin>2*eps,'bitwise':x.tobytes()==y.tobytes()}
p=argparse.ArgumentParser();p.add_argument('--root',type=P,required=True);p.add_argument('--output',type=P,required=True);a=p.parse_args()
baselines=[load(a.root/name) for name in ['61_C2_B0_GPU1_1','63_C2_B0_GPU1_2','64_C2_B0_GPU1_3']];pm,p0=load(a.root/'62_C2_P0_GPU1')
assert len({x[0]['fixture_sha256'] for x in baselines} | {pm['fixture_sha256']})==1
keys=set(p0);assert len(keys)==48 and all(set(b)==keys for _,b in baselines)
rows=[];self_rows=[]
for key in sorted(keys):
 b=[r for _,table in baselines for r in table[key]];c=p0[key];assert len(b)==9 and len(c)==3
 assert len({r['input_sha256'] for r,_ in b+c})==1
 unique={r['logits_sha256']:x for r,x in b};bs=list(unique.values());stack=np.stack([x for _,x in b]);lo=stack.min(axis=0);hi=stack.max(axis=0)
 witnesses=set(unique);winners={int(np.argmax(x)) for _,x in b}
 for x,y in itertools.combinations(bs,2):self_rows.append({'key':key,**stats(x,y)})
 for r,y in c:
  outside=(y<lo)|(y>hi);excess=np.maximum(lo.astype(np.float64)-y.astype(np.float64),y.astype(np.float64)-hi.astype(np.float64));excess=np.maximum(excess,0)
  near=min((stats(x,y) for x in bs),key=lambda d:d['maxabs'])
  rows.append({'key':key,'repeat':r['repeat'],'input_sha256':r['input_sha256'],
   'B0_unique_full_vectors':len(bs),'outside_envelope_elements':int(outside.sum()),'maximum_envelope_excess':float(excess.max()),
   'matches_genuine_B0_vector':r['logits_sha256'] in witnesses,'top1_seen_in_B0':int(np.argmax(y)) in winners,
   'versus_first_B0':stats(b[0][1],y),'nearest_genuine_B0':near})
summary={'status':'PASS_OBSERVED_C2_ENVELOPE' if all(r['outside_envelope_elements']==0 and r['top1_seen_in_B0'] for r in rows) else 'FAIL_OBSERVED_C2_ENVELOPE',
 'prefixes':48,'repeats_P0':3,'fresh_B0_starts':3,'B0_vectors_per_prefix':9,'vocabulary':248320,'candidate_rows':len(rows),
 'candidate_matches_genuine_B0':sum(r['matches_genuine_B0_vector'] for r in rows),
 'candidate_rows_outside_envelope':sum(r['outside_envelope_elements']>0 for r in rows),
 'candidate_rows_new_top1':sum(not r['top1_seen_in_B0'] for r in rows),
 'B0_self_maxabs':max((r['maxabs'] for r in self_rows),default=0),'P0_vs_first_maxabs':max(r['versus_first_B0']['maxabs'] for r in rows),
 'scope':'Bounded48 fixed native prefixes, sequential requests with C4 capacity. Elementwise empirical min/max over predeclared9B0 vectors, zero added tolerance. No claim of universal stability, concurrent full-head equality, C3 waiver or production promotion.',
 'rows':rows,'B0_self_comparisons':self_rows}
a.output.write_text(json.dumps(summary,indent=2));print(json.dumps({k:v for k,v in summary.items() if k not in ['rows','B0_self_comparisons']},indent=2))
