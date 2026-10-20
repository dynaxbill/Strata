// Private CPU-only actual C2 witness. Calls unchanged archived native CPU functions.
#include "strata/kernels/cpu/expert_layout.hpp"
#include "strata/kernels/cpu/native_expert.hpp"
#include "strata/kernels/cpu/expert.hpp"
#include "ggml.h"
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
namespace cpu=strata::kernels::cpu;
void need(bool b,const std::string&s){if(!b)throw std::runtime_error(s);}
void save(const std::string&p,const void*x,size_t n){std::ofstream f(p,std::ios::binary);f.write((const char*)x,n);need(bool(f),"write "+p);}
std::vector<float> load(const std::string&p,size_t n){std::ifstream f(p,std::ios::binary|std::ios::ate);need(bool(f)&&f.tellg()==(std::streamoff)(n*4),"input size "+p);f.seekg(0);std::vector<float>x(n);f.read((char*)x.data(),n*4);need(bool(f),"read "+p);for(float v:x)need(std::isfinite(v),"nonfinite activation");return x;}
void run(const cpu::NativeFmt&f,const std::vector<uint8_t>&weights,const std::vector<float>&x0,const std::vector<float>&x1,int nt,const std::string&prefix){
 struct alignas(64) Quant {uint8_t a[cpu::kNativeActBytes]{},hq[cpu::kNativeHBytes]{};};Quant q[2];
 std::vector<float> h[2]={std::vector<float>(f.n_ff),std::vector<float>(f.n_ff)},out[2]={std::vector<float>(f.n_embd),std::vector<float>(f.n_embd)};
 const void*ap[2]={q[0].a,q[1].a};float*hp[2]={h[0].data(),h[1].data()};float*op[2]={out[0].data(),out[1].data()};
 cpu::native_quant_act(f,x0.data(),q[0].a);if(nt==2)cpu::native_quant_act(f,x1.data(),q[1].a);
 cpu::native_gu_rows(f,weights.data(),ap,nt,hp,0,(int)f.n_ff);
 if(cpu::q2_native_kernels(f.d_type)){
  cpu::ActQ act[2]{};const cpu::ActQ*aq[2]={&act[0],&act[1]};
  for(int t=0;t<nt;++t){cpu::act_quant_any(h[t].data(),(int)f.n_ff,act[t]);save(prefix+"-row"+std::to_string(t)+"-hiddenQ.bin",&act[t],sizeof(act[t]));}
  cpu::q2_rows_any(weights.data()+f.down_off,f.d_row,(int)(f.n_ff/64),aq,nt,op,0,(int)f.n_embd);
 }else{
  const void*hq[2]={q[0].hq,q[1].hq};for(int t=0;t<nt;++t){cpu::native_quant_h(f,h[t].data(),q[t].hq);save(prefix+"-row"+std::to_string(t)+"-hiddenQ.bin",q[t].hq,f.h_bytes);}
  cpu::native_down_rows(f,weights.data(),hq,nt,op,0,(int)f.n_embd);
 }
 for(int t=0;t<nt;++t){for(float v:out[t])need(std::isfinite(v),"nonfinite output");std::string p=prefix+"-row"+std::to_string(t);save(p+"-activationQ.bin",q[t].a,f.act_bytes);save(p+"-hidden.f32",h[t].data(),h[t].size()*4);save(p+"-output.f32",out[t].data(),out[t].size()*4);}
}
int main(int argc,char**argv){try{
 need(argc==6,"usage: pack gguf x0 extra output-prefix");std::string err;need(cpu::expert_layout_load(argv[1],48,512,err),err);const auto&lay=cpu::expert_layout();need(lay.native,"native weights required");const int layer=15,expert=255;const auto&f=lay.fmt[layer];
 std::string native=argv[2],dir=native.substr(0,native.find_last_of('/')+1),prefix=argv[5];std::vector<uint8_t>weights(lay.blob_bytes(layer));const size_t lens[]={f.up_off,f.down_off-f.up_off,f.bytes-f.down_off};size_t at=0;
 for(int part=0;part<3;++part){size_t i=layer*3+part;std::string name=lay.gguf_file.empty()||lay.gguf_file[i].empty()?native:dir+lay.gguf_file[i];std::ifstream in(name,std::ios::binary);need(bool(in),"weight shard");in.seekg(lay.gguf_off[i]+(uint64_t)expert*lens[part]);in.read((char*)weights.data()+at,lens[part]);need(bool(in),"expert weight read");at+=lens[part];}need(at==weights.size(),"weight extent");
 auto x0=load(argv[3],f.n_embd),extra=load(argv[4],f.n_embd);save(prefix+"-weights.bin",weights.data(),weights.size());
 run(f,weights,x0,extra,1,prefix+"-nt1");run(f,weights,x0,extra,2,prefix+"-nt2");run(f,weights,x0,extra,2,prefix+"-nt2-repeat");run(f,weights,x0,extra,1,prefix+"-nt1-repeat");run(f,weights,x0,x0,2,prefix+"-nt2-duplicate");
 std::cout<<"{\"layer\":15,\"expert\":255,\"gu_type\":\""<<ggml_type_name((ggml_type)f.gu_type)<<"\",\"down_type\":\""<<ggml_type_name((ggml_type)f.d_type)<<"\",\"mt_min\":"<<cpu::native_gu_mt_min(f.gu_type)<<",\"cpu_avx2\":"<<(cpu::cpu_avx2_ok()?"true":"false")<<",\"cpu_avx512\":"<<(cpu::cpu_avx512_ok()?"true":"false")<<",\"GPU_used\":false,\"n_embd\":"<<f.n_embd<<",\"n_ff\":"<<f.n_ff<<"}\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
