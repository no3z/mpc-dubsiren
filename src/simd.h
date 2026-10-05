#pragma once
// Four-lane float SIMD layer. NEON intrinsics on ARM (the Force target),
// SSE2 on x86 (desktop tests and benchmarks), plain loops otherwise.
// Masks are all-ones/all-zero lanes; select(m,a,b) is m ? a : b per lane.
// NaN handling differs between NEON and SSE min/max: callers that may see
// NaN clear it explicitly with finite() before clamping.
#include <cstdint>
#include <cstring>
// -DDUB_SIMD_SCALAR forces the portable loops (equivalence checks).
#if defined(DUB_SIMD_SCALAR)
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#define DUB_SIMD_NEON 1
#elif defined(__SSE2__)
#include <emmintrin.h>
#define DUB_SIMD_SSE 1
#endif

namespace dub::simd {
#if defined(DUB_SIMD_NEON)
using vf=float32x4_t; using vm=uint32x4_t; using vi=int32x4_t; using vu=uint32x4_t;
inline vf load(const float* p) noexcept{return vld1q_f32(p);}
inline void store(float* p,vf v) noexcept{vst1q_f32(p,v);}
inline vf splat(float x) noexcept{return vdupq_n_f32(x);}
inline vf add(vf a,vf b) noexcept{return vaddq_f32(a,b);}
inline vf sub(vf a,vf b) noexcept{return vsubq_f32(a,b);}
inline vf mul(vf a,vf b) noexcept{return vmulq_f32(a,b);}
// a+b*c
inline vf madd(vf a,vf b,vf c) noexcept{
#if defined(__ARM_FEATURE_FMA)
    return vfmaq_f32(a,b,c);
#else
    return vmlaq_f32(a,b,c);
#endif
}
inline vf min(vf a,vf b) noexcept{return vminq_f32(a,b);}
inline vf max(vf a,vf b) noexcept{return vmaxq_f32(a,b);}
inline vf abs(vf a) noexcept{return vabsq_f32(a);}
inline vm lt(vf a,vf b) noexcept{return vcltq_f32(a,b);}
inline vm gt(vf a,vf b) noexcept{return vcgtq_f32(a,b);}
inline vm ge(vf a,vf b) noexcept{return vcgeq_f32(a,b);}
inline vm eq(vf a,vf b) noexcept{return vceqq_f32(a,b);}
inline vm mand(vm a,vm b) noexcept{return vandq_u32(a,b);}
inline vf select(vm m,vf a,vf b) noexcept{return vbslq_f32(m,a,b);}
inline bool any(vm m) noexcept{const uint32x2_t r=vorr_u32(vget_low_u32(m),vget_high_u32(m));return (vget_lane_u32(r,0)|vget_lane_u32(r,1))!=0;}
inline float hmax(vf v) noexcept{float32x2_t r=vpmax_f32(vget_low_f32(v),vget_high_f32(v));r=vpmax_f32(r,r);return vget_lane_f32(r,0);}
// Estimate plus two Newton steps: ~24-bit reciprocal. 1/0 stays +inf.
inline vf recip(vf a) noexcept{vf r=vrecpeq_f32(a);r=vmulq_f32(vrecpsq_f32(a,r),r);return vmulq_f32(vrecpsq_f32(a,r),r);}
inline vi bits(vf x) noexcept{return vreinterpretq_s32_f32(x);}
inline vf from_bits(vi x) noexcept{return vreinterpretq_f32_s32(x);}
inline vi truncate(vf x) noexcept{return vcvtq_s32_f32(x);}
inline vf to_float(vi x) noexcept{return vcvtq_f32_s32(x);}
inline vf to_float(vu x) noexcept{return vcvtq_f32_u32(x);}
inline vi iadd(vi a,vi b) noexcept{return vaddq_s32(a,b);}
inline vi isplat(int32_t x) noexcept{return vdupq_n_s32(x);}
inline vi iand(vi a,vi b) noexcept{return vandq_s32(a,b);}
inline vi ior(vi a,vi b) noexcept{return vorrq_s32(a,b);}
template<int N> inline vi shl(vi a) noexcept{return vshlq_n_s32(a,N);}
template<int N> inline vi sra(vi a) noexcept{return vshrq_n_s32(a,N);}
inline vu uload(const uint32_t* p) noexcept{return vld1q_u32(p);}
inline void ustore(uint32_t* p,vu v) noexcept{vst1q_u32(p,v);}
inline vu uxor(vu a,vu b) noexcept{return veorq_u32(a,b);}
template<int N> inline vu ushl(vu a) noexcept{return vshlq_n_u32(a,N);}
template<int N> inline vu ushr(vu a) noexcept{return vshrq_n_u32(a,N);}
// Interleaved stereo int16 from values already scaled and rounded to int32.
inline void store_stereo_s16(int16_t* out,vi l,vi r) noexcept{
    int16x4x2_t v;v.val[0]=vqmovn_s32(l);v.val[1]=vqmovn_s32(r);vst2_s16(out,v);
}
#elif defined(DUB_SIMD_SSE)
using vf=__m128; using vm=__m128; using vi=__m128i; using vu=__m128i;
inline vf load(const float* p) noexcept{return _mm_loadu_ps(p);}
inline void store(float* p,vf v) noexcept{_mm_storeu_ps(p,v);}
inline vf splat(float x) noexcept{return _mm_set1_ps(x);}
inline vf add(vf a,vf b) noexcept{return _mm_add_ps(a,b);}
inline vf sub(vf a,vf b) noexcept{return _mm_sub_ps(a,b);}
inline vf mul(vf a,vf b) noexcept{return _mm_mul_ps(a,b);}
inline vf madd(vf a,vf b,vf c) noexcept{return _mm_add_ps(a,_mm_mul_ps(b,c));}
inline vf min(vf a,vf b) noexcept{return _mm_min_ps(a,b);}
inline vf max(vf a,vf b) noexcept{return _mm_max_ps(a,b);}
inline vf abs(vf a) noexcept{return _mm_andnot_ps(_mm_set1_ps(-0.f),a);}
inline vm lt(vf a,vf b) noexcept{return _mm_cmplt_ps(a,b);}
inline vm gt(vf a,vf b) noexcept{return _mm_cmpgt_ps(a,b);}
inline vm ge(vf a,vf b) noexcept{return _mm_cmpge_ps(a,b);}
inline vm eq(vf a,vf b) noexcept{return _mm_cmpeq_ps(a,b);}
inline vm mand(vm a,vm b) noexcept{return _mm_and_ps(a,b);}
inline vf select(vm m,vf a,vf b) noexcept{return _mm_or_ps(_mm_and_ps(m,a),_mm_andnot_ps(m,b));}
inline bool any(vm m) noexcept{return _mm_movemask_ps(m)!=0;}
inline float hmax(vf v) noexcept{v=_mm_max_ps(v,_mm_shuffle_ps(v,v,_MM_SHUFFLE(2,3,0,1)));v=_mm_max_ps(v,_mm_shuffle_ps(v,v,_MM_SHUFFLE(1,0,3,2)));return _mm_cvtss_f32(v);}
inline vf recip(vf a) noexcept{return _mm_div_ps(_mm_set1_ps(1.f),a);}
inline vi bits(vf x) noexcept{return _mm_castps_si128(x);}
inline vf from_bits(vi x) noexcept{return _mm_castsi128_ps(x);}
inline vi truncate(vf x) noexcept{return _mm_cvttps_epi32(x);}
inline vf to_float(vi x) noexcept{return _mm_cvtepi32_ps(x);}
inline vi iadd(vi a,vi b) noexcept{return _mm_add_epi32(a,b);}
inline vi isplat(int32_t x) noexcept{return _mm_set1_epi32(x);}
inline vi iand(vi a,vi b) noexcept{return _mm_and_si128(a,b);}
inline vi ior(vi a,vi b) noexcept{return _mm_or_si128(a,b);}
template<int N> inline vi shl(vi a) noexcept{return _mm_slli_epi32(a,N);}
template<int N> inline vi sra(vi a) noexcept{return _mm_srai_epi32(a,N);}
inline vu uload(const uint32_t* p) noexcept{return _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));}
inline void ustore(uint32_t* p,vu v) noexcept{_mm_storeu_si128(reinterpret_cast<__m128i*>(p),v);}
inline vu uxor(vu a,vu b) noexcept{return _mm_xor_si128(a,b);}
template<int N> inline vu ushl(vu a) noexcept{return _mm_slli_epi32(a,N);}
template<int N> inline vu ushr(vu a) noexcept{return _mm_srli_epi32(a,N);}
inline void store_stereo_s16(int16_t* out,vi l,vi r) noexcept{
    const __m128i lp=_mm_packs_epi32(l,l),rp=_mm_packs_epi32(r,r);
    _mm_storeu_si128(reinterpret_cast<__m128i*>(out),_mm_unpacklo_epi16(lp,rp));
}
#else
struct vf{float v[4];}; struct vm{uint32_t v[4];}; struct vi{int32_t v[4];}; struct vu{uint32_t v[4];};
#define DUB_LANES(T,expr) T r;for(int k=0;k<4;++k)r.v[k]=(expr);return r
inline vf load(const float* p) noexcept{vf r;std::memcpy(r.v,p,16);return r;}
inline void store(float* p,vf v) noexcept{std::memcpy(p,v.v,16);}
inline vf splat(float x) noexcept{DUB_LANES(vf,x);}
inline vf add(vf a,vf b) noexcept{DUB_LANES(vf,a.v[k]+b.v[k]);}
inline vf sub(vf a,vf b) noexcept{DUB_LANES(vf,a.v[k]-b.v[k]);}
inline vf mul(vf a,vf b) noexcept{DUB_LANES(vf,a.v[k]*b.v[k]);}
inline vf madd(vf a,vf b,vf c) noexcept{DUB_LANES(vf,a.v[k]+b.v[k]*c.v[k]);}
inline vf min(vf a,vf b) noexcept{DUB_LANES(vf,a.v[k]<b.v[k]?a.v[k]:b.v[k]);}
inline vf max(vf a,vf b) noexcept{DUB_LANES(vf,a.v[k]>b.v[k]?a.v[k]:b.v[k]);}
inline vf abs(vf a) noexcept{DUB_LANES(vf,a.v[k]<0?-a.v[k]:a.v[k]);}
inline vm lt(vf a,vf b) noexcept{DUB_LANES(vm,a.v[k]<b.v[k]?~0u:0u);}
inline vm gt(vf a,vf b) noexcept{DUB_LANES(vm,a.v[k]>b.v[k]?~0u:0u);}
inline vm ge(vf a,vf b) noexcept{DUB_LANES(vm,a.v[k]>=b.v[k]?~0u:0u);}
inline vm eq(vf a,vf b) noexcept{DUB_LANES(vm,a.v[k]==b.v[k]?~0u:0u);}
inline vm mand(vm a,vm b) noexcept{DUB_LANES(vm,a.v[k]&b.v[k]);}
inline vf select(vm m,vf a,vf b) noexcept{DUB_LANES(vf,m.v[k]?a.v[k]:b.v[k]);}
inline bool any(vm m) noexcept{return (m.v[0]|m.v[1]|m.v[2]|m.v[3])!=0;}
inline float hmax(vf v) noexcept{float r=v.v[0];for(int k=1;k<4;++k)r=v.v[k]>r?v.v[k]:r;return r;}
inline vf recip(vf a) noexcept{DUB_LANES(vf,1.f/a.v[k]);}
inline vi bits(vf x) noexcept{vi r;std::memcpy(r.v,x.v,16);return r;}
inline vf from_bits(vi x) noexcept{vf r;std::memcpy(r.v,x.v,16);return r;}
inline vi truncate(vf x) noexcept{DUB_LANES(vi,int32_t(x.v[k]));}
inline vf to_float(vi x) noexcept{DUB_LANES(vf,float(x.v[k]));}
inline vf to_float(vu x) noexcept{DUB_LANES(vf,float(x.v[k]));}
inline vi iadd(vi a,vi b) noexcept{DUB_LANES(vi,int32_t(uint32_t(a.v[k])+uint32_t(b.v[k])));}
inline vi isplat(int32_t x) noexcept{DUB_LANES(vi,x);}
inline vi iand(vi a,vi b) noexcept{DUB_LANES(vi,a.v[k]&b.v[k]);}
inline vi ior(vi a,vi b) noexcept{DUB_LANES(vi,a.v[k]|b.v[k]);}
template<int N> inline vi shl(vi a) noexcept{DUB_LANES(vi,int32_t(uint32_t(a.v[k])<<N));}
template<int N> inline vi sra(vi a) noexcept{DUB_LANES(vi,a.v[k]>>N);}
inline vu uload(const uint32_t* p) noexcept{vu r;std::memcpy(r.v,p,16);return r;}
inline void ustore(uint32_t* p,vu v) noexcept{std::memcpy(p,v.v,16);}
inline vu uxor(vu a,vu b) noexcept{DUB_LANES(vu,a.v[k]^b.v[k]);}
template<int N> inline vu ushl(vu a) noexcept{DUB_LANES(vu,a.v[k]<<N);}
template<int N> inline vu ushr(vu a) noexcept{DUB_LANES(vu,a.v[k]>>N);}
inline void store_stereo_s16(int16_t* out,vi l,vi r) noexcept{
    for(int k=0;k<4;++k){
        out[2*k]=int16_t(l.v[k]<-32768?-32768:l.v[k]>32767?32767:l.v[k]);
        out[2*k+1]=int16_t(r.v[k]<-32768?-32768:r.v[k]>32767?32767:r.v[k]);
    }
}
#undef DUB_LANES
#endif

