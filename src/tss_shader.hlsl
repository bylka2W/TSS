Texture2D<float> rcas_in : register(t4);
RWTexture2D<float> lock_uav : register(u2);
RWTexture2D<float> dst_uav : register(u0);
Texture2D<float> src_tex : register(t0);
Texture2D<float> cur_tex : register(t1);
RWTexture2D<float> easu_buf : register(u4);
SamplerState src_smp : register(s0);
RWTexture2D<float> rcas_out : register(u3);
Texture2D<float> hist_tex : register(t3);
RWTexture2D<float> out_uav : register(u1);
Texture2D<float> mv_tex : register(t2);
cbuffer TSS_Constants : register(b0) {
    int w;
    int h;
    int ow;
    int oh;
};

[numthreads(8,8,1)]
void TSS_Bilinear(uint3 tid : SV_DispatchThreadID) {
    int ix;
    int iy;
    float v;
    float v00;
    float v10;
    float v01;
    float v11;
    float fx;
    float fy;
    float px;
    float py;
    float top;
    float bot;
    int hoff;
    int off0;
    float v0;
    float v1;
    float v2;
    float v3;
    float cen;
    float ww;
    float e;
    float n;
    float s;
    float cL;
    float nL;
    float sL;
    float eL;
    float wL;
    float nz_s;
    float nz_h;
    float nz_r;
    float nz;
    float m4;
    float x4;
    float hit;
    float lo;
    float rcpL;
    float pp_x;
    float pp_y;
    float ip_x;
    float ip_y;
    float b;
    float c;
    float hh;
    float ii;
    float l;
    float f;
    float g;
    float j;
    float k;
    float o;
    float dir_x;
    float dir_y;
    float len_val;
    float w1;
    float w2;
    float w3;
    float w4;
    float dc;
    float cb;
    float len_xy;
    float rcp_len;
    float dir_diff;
    float lensat;
    float ec;
    float ca;
    float dir_len2;
    float dir_rcp;
    float stretch;
    float len2_x;
    float len2_y;
    float lob;
    float clp;
    float mn;
    float mx;
    float ac;
    float aw;
    float off_x;
    float off_y;
    float rot_x;
    float rot_y;
    float d2;
    float wB;
    float wA;
    float w_tap;
    float result;
    uint x = tid.x;
    uint y = tid.y;
    if (x >= ow || y >= oh) return;
    px = (x + 0.5) / 2.0 - 0.5;
    if (px < 0.0) {
    px = 0.0;
    }
    if (px >= w) {
    px = w - 1.001;
    }
    py = (y + 0.5) / 2.0 - 0.5;
    if (py < 0.0) {
    py = 0.0;
    }
    if (py >= h) {
    py = h - 1.001;
    }
    ix = px;
    iy = py;
    fx = px - ix;
    fy = py - iy;
    if (ix < 0) {
    ix = 0;
    }
    if (ix >= w - 1) {
    ix = w - 2;
    }
    if (iy < 0) {
    iy = 0;
    }
    if (iy >= h - 1) {
    iy = h - 2;
    }
    v00 = src_tex[uint2(ix, iy)];
    v10 = src_tex[uint2(ix + 1, iy)];
    v01 = src_tex[uint2(ix, (iy + 1))];
    v11 = src_tex[uint2(ix + 1, (iy + 1))];
    top = v00 + (v10 - v00) * fx;
    bot = v01 + (v11 - v01) * fx;
    v = top + (bot - top) * fy;
    dst_uav[uint2(x, y)] = v;
}


