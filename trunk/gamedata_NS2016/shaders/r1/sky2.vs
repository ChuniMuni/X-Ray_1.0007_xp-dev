#include "common.h"
struct vi {
float4 p:POSITION;
float4 c:COLOR0;
float3 tc0:TEXCOORD0;
float3 tc1:TEXCOORD1;
};
struct vf {
float4 hpos:POSITION;
float4 c:COLOR0;
float3 tc0:TEXCOORD0;
float3 tc1:TEXCOORD1;
};
vf main(vi v) {
vf o;
o.hpos = mul(m_WVP,v.p);
o.c = v.c;
o.tc0 = v.tc0;
o.tc1 = v.tc1;
return o;
}