// ---- Shared math built on the primitives above ---------------------------
inline vf clamp(vf x,float lo,float hi) noexcept{return min(max(x,splat(lo)),splat(hi));}
// NaN lanes become zero; finite and infinite lanes pass through.
inline vf finite(vf x) noexcept{return select(eq(x,x),x,splat(0.f));}
// Finite, denormal-free and bounded: the scalar safe() of earlier releases.
inline vf guard(vf x,float limit) noexcept{
    x=clamp(finite(x),-limit,limit);
    return select(gt(abs(x),splat(1e-20f)),x,splat(0.f));
}
// Exact floor for |x| < 2^23 (no ARMv7 rounding instruction).
inline vf floor(vf x) noexcept{
    const vf t=to_float(truncate(x));
    return select(gt(t,x),sub(t,splat(1.f)),t);
}
// x-floor(x) for x in [-1,2): two compares instead of a conversion.
inline vf wrap(vf x) noexcept{
    x=select(ge(x,splat(1.f)),sub(x,splat(1.f)),x);
    return select(lt(x,splat(0.f)),add(x,splat(1.f)),x);
}
// sin(2*pi*phase) for any finite phase within float range of integers.
// Folded to [-pi/2,pi/2]; degree-11 odd polynomial, error below 1.5e-7.
inline vf sin2pi(vf phase) noexcept{
    vf x=sub(phase,floor(add(phase,splat(.5f))));
    x=select(gt(x,splat(.25f)),sub(splat(.5f),x),x);
    x=select(lt(x,splat(-.25f)),sub(splat(-.5f),x),x);
    const vf y=mul(x,splat(6.28318530717958647f)),y2=mul(y,y);
    vf p=madd(splat(2.7557319e-6f),y2,splat(-2.5052108e-8f));
    p=madd(splat(-1.98412698e-4f),y2,p);
    p=madd(splat(8.33333333e-3f),y2,p);
    p=madd(splat(-1.66666667e-1f),y2,p);
    p=madd(splat(1.f),y2,p);
    return mul(y,p);
}
// log2 for positive normal x; Cephes logf polynomial, ~1e-7 relative.
inline vf log2(vf x) noexcept{
    const vi b=bits(x);
    vf e=to_float(iadd(sra<23>(b),isplat(-127)));
    vf m=from_bits(ior(iand(b,isplat(0x007fffff)),isplat(0x3f800000)));
    const vm big=gt(m,splat(1.41421356f));
    m=select(big,mul(m,splat(.5f)),m);e=select(big,add(e,splat(1.f)),e);
    const vf t=sub(m,splat(1.f)),z=mul(t,t);
    vf p=madd(splat(-1.1514610310e-1f),t,splat(7.0376836292e-2f));
    p=madd(splat(1.1676998740e-1f),t,p);
    p=madd(splat(-1.2420140846e-1f),t,p);
    p=madd(splat(1.4249322787e-1f),t,p);
    p=madd(splat(-1.6668057665e-1f),t,p);
    p=madd(splat(2.0000714765e-1f),t,p);
    p=madd(splat(-2.4999993993e-1f),t,p);
    p=madd(splat(3.3333331174e-1f),t,p);
    vf ln=madd(t,mul(t,z),p);
    ln=madd(ln,z,splat(-.5f));
    return madd(e,ln,splat(1.44269504089f));
}
// 2^x for x in [-126,126]; Cephes exp2f polynomial, ~2e-7 relative.
inline vf exp2(vf x) noexcept{
    x=clamp(x,-126.f,126.f);
    const vf n=floor(add(x,splat(.5f))),f=sub(x,n);
    vf p=madd(splat(1.339887440266574e-3f),f,splat(1.535336188319500e-4f));
    p=madd(splat(9.618437357674640e-3f),f,p);
    p=madd(splat(5.550332471162809e-2f),f,p);
    p=madd(splat(2.402264791363012e-1f),f,p);
    p=madd(splat(6.931472028550421e-1f),f,p);
    p=madd(splat(1.f),f,p);
    return mul(p,from_bits(shl<23>(iadd(truncate(n),isplat(127)))));
}
// Round-to-nearest-even int32 of |x| < 2^22 without changing the FP mode.
inline vi round_int(vf x) noexcept{
    return iadd(bits(add(x,splat(12582912.f))),isplat(-0x4B400000));
}
}
