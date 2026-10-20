// Actual L44/E449 witness: exact existing pool dispatch and exact captured PCIe GPU group shape.
#include "strata/kernels/cpu/expert_layout.hpp"
#include "strata/kernels/cpu/native_expert.hpp"
#include "strata/kernels/cpu/expert.hpp"
#include "strata/kernels/iq_kernels.hpp"
#include "ggml.h"
#include "ggml-cpu.h"
#include <cuda_runtime.h>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace cpu=strata::kernels::cpu;
using namespace strata::kernels;
#define CK(x) do{auto e=(x);if(e!=cudaSuccess)throw std::runtime_error(std::string(#x)+": "+cudaGetErrorString(e));}while(0)
void need(bool x,const std::string&s){if(!x)throw std::runtime_error(s);}
void save(const std::string&p,const void*x,size_t n){std::ofstream f(p,std::ios::binary);f.write((const char*)x,n);need(bool(f),"write "+p);}
template<class T>std::vector<T> load(const std::string&p,size_t n){std::ifstream f(p,std::ios::binary|std::ios::ate);need(bool(f)&&f.tellg()==(std::streamoff)(n*sizeof(T)),"size "+p);f.seekg(0);std::vector<T>x(n);f.read((char*)x.data(),n*sizeof(T));need(bool(f),"read "+p);return x;}
std::vector<float> cpu_run(const cpu::NativeFmt&f,const std::vector<uint8_t>&blob,const std::vector<float>&x,const std::string&out){
 std::vector<float> h(f.n_ff),y(f.n_embd);alignas(64)uint8_t a[cpu::kNativeActBytes]{},hq[cpu::kNativeHBytes]{};need(f.act_bytes<=sizeof a&&f.h_bytes<=sizeof hq,"quant workspace extent");
 cpu::native_quant_act(f,x.data(),a);const void*ap[]={a};float*hp[]={h.data()};cpu::native_gu_rows(f,blob.data(),ap,1,hp,0,(int)f.n_ff);float*op[]={y.data()};
 if(cpu::q2_native_kernels(f.d_type)){
  cpu::ActQ aq{};cpu::act_quant_any(h.data(),(int)f.n_ff,aq);const cpu::ActQ*qp[]={&aq};cpu::q2_rows_any(blob.data()+f.down_off,f.d_row,(int)(f.n_ff/64),qp,1,op,0,(int)f.n_embd);if(!out.empty())save(out+"-cpu-hiddenQ.bin",&aq,sizeof aq);
 }else{
  cpu::native_quant_h(f,h.data(),hq);const void*qp[]={hq};cpu::native_down_rows(f,blob.data(),qp,1,op,0,(int)f.n_embd);if(!out.empty())save(out+"-cpu-hiddenQ.bin",hq,f.h_bytes);
 }
 if(!out.empty()){save(out+"-cpu-activationQ.bin",a,f.act_bytes);save(out+"-cpu-hidden.f32",h.data(),h.size()*4);}for(float v:y)need(std::isfinite(v),"nonfinite CPU output");return y;
}
int main(int argc,char**argv){try{
 need(argc==8,"usage: pack gguf input-dir layer expert output-prefix cpu|both");const std::string mode=argv[7];need(mode=="cpu"||mode=="both","mode");const char*mt=std::getenv("STRATA_IQ_MT_MIN");need(mt&&std::string(mt)=="1","Must preserve trace MT_MIN1");std::string err;need(cpu::expert_layout_load(argv[1],48,512,err),err);const auto&lay=cpu::expert_layout();need(lay.native,"native layout required");const int l=std::stoi(argv[4]),ex=std::stoi(argv[5]);need(l==44&&ex==449,"wrong actual witness");const auto&f=lay.fmt[l];const std::string native=argv[2],dir=native.substr(0,native.find_last_of('/')+1),input=std::string(argv[3])+"/",out=argv[6];
 std::vector<uint8_t>blob(lay.blob_bytes(l));const size_t lens[]={f.up_off,f.down_off-f.up_off,f.bytes-f.down_off};size_t at=0;
 for(int part=0;part<3;++part){size_t i=l*3+part;std::string name=lay.gguf_file.empty()||lay.gguf_file[i].empty()?native:dir+lay.gguf_file[i];std::ifstream in(name,std::ios::binary);need(bool(in),"weight shard");in.seekg(lay.gguf_off[i]+(uint64_t)ex*lens[part]);in.read((char*)blob.data()+at,lens[part]);need(bool(in),"expert bytes");at+=lens[part];}need(at==blob.size(),"blob extent");save(out+"-weights.bin",blob.data(),blob.size());
 auto x=load<float>(input+"activation.f32",f.n_embd),expected_cpu=load<float>(input+"expected-cpu.f32",f.n_embd);for(float v:x)need(std::isfinite(v),"nonfinite input");auto c=cpu_run(f,blob,x,out),c2=cpu_run(f,blob,x,"");save(out+"-cpu.f32",c.data(),c.size()*4);bool cpu_repeat=!std::memcmp(c.data(),c2.data(),c.size()*4),cpu_exact=!std::memcmp(c.data(),expected_cpu.data(),c.size()*4);
 const auto*gu=ggml_get_type_traits_cpu((ggml_type)f.gu_type);const auto*down=ggml_get_type_traits_cpu((ggml_type)f.d_type);std::ostringstream base;base<<std::setprecision(17)<<"\"layer\":44,\"expert\":449,\"gu_type\":\""<<ggml_type_name((ggml_type)f.gu_type)<<"\",\"down_type\":\""<<ggml_type_name((ggml_type)f.d_type)<<"\",\"n_embd\":"<<f.n_embd<<",\"n_ff\":"<<f.n_ff<<",\"mt_min\":"<<cpu::native_gu_mt_min(f.gu_type)<<",\"cpu_activation_type\":\""<<ggml_type_name(gu->vec_dot_type)<<"\",\"cpu_hidden_type\":\""<<(cpu::q2_native_kernels(f.d_type)?"Strata_ActQ_32_FP32_scale_half_away":ggml_type_name(down->vec_dot_type))<<"\",\"gpu_activation_type\":\"q8_1\",\"cpu_repeat_bitwise\":"<<(cpu_repeat?"true":"false")<<",\"CPU_TRACE_BITWISE\":"<<(cpu_exact?"true":"false");
 std::string cpu_json="{"+base.str()+",\"GPU_used\":false}";save(out+"-cpu.json",cpu_json.data(),cpu_json.size());if(!cpu_repeat||!cpu_exact){std::cout<<cpu_json<<'\n';throw std::runtime_error("Exact CPU pool trace replay failed; no tolerance accepted");}if(mode=="cpu"){std::cout<<cpu_json<<'\n';return 0;}
 std::ifstream desc_file(input+"gpu-plan-descriptor.i32",std::ios::binary|std::ios::ate);need(bool(desc_file),"GPU descriptor");size_t count=(size_t)desc_file.tellg()/4;desc_file.close();auto desc=load<int32_t>(input+"gpu-plan-descriptor.i32",count);need(desc.size()>=9,"GPU descriptor header");int cap=desc[2],ngpu=desc[3],selected_group=desc[4],target=desc[5],gy=desc[6],rows=desc[7],selected_row=desc[8];need(desc[0]==l&&desc[1]==ex&&cap==rows*10&&rows>=1&&rows<=6&&ngpu>=1&&ngpu<=cap&&selected_group>=0&&selected_group<ngpu&&target>=0&&target<cap&&gy==4&&selected_row>=0&&selected_row<rows&&desc.size()==(size_t)(9+ngpu+1+2*cap),"GPU descriptor shape");const int32_t*starts=desc.data()+9,*dst=starts+ngpu+1,*tok=dst+cap;need(starts[selected_group+1]-starts[selected_group]==1,"Actual expert NT must remain1");need(dst[starts[selected_group]]==target&&tok[starts[selected_group]]==selected_row,"selected actual GPU entry");for(int i=0;i<ngpu;++i)need(0<=starts[i]&&starts[i]<=starts[i+1]&&starts[i+1]<=cap,"GPU group extent");
 auto gx=load<float>(input+"gpu-group-activations.f32",(size_t)rows*f.n_embd);need(!std::memcmp(gx.data()+(size_t)selected_row*f.n_embd,x.data(),x.size()*4),"GPU selected activation differs");auto expected_gpu=load<float>(input+"expected-gpu.f32",f.n_embd);
 CK(cudaSetDevice(0));cudaStream_t stream;CK(cudaStreamCreateWithFlags(&stream,cudaStreamNonBlocking));void*db,*dx,*dxq,*scratch;float*dy;unsigned long long*ptr;int32_t*gs,*gn,*gd,*gt;
 CK(cudaMalloc(&db,blob.size()));CK(cudaMalloc(&dx,gx.size()*4));CK(cudaMalloc(&dxq,(size_t)rows*(f.n_embd/32)*36));CK(cudaMalloc(&scratch,native_expert_scratch_bytes(cap,f.n_ff)));CK(cudaMalloc((void**)&dy,(size_t)cap*f.n_embd*4));CK(cudaMalloc((void**)&ptr,(size_t)ngpu*8));CK(cudaMalloc((void**)&gs,(size_t)(ngpu+1)*4));CK(cudaMalloc((void**)&gn,4));CK(cudaMalloc((void**)&gd,(size_t)cap*4));CK(cudaMalloc((void**)&gt,(size_t)cap*4));
 CK(cudaMemcpy(db,blob.data(),blob.size(),cudaMemcpyHostToDevice));std::vector<uint8_t>readback(blob.size());CK(cudaMemcpy(readback.data(),db,readback.size(),cudaMemcpyDeviceToHost));need(readback==blob,"GPU weight copy is not bitwise");CK(cudaMemcpy(dx,gx.data(),gx.size()*4,cudaMemcpyHostToDevice));std::vector<unsigned long long>ptrs(ngpu,(unsigned long long)db);CK(cudaMemcpy(ptr,ptrs.data(),ptrs.size()*8,cudaMemcpyHostToDevice));CK(cudaMemcpy(gs,starts,(ngpu+1)*4,cudaMemcpyHostToDevice));CK(cudaMemcpy(gn,&ngpu,4,cudaMemcpyHostToDevice));CK(cudaMemcpy(gd,dst,cap*4,cudaMemcpyHostToDevice));CK(cudaMemcpy(gt,tok,cap*4,cudaMemcpyHostToDevice));
 const auto L=native_expert_layout(f.gu_type,f.d_type,f.n_embd,f.n_ff);std::vector<float>g(f.n_embd),g2(f.n_embd);
 for(auto*v:{&g,&g2}){quantize_q8_1_rows((const float*)dx,rows,f.n_embd,dxq,stream);native_expert_grouped(L,ptr,gs,gn,gd,gt,cap,cap,dxq,scratch,dy,stream,gy);CK(cudaGetLastError());CK(cudaStreamSynchronize(stream));CK(cudaMemcpy(v->data(),dy+(size_t)target*f.n_embd,v->size()*4,cudaMemcpyDeviceToHost));}
 save(out+"-gpu.f32",g.data(),g.size()*4);std::vector<uint8_t>gq((size_t)rows*(f.n_embd/32)*36);CK(cudaMemcpy(gq.data(),dxq,gq.size(),cudaMemcpyDeviceToHost));save(out+"-gpu-activationQ.bin",gq.data(),gq.size());bool repeat=!std::memcmp(g.data(),g2.data(),g.size()*4),exact=!std::memcmp(g.data(),expected_gpu.data(),g.size()*4);double mx=0,sq=0;for(size_t i=0;i<g.size();++i){need(std::isfinite(g[i]),"nonfinite GPU output");double d=(double)c[i]-g[i];mx=std::max(mx,std::abs(d));sq+=d*d;}
 std::ostringstream report;report<<std::setprecision(17)<<"{"<<base.str()<<",\"GPU_used\":true,\"GPU_TRACE_BITWISE\":"<<(exact?"true":"false")<<",\"gpu_repeat_bitwise\":"<<(repeat?"true":"false")<<",\"gpu_weight_copy_bitwise\":true,\"GPU_cap\":"<<cap<<",\"GPU_input_rows\":"<<rows<<",\"GPU_target_row\":"<<selected_row<<",\"GPU_PCIE_group_count\":"<<ngpu<<",\"GPU_gy\":"<<gy<<",\"maxabs\":"<<mx<<",\"RMS\":"<<std::sqrt(sq/g.size())<<"}";std::string text=report.str();save(out+"-gpu.json",text.data(),text.size());std::cout<<text<<'\n';need(repeat&&exact,"Exact actual GPU trace replay failed; no tolerance accepted");for(void*p:{db,dx,dxq,scratch,(void*)dy,(void*)ptr,(void*)gs,(void*)gn,(void*)gd,(void*)gt})CK(cudaFree(p));CK(cudaStreamDestroy(stream));return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