[numthreads(8,8,1)]
void TSS_RCAS(uint3 tid : SV_DispatchThreadID) {
    int ix;
    int iy;
    float v;
    float v00;
    float v10;
    float v01;
    float v11;
    float fx;
    float fy;
    float px;
    float py;
    float top;
    float bot;
    int hoff;
    int off0;
    float v0;
    float v1;
    float v2;
    float v3;
    float cen;
    float ww;
    float e;
    float n;
    float s;
    float cL;
    float nL;
    float sL;
    float eL;
    float wL;
    float nz_s;
    float nz_h;
    float nz_r;
    float nz;
    float m4;
    float x4;
    float hit;
    float lo;
    float rcpL;
    float pp_x;
    float pp_y;
    float ip_x;
    float ip_y;
    float b;
    float c;
    float hh;
    float ii;
    float l;
    float f;
    float g;
    float j;
    float k;
    float o;
    float dir_x;
    float dir_y;
    float len_val;
    float w1;
    float w2;
    float w3;
    float w4;
    float dc;
    float cb;
    float len_xy;
    float rcp_len;
    float dir_diff;
    float lensat;
    float ec;
    float ca;
    float dir_len2;
    float dir_rcp;
    float stretch;
    float len2_x;
    float len2_y;
    float lob;
    float clp;
    float mn;
    float mx;
    float ac;
    float aw;
    float off_x;
    float off_y;
    float rot_x;
    float rot_y;
    float d2;
    float wB;
    float wA;
    float w_tap;
    float result;
    uint x = tid.x;
    uint y = tid.y;
    if (x >= ow || y >= oh) return;
    hoff = (y * ow + x) * 4;
    cen = rcas_in[uint2(x, y)];
    if (x > 0) {
    ww = rcas_in[uint2(x - 1, y)];
    } else {
    ww = cen;
    }
    if (x < ow - 1) {
    e = rcas_in[uint2(x + 1, y)];
    } else {
    e = cen;
    }
    if (y > 0) {
    n = rcas_in[uint2(x, y - 1)];
    } else {
    n = cen;
    }
    if (y < oh - 1) {
    s = rcas_in[uint2(x, y + 1)];
    } else {
    s = cen;
    }
    cL = cen + cen;
    nL = n + n; sL = s + s; eL = e + e; wL = ww + ww;
    nz_s = nL + wL + eL + sL;
    nz_h = 0.25 * nz_s - cL;
    nz_r = max(max(nL, wL), max(cL, max(eL, sL))) - min(min(nL, wL), min(cL, min(eL, sL)));
    if (nz_r < 0.001) {
    nz_r = 0.001;
    }
    nz = 1.0 - 0.5 * saturate(abs(nz_h) / nz_r);
    m4 = min(min(n, ww), min(e, s));
    x4 = max(max(n, ww), max(e, s));
    hit = max(min(m4, cen) / (4.0 * x4 + 0.001), (1.0 - max(x4, cen)) / (4.0 * m4 - 4.0 + 0.001));
    lo = max(-hit, -0.25);
    if (lo < -0.25) {
    lo = -0.25;
    }
    lo = lo * 0.5 * nz;
    rcpL = 1.0 / (4.0 * lo + 1.0);
    rcas_out[uint2(x, y)] = (lo * (n + ww + s + e) + cen) * rcpL;
}


