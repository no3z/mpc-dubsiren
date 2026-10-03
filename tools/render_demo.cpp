#include "siren.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
static void u16(FILE* f,unsigned v){std::fputc(v&255,f);std::fputc((v>>8)&255,f);}
static void u32(FILE* f,unsigned v){u16(f,v&65535);u16(f,v>>16);}
int main(int argc,char** argv){
    FILE* f=std::fopen(argc>1?argv[1]:"build/demo.wav","wb");if(!f)return 1;
    constexpr unsigned frames=44100*36,bytes=frames*4;
    std::fwrite("RIFF",1,4,f);u32(f,bytes+36);std::fwrite("WAVEfmt ",1,8,f);u32(f,16);u16(f,1);u16(f,2);u32(f,44100);u32(f,176400);u16(f,4);u16(f,16);std::fwrite("data",1,4,f);u32(f,bytes);
    dub::Siren s;float l[128],r[128];
    for(int preset=0;preset<12;++preset){
        s.set(dub::P_stop,1);s.set(dub::P_preset,float(preset));s.set(dub::P_latch,1);
        for(int offset=0;offset<44100*3;){
            if(offset>=44100*2)s.set(dub::P_latch,0);
            const int n=std::min(128,44100*3-offset);s.render(l,r,n);
            for(int i=0;i<n;++i){u16(f,unsigned(int16_t(std::lrint(l[i]*32767))));u16(f,unsigned(int16_t(std::lrint(r[i]*32767))));}offset+=n;
        }
    }
    return std::fclose(f)!=0;
}
