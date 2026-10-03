#include "siren.h"
#include "params.h"
#include <cstdio>
int main(){
    dub::Siren s;
    std::puts("{\"format\":\"DubForceSiren-1\",\"presets\":[");
    for(int preset=0;preset<12;++preset){
        s.set(dub::P_preset,float(preset));
        std::printf("%s{\"name\":\"%s\",\"values\":{",preset ? ",\n" : "",PARAMS[dub::P_preset].opts[preset]);
        for(int i=0;i<dub::P_Count;++i)std::printf("%s\"%s\":%.9g",i ? "," : "",PARAMS[i].key,s.get(i));
        std::printf("}}");
    }
    std::puts("\n]}");
}
