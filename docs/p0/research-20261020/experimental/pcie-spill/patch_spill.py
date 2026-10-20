"""Private opt-in source patch; no build, GPU, or acceptance side effect."""
import hashlib,json,pathlib,sys
def once(s,a,b):
 assert s.count(a)==1,(a,s.count(a));return s.replace(a,b,1)
def verify(s):
 s=once(s,'#include "strata/core/verify.hpp"','#include "strata/core/verify.hpp"\n#include "strata/core/p0_pcie_spill.hpp"')
 s=once(s,'    sink_.staging_cap = per;','    sink_.staging_cap = p0_pcie_spill_enabled() && sink_.pcie_mode == 2 ? std::min<int64_t>(64, sink_.cap) : per;')
 a='''                fetch_blobs(p_ptr2, p_counts + 2, stage, (int64_t) lay.blob_bytes(l), (int) per, cs);
                rebase_ptrs((unsigned long long*) p_ptr2, p_counts + 2, stage, (int64_t) lay.blob_bytes(l), cs);'''
 b='''                if (p0_pcie_spill_enabled()) {
                    p0_pcie_spill_fetch(p_ptr2, p_counts + 2, stage, (int64_t) lay.blob_bytes(l), (int) per, cs);
                    p0_pcie_spill_rebase((unsigned long long*) p_ptr2, p_counts + 2, stage, (int64_t) lay.blob_bytes(l), (int) per, cs);
                } else {
'''+a+'''
                }'''
 return once(s,a,b)
def cmake(s):
 a='  target_include_directories(strata_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)'
 b='''  if(NOT STRATA_ENABLE_CUDA OR STRATA_HIP_GFX906)
    message(FATAL_ERROR "The private P0 PCIe spill experiment is CUDA-only; HIP/SYCL qualification has not run")
  endif()
  target_sources(strata_engine PRIVATE src/core/p0_pcie_spill.cu)
  add_executable(p0_spill_copy_test tools/p0_spill_copy_test.cu src/core/p0_pcie_spill.cu)
  target_include_directories(p0_spill_copy_test PRIVATE include)
  target_link_libraries(p0_spill_copy_test PRIVATE CUDA::cudart)
'''
 return once(s,a,b+a)
if __name__=='__main__':
 root=pathlib.Path(sys.argv[1]);out={}
 for name,fn in [('src/core/verify.cpp',verify),('CMakeLists.txt',cmake)]:
  p=root/name;b=p.read_bytes();out[name]={'before':hashlib.sha256(b).hexdigest()};p.write_text(fn(b.decode()),encoding='utf-8',newline='\n');out[name]['after']=hashlib.sha256(p.read_bytes()).hexdigest()
 (root/'engine/P0_SPILL_PATCH.json').write_text(json.dumps(out,indent=2));print(json.dumps(out))
