#include "common.h"
#include "skin.h"
vf_spot _main(v_model v) {
vf_spot o;
float4 pos = v.pos;
float3 pos_w = mul(m_W,pos);
float4 pos_w4 = float4(pos_w,1);
float3 norm_w = normalize(mul(m_W,v.norm));
o.hpos = mul(m_WVP,pos);
o.tc0 = v.tc.xy;
o.color = calc_spot(o.tc1,o.tc2,pos_w4,norm_w);
return o;
}
#ifdef SKIN_NONE
vf_spot main(v_model v) { return _main(v);}
#endif
#ifdef SKIN_0
vf_spot main(v_model_skinned_0 v) { return _main(skinning_0(v));}
#endif
#ifdef SKIN_1
vf_spot main(v_model_skinned_1 v) { return _main(skinning_1(v));}
#endif
#ifdef SKIN_2
vf_spot main(v_model_skinned_2 v) { return _main(skinning_2(v));}
#endif