[numthreads(8,8,1)]
void TSS_EASU(uint3 tid : SV_DispatchThreadID) {
    int ix;
    int iy;
    float v;
    float v00;
    float v10;
    float v01;
    float v11;
    float fx;
    float fy;
    float px;
    float py;
    float top;
    float bot;
    int hoff;
    int off0;
    float v0;
    float v1;
    float v2;
    float v3;
    float cen;
    float ww;
    float e;
    float n;
    float s;
    float cL;
    float nL;
    float sL;
    float eL;
    float wL;
    float nz_s;
    float nz_h;
    float nz_r;
    float nz;
    float m4;
    float x4;
    float hit;
    float lo;
    float rcpL;
    float pp_x;
    float pp_y;
    float ip_x;
    float ip_y;
    float b;
    float c;
    float hh;
    float ii;
    float l;
    float f;
    float g;
    float j;
    float k;
    float o;
    float dir_x;
    float dir_y;
    float len_val;
    float w1;
    float w2;
    float w3;
    float w4;
    float dc;
    float cb;
    float len_xy;
    float rcp_len;
    float dir_diff;
    float lensat;
    float ec;
    float ca;
    float dir_len2;
    float dir_rcp;
    float stretch;
    float len2_x;
    float len2_y;
    float lob;
    float clp;
    float mn;
    float mx;
    float ac;
    float aw;
    float off_x;
    float off_y;
    float rot_x;
    float rot_y;
    float d2;
    float wB;
    float wA;
    float w_tap;
    float result;
    uint x = tid.x;
    uint y = tid.y;
    if (x >= ow || y >= oh) return;
    pp_x = (x + 0.5) / 2.0 - 0.5;
    pp_y = (y + 0.5) / 2.0 - 0.5;
    if (pp_x < 0.0) {
    ix = -1;
    } else {
    ix = pp_x;
    }
    if (pp_y < 0.0) {
    iy = -1;
    } else {
    iy = pp_y;
    }
    fx = pp_x - ix;
    fy = pp_y - iy;
    if (ix < 1) {
    ix = 1;
    }
    if (ix >= w - 3) {
    ix = w - 3;
    }
    if (iy < 1) {
    iy = 1;
    }
    if (iy >= h - 3) {
    iy = h - 3;
    }
    b = src_tex[uint2(ix, (iy - 1))];
    c = src_tex[uint2((ix + 1), (iy - 1))];
    e = src_tex[uint2((ix - 1), iy)];
    f = src_tex[uint2(ix, iy)];
    g = src_tex[uint2((ix + 1), iy)];
    hh = src_tex[uint2((ix + 2), iy)];
    ii = src_tex[uint2((ix - 1), (iy + 1))];
    j = src_tex[uint2(ix, (iy + 1))];
    k = src_tex[uint2((ix + 1), (iy + 1))];
    l = src_tex[uint2((ix + 2), (iy + 1))];
    n = src_tex[uint2(ix, (iy + 2))];
    o = src_tex[uint2((ix + 1), (iy + 2))];
    w1 = (1.0 - fy) * (1.0 - fx);
    dc = g - f; cb = f - e;
    len_xy = max(abs(dc), abs(cb));
    rcp_len = 1.0 / (len_xy + 1e-8);
    dir_diff = g - e;
    dir_x = dir_diff * w1;
    lensat = max(0.0, min(abs(dir_diff) * rcp_len, 1.0));
    lensat = lensat * lensat;
    len_val = lensat * w1;
    ec = j - f; ca = f - b;
    len_xy = max(abs(ec), abs(ca));
    rcp_len = 1.0 / (len_xy + 1e-8);
    dir_diff = j - b;
    dir_y = dir_diff * w1;
    lensat = max(0.0, min(abs(dir_diff) * rcp_len, 1.0));
    lensat = lensat * lensat;
    len_val = len_val + lensat * w1;
    w2 = fx * (1.0 - fy);
    dc = hh - g; cb = g - f;
    len_xy = max(abs(dc), abs(cb));
    rcp_len = 1.0 / (len_xy + 1e-8);
    dir_diff = hh - f;
    dir_x = dir_x + dir_diff * w2;
    lensat = max(0.0, min(abs(dir_diff) * rcp_len, 1.0));
    lensat = lensat * lensat;
    len_val = len_val + lensat * w2;
    ec = k - g; ca = g - c;
    len_xy = max(abs(ec), abs(ca));
    rcp_len = 1.0 / (len_xy + 1e-8);
    dir_diff = k - c;
    dir_y = dir_y + dir_diff * w2;
    lensat = max(0.0, min(abs(dir_diff) * rcp_len, 1.0));
    lensat = lensat * lensat;
    len_val = len_val + lensat * w2;
    w3 = fy * (1.0 - fx);
    dc = k - j; cb = j - ii;
    len_xy = max(abs(dc), abs(cb));
    rcp_len = 1.0 / (len_xy + 1e-8);
    dir_diff = k - ii;
    dir_x = dir_x + dir_diff * w3;
    lensat = max(0.0, min(abs(dir_diff) * rcp_len, 1.0));
    lensat = lensat * lensat;
    len_val = len_val + lensat * w3;
    ec = n - j; ca = j - f;
    len_xy = max(abs(ec), abs(ca));
    rcp_len = 1.0 / (len_xy + 1e-8);
    dir_diff = n - f;
    dir_y = dir_y + dir_diff * w3;
    lensat = max(0.0, min(abs(dir_diff) * rcp_len, 1.0));
    lensat = lensat * lensat;
    len_val = len_val + lensat * w3;
    w4 = fx * fy;
    dc = l - k; cb = k - j;
    len_xy = max(abs(dc), abs(cb));
    rcp_len = 1.0 / (len_xy + 1e-8);
    dir_diff = l - j;
    dir_x = dir_x + dir_diff * w4;
    lensat = max(0.0, min(abs(dir_diff) * rcp_len, 1.0));
    lensat = lensat * lensat;
    len_val = len_val + lensat * w4;
    ec = o - k; ca = k - g;
    len_xy = max(abs(ec), abs(ca));
    rcp_len = 1.0 / (len_xy + 1e-8);
    dir_diff = o - g;
    dir_y = dir_y + dir_diff * w4;
    lensat = max(0.0, min(abs(dir_diff) * rcp_len, 1.0));
    lensat = lensat * lensat;
    len_val = len_val + lensat * w4;
    dir_len2 = dir_x * dir_x + dir_y * dir_y;
    if (dir_len2 > 1.0 / 32768.0) {
    dir_rcp = 1.0 / sqrt(dir_len2);
    dir_x = dir_x * dir_rcp;
    dir_y = dir_y * dir_rcp;
    } else {;
    dir_x = 1.0;
    dir_y = 0.0;
    }
    len_val = len_val * 0.5;
    len_val = len_val * len_val;
    stretch = (dir_x * dir_x + dir_y * dir_y) / max(abs(dir_x), abs(dir_y));
    len2_x = 1.0 + (stretch - 1.0) * len_val;
    len2_y = 1.0 + (-0.5) * len_val;
    lob = 0.5 + (0.21 - 0.5) * len_val;
    clp = 1.0 / lob;
    mn = min(min(min(f, g), j), k);
    mx = max(max(max(f, g), j), k);
    ac = 0.0; aw = 0.0;
    off_x = 0.0 - fx; off_y = -1.0 - fy;
    rot_x = off_x * dir_x + off_y * dir_y;
    rot_y = off_x * (-dir_y) + off_y * dir_x;
    rot_x = rot_x * len2_x; rot_y = rot_y * len2_y;
    d2 = rot_x * rot_x + rot_y * rot_y;
    if (d2 > clp) {
    d2 = clp;
    }
    wB = 0.4 * d2 - 1.0; wA = lob * d2 - 1.0;
    wB = wB * wB; wA = wA * wA;
    wB = 1.5625 * wB - 0.5625;
    w_tap = wB * wA;
    ac = ac + b * w_tap; aw = aw + w_tap;
    off_x = 1.0 - fx; off_y = -1.0 - fy;
    rot_x = off_x * dir_x + off_y * dir_y;
    rot_y = off_x * (-dir_y) + off_y * dir_x;
    rot_x = rot_x * len2_x; rot_y = rot_y * len2_y;
    d2 = rot_x * rot_x + rot_y * rot_y;
    if (d2 > clp) {
    d2 = clp;
    }
    wB = 0.4 * d2 - 1.0; wA = lob * d2 - 1.0;
    wB = wB * wB; wA = wA * wA;
    wB = 1.5625 * wB - 0.5625;
    w_tap = wB * wA;
    ac = ac + c * w_tap; aw = aw + w_tap;
    off_x = -1.0 - fx; off_y = 0.0 - fy;
    rot_x = off_x * dir_x + off_y * dir_y;
    rot_y = off_x * (-dir_y) + off_y * dir_x;
    rot_x = rot_x * len2_x; rot_y = rot_y * len2_y;
    d2 = rot_x * rot_x + rot_y * rot_y;
    if (d2 > clp) {
    d2 = clp;
    }
    wB = 0.4 * d2 - 1.0; wA = lob * d2 - 1.0;
    wB = wB * wB; wA = wA * wA;
    wB = 1.5625 * wB - 0.5625;
    w_tap = wB * wA;
    ac = ac + e * w_tap; aw = aw + w_tap;
    off_x = 0.0 - fx; off_y = 0.0 - fy;
    rot_x = off_x * dir_x + off_y * dir_y;
    rot_y = off_x * (-dir_y) + off_y * dir_x;
    rot_x = rot_x * len2_x; rot_y = rot_y * len2_y;
    d2 = rot_x * rot_x + rot_y * rot_y;
    if (d2 > clp) {
    d2 = clp;
    }
    wB = 0.4 * d2 - 1.0; wA = lob * d2 - 1.0;
    wB = wB * wB; wA = wA * wA;
    wB = 1.5625 * wB - 0.5625;
    w_tap = wB * wA;
    ac = ac + f * w_tap; aw = aw + w_tap;
    off_x = 1.0 - fx; off_y = 0.0 - fy;
    rot_x = off_x * dir_x + off_y * dir_y;
    rot_y = off_x * (-dir_y) + off_y * dir_x;
    rot_x = rot_x * len2_x; rot_y = rot_y * len2_y;
    d2 = rot_x * rot_x + rot_y * rot_y;
    if (d2 > clp) {
    d2 = clp;
    }
    wB = 0.4 * d2 - 1.0; wA = lob * d2 - 1.0;
    wB = wB * wB; wA = wA * wA;
    wB = 1.5625 * wB - 0.5625;
    w_tap = wB * wA;
    ac = ac + g * w_tap; aw = aw + w_tap;
    off_x = 2.0 - fx; off_y = 0.0 - fy;
    rot_x = off_x * dir_x + off_y * dir_y;
    rot_y = off_x * (-dir_y) + off_y * dir_x;
    rot_x = rot_x * len2_x; rot_y = rot_y * len2_y;
    d2 = rot_x * rot_x + rot_y * rot_y;
    if (d2 > clp) {
    d2 = clp;
    }
    wB = 0.4 * d2 - 1.0; wA = lob * d2 - 1.0;
    wB = wB * wB; wA = wA * wA;
    wB = 1.5625 * wB - 0.5625;
    w_tap = wB * wA;
    ac = ac + hh * w_tap; aw = aw + w_tap;
    off_x = -1.0 - fx; off_y = 1.0 - fy;
    rot_x = off_x * dir_x + off_y * dir_y;
    rot_y = off_x * (-dir_y) + off_y * dir_x;
    rot_x = rot_x * len2_x; rot_y = rot_y * len2_y;
    d2 = rot_x * rot_x + rot_y * rot_y;
    if (d2 > clp) {
    d2 = clp;
    }
    wB = 0.4 * d2 - 1.0; wA = lob * d2 - 1.0;
    wB = wB * wB; wA = wA * wA;
    wB = 1.5625 * wB - 0.5625;
    w_tap = wB * wA;
    ac = ac + ii * w_tap; aw = aw + w_tap;
    off_x = 0.0 - fx; off_y = 1.0 - fy;
    rot_x = off_x * dir_x + off_y * dir_y;
    rot_y = off_x * (-dir_y) + off_y * dir_x;
    rot_x = rot_x * len2_x; rot_y = rot_y * len2_y;
    d2 = rot_x * rot_x + rot_y * rot_y;
    if (d2 > clp) {
    d2 = clp;
    }
    wB = 0.4 * d2 - 1.0; wA = lob * d2 - 1.0;
    wB = wB * wB; wA = wA * wA;
    wB = 1.5625 * wB - 0.5625;
    w_tap = wB * wA;
    ac = ac + j * w_tap; aw = aw + w_tap;
    off_x = 1.0 - fx; off_y = 1.0 - fy;
    rot_x = off_x * dir_x + off_y * dir_y;
    rot_y = off_x * (-dir_y) + off_y * dir_x;
    rot_x = rot_x * len2_x; rot_y = rot_y * len2_y;
    d2 = rot_x * rot_x + rot_y * rot_y;
    if (d2 > clp) {
    d2 = clp;
    }
    wB = 0.4 * d2 - 1.0; wA = lob * d2 - 1.0;
    wB = wB * wB; wA = wA * wA;
    wB = 1.5625 * wB - 0.5625;
    w_tap = wB * wA;
    ac = ac + k * w_tap; aw = aw + w_tap;
    off_x = 2.0 - fx; off_y = 1.0 - fy;
    rot_x = off_x * dir_x + off_y * dir_y;
    rot_y = off_x * (-dir_y) + off_y * dir_x;
    rot_x = rot_x * len2_x; rot_y = rot_y * len2_y;
    d2 = rot_x * rot_x + rot_y * rot_y;
    if (d2 > clp) {
    d2 = clp;
    }
    wB = 0.4 * d2 - 1.0; wA = lob * d2 - 1.0;
    wB = wB * wB; wA = wA * wA;
    wB = 1.5625 * wB - 0.5625;
    w_tap = wB * wA;
    ac = ac + l * w_tap; aw = aw + w_tap;
    off_x = 0.0 - fx; off_y = 2.0 - fy;
    rot_x = off_x * dir_x + off_y * dir_y;
    rot_y = off_x * (-dir_y) + off_y * dir_x;
    rot_x = rot_x * len2_x; rot_y = rot_y * len2_y;
    d2 = rot_x * rot_x + rot_y * rot_y;
    if (d2 > clp) {
    d2 = clp;
    }
    wB = 0.4 * d2 - 1.0; wA = lob * d2 - 1.0;
    wB = wB * wB; wA = wA * wA;
    wB = 1.5625 * wB - 0.5625;
    w_tap = wB * wA;
    ac = ac + n * w_tap; aw = aw + w_tap;
    off_x = 1.0 - fx; off_y = 2.0 - fy;
    rot_x = off_x * dir_x + off_y * dir_y;
    rot_y = off_x * (-dir_y) + off_y * dir_x;
    rot_x = rot_x * len2_x; rot_y = rot_y * len2_y;
    d2 = rot_x * rot_x + rot_y * rot_y;
    if (d2 > clp) {
    d2 = clp;
    }
    wB = 0.4 * d2 - 1.0; wA = lob * d2 - 1.0;
    wB = wB * wB; wA = wA * wA;
    wB = 1.5625 * wB - 0.5625;
    w_tap = wB * wA;
    ac = ac + o * w_tap; aw = aw + w_tap;
    result = ac / aw;
    if (result < mn) {
    result = mn;
    }
    if (result > mx) {
    result = mx;
    }
    easu_buf[uint2(x, y)] = result;
}


