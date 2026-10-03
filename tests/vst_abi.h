#pragma once
#include <cstdint>
struct AEffect;
using AudioMaster = intptr_t (*)(AEffect*,int32_t,int32_t,intptr_t,void*,float);
struct AEffect {
    int32_t magic;
    intptr_t (*dispatcher)(AEffect*,int32_t,int32_t,intptr_t,void*,float);
    void (*process)(AEffect*,float**,float**,int32_t);
    void (*setParameter)(AEffect*,int32_t,float);
    float (*getParameter)(AEffect*,int32_t);
    int32_t numPrograms,numParams,numInputs,numOutputs,flags;
    intptr_t resvd1,resvd2;
    int32_t initialDelay,realQualities,offQualities;
    float ioRatio;
    void* object;
    void* user;
    int32_t uniqueID,version;
    void (*processReplacing)(AEffect*,float**,float**,int32_t);
    void (*processDoubleReplacing)(AEffect*,double**,double**,int32_t);
    char future[56];
};
struct MidiEvent {
    int32_t type=1,byteSize=32,deltaFrames=0,flags=0,noteLength=0,noteOffset=0;
    uint8_t midiData[4]={0,0,0,0};
    char detune=0,noteOffVelocity=0,reserved1=0,reserved2=0;
};
struct Events {int32_t numEvents=1; intptr_t reserved=0; void* events[2];};
extern "C" AEffect* VSTPluginMain(AudioMaster);
