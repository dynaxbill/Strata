"""Public arithmetic/data-integrity check, not a model or resource release gate."""
import collections,json,math,pathlib
R=pathlib.Path(__file__).resolve().parents[1];v=json.loads((R/'data/spill-c4-requests.json').read_text(encoding='utf-8'));expected=json.loads((R/'data/spill-c4.json').read_text(encoding='utf-8'))
def p95(x):return sorted(x)[math.ceil(.95*len(x))-1]
out={};work={}
for c in v['cases']:
 assert c['peak_running']==c['peak_decoding_slots']==4 and len(c['requests'])==72
 assert collections.Counter(q['round_index'] for q in c['requests'])==collections.Counter({i:4 for i in range(18)})
 for q in c['requests']:assert q['excluded_warmup']==(q['round_index']<2) and q['status']=='PASS'
 rr=[q for q in c['requests'] if not q['excluded_warmup']];assert len(rr)==64 and all(q['timings']['predicted_n']==256 for q in rr)
 vals=[q['timings']['predicted_per_token_ms'] for q in rr];assert all(math.isfinite(x) for x in vals);assert p95(vals)==c['original_result_p95_ms']==expected['cases'][c['label']]['p95_ms'];out[c['label']]=p95(vals);work[c['label']]=[collections.Counter(q[k] for q in rr) for k in ['request_sha256','answer_sha256']]+[collections.Counter(q['timings']['cache_n'] for q in rr)]
a,b,c=[out[k] for k in ['139_C4_B0_spill_before','140_C4_P0_spill','141_C4_B0_spill_after']];gate=b<=1.03*a and b<=1.03*c
assert gate==expected['P0_vs_matched_B0_gate']['point_3pct_gate'] and all(x==next(iter(work.values())) for x in work.values())
print(json.dumps({'status':'PUBLIC_SPILL_C4_RECOMPUTATION_PASS','p95_ms':out,'original_3pct_gate':gate,'resource_zero_swap':'FAIL','NOT_PROMOTED':True},indent=2))
