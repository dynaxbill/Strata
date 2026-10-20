"""Offline re-computation of all public C4 measurements; original gate unchanged."""
import collections,json,math,pathlib
R=pathlib.Path(__file__).resolve().parents[1]
v=json.loads((R/'data/c4-requests.json').read_text(encoding='utf-8'))
def p95(xs):return sorted(xs)[math.ceil(.95*len(xs))-1]
assert v['rounds']==18 and v['exclude_first_rounds']==2 and v['concurrency']==4 and v['original_limit_ratio']==1.03
rows={};work={}
for case in v['cases']:
 assert case['peak_running']==case['peak_decoding_slots']==4 and len(case['requests'])==72
 assert collections.Counter(q['round_index'] for q in case['requests'])==collections.Counter({i:4 for i in range(18)})
 for q in case['requests']:
  assert q['excluded_warmup']==(q['round_index']<2) and q['status']=='PASS'
  assert all(math.isfinite(q[k]) for k in ['elapsed_s','ttft_e2e_s'])
 measured=[q for q in case['requests'] if not q['excluded_warmup']];assert len(measured)==64
 assert all(q['timings']['predicted_n']==256 and q['timings']['cache_n']==4089 for q in measured)
 vals=[q['timings']['predicted_per_token_ms'] for q in measured];assert all(math.isfinite(x) and x>0 for x in vals)
 p=p95(vals);assert p==case['original_result_p95_ms']
 rows[case['label']]={'p95_ms':p,'TTFT_p95_s':p95([q['ttft_e2e_s'] for q in measured]),'E2E_p95_s':p95([q['elapsed_s'] for q in measured]),'N':64}
 work[case['label']]=[collections.Counter(q[k] for q in measured) for k in ['request_sha256','answer_sha256']]
ons=['102_C4_P0_VERIFY_ON1','104_C4_P0_VERIFY_ON2'];offs=['103_C4_P0_VERIFY_OFF1','105_C4_P0_VERIFY_OFF2'];b0s=['101_C4_B0_before','106_C4_B0_after']
assert all(work[n]==work[ons[0]] for n in ons+offs)
gates={n:{b:rows[n]['p95_ms']<=1.03*rows[b]['p95_ms'] for b in b0s} for n in offs}
assert all(all(x.values()) for x in gates.values())
c2=json.loads((R/'data/c2-singlefactor.json').read_text(encoding='utf-8'));assert c2['paired_bitwise']==c2['paired_total']==144 and c2['paired_maxabs']==0
assert all(c2['results'][level]['status']=='FAIL_OBSERVED_C2_ENVELOPE' and c2['results'][level]['candidate_rows_outside_envelope']==8 for level in ['ON','OFF'])
print(json.dumps({'status':'PUBLIC_C4_REQUEST_RECOMPUTATION_PASS','arms':rows,'original_3pct_OFF_gates':gates,'C2_stays_FAIL_each_8_of_144':True,'NOT_PROMOTED':True},indent=2))
