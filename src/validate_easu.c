#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <immintrin.h>
__forceinline float FMA(float a, float b, float c) {
    return _mm_cvtss_f32(_mm_fmadd_ss(_mm_set_ss(a), _mm_set_ss(b), _mm_set_ss(c)));
}

#define IW 128
#define IH 128
#define OW 256
#define OH 256

static float rc(const float *d, int x, int y, int w, int h) {
    if (x < 0) x = 0; if (x >= w) x = w - 1;
    if (y < 0) y = 0; if (y >= h) y = h - 1;
    return d[y * w + x];
}

static void gp(float *d, int w, int h) {
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            float v = (float)(x + y) / (float)(w + h - 2);
            int bx = x / 8, by = y / 8;
            if ((bx + by) & 1) v = (v > 0.5f) ? 0.1f : 0.9f;
            if (x == w / 2 || y == h / 2) v = 1.0f;
            d[y * w + x] = v;
        }
}

static void wp(const char *p, const float *d, int w, int h) {
    FILE *f = fopen(p, "wb");
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            float v = d[y * w + x];
            if (v < 0) v = 0; if (v > 1) v = 1;
            unsigned char c = (unsigned char)(v * 255.0f + 0.5f);
            fwrite(&c, 1, 3, f);
        }
    fclose(f);
}

int main() {
    float *in = (float *)malloc(IW * IH * sizeof(float));
    float *ref = (float *)malloc(OW * OH * sizeof(float));

    gp(in, IW, IH);
    wp("C:\\TSS\\src\\ref_input.ppm", in, IW, IH);

    // GPU-style reference (matching DX12 EASU: truncation + neg-fix + pre-clamp)
    float con0 = (float)IW / (float)OW;
    float con1 = (float)IH / (float)OH;
    float con2 = 0.5f * con0 - 0.5f;
    float con3 = 0.5f * con1 - 0.5f;

    for (int oy = 0; oy < OH; oy++) {
        for (int ox = 0; ox < OW; ox++) {
            float pp_x = (float)ox * con0 + con2;
            float pp_y = (float)oy * con1 + con3;
            int ix = (pp_x < 0) ? -1 : (int)pp_x;
            int iy = (pp_y < 0) ? -1 : (int)pp_y;
            float fx = pp_x - (float)ix;
            float fy = pp_y - (float)iy;
            if (ix < 1) ix = 1;
            if (ix >= IW - 3) ix = IW - 3;
            if (iy < 1) iy = 1;
            if (iy >= IH - 3) iy = IH - 3;

            float b  = in[(iy-1) * IW + ix];        // (ix,   iy-1)
            float c  = in[(iy-1) * IW + (ix+1)];    // (ix+1, iy-1)
            float e  = in[iy * IW + (ix-1)];        // (ix-1, iy)
            float f  = in[iy * IW + ix];            // (ix,   iy)
            float g  = in[iy * IW + (ix+1)];        // (ix+1, iy)
            float hh = in[iy * IW + (ix+2)];        // (ix+2, iy)
            float ii = in[(iy+1) * IW + (ix-1)];    // (ix-1, iy+1)
            float j  = in[(iy+1) * IW + ix];        // (ix,   iy+1)
            float k  = in[(iy+1) * IW + (ix+1)];    // (ix+1, iy+1)
            float l  = in[(iy+1) * IW + (ix+2)];    // (ix+2, iy+1)
            float n  = in[(iy+2) * IW + ix];        // (ix,   iy+2)
            float o  = in[(iy+2) * IW + (ix+1)];    // (ix+1, iy+2)

            float dir_x = 0, dir_y = 0, len_val = 0;

            // Q1
            float w1 = (1.0f - fy) * (1.0f - fx);
            float dc = g - f, cb = f - e;
            float len_xy = fmaxf(fabsf(dc), fabsf(cb));
            float rcp_len = 1.0f / (len_xy + 1e-8f);
            float dir_diff = g - e;
            dir_x = FMA(dir_diff, w1, dir_x);
            float lensat = fmaxf(0.0f, fminf(fabsf(dir_diff) * rcp_len, 1.0f));
            lensat *= lensat; len_val = FMA(lensat, w1, len_val);
            float ec = j - f, ca = f - b;
            len_xy = fmaxf(fabsf(ec), fabsf(ca));
            rcp_len = 1.0f / (len_xy + 1e-8f);
            dir_diff = j - b;
            dir_y = FMA(dir_diff, w1, dir_y);
            lensat = fmaxf(0.0f, fminf(fabsf(dir_diff) * rcp_len, 1.0f));
            lensat *= lensat; len_val = FMA(lensat, w1, len_val);

            // Q2
            float w2 = fx * (1.0f - fy);
            dc = hh - g; cb = g - f;
            len_xy = fmaxf(fabsf(dc), fabsf(cb));
            rcp_len = 1.0f / (len_xy + 1e-8f);
            dir_diff = hh - f;
            dir_x = FMA(dir_diff, w2, dir_x);
            lensat = fmaxf(0.0f, fminf(fabsf(dir_diff) * rcp_len, 1.0f));
            lensat *= lensat; len_val = FMA(lensat, w2, len_val);
            ec = k - g; ca = g - c;
            len_xy = fmaxf(fabsf(ec), fabsf(ca));
            rcp_len = 1.0f / (len_xy + 1e-8f);
            dir_diff = k - c;
            dir_y = FMA(dir_diff, w2, dir_y);
            lensat = fmaxf(0.0f, fminf(fabsf(dir_diff) * rcp_len, 1.0f));
            lensat *= lensat; len_val = FMA(lensat, w2, len_val);

            // Q3
            float w3 = (1.0f - fx) * fy;
            dc = k - j; cb = j - ii;
            len_xy = fmaxf(fabsf(dc), fabsf(cb));
            rcp_len = 1.0f / (len_xy + 1e-8f);
            dir_diff = k - ii;
            dir_x = FMA(dir_diff, w3, dir_x);
            lensat = fmaxf(0.0f, fminf(fabsf(dir_diff) * rcp_len, 1.0f));
            lensat *= lensat; len_val = FMA(lensat, w3, len_val);
            ec = n - j; ca = j - f;
            len_xy = fmaxf(fabsf(ec), fabsf(ca));
            rcp_len = 1.0f / (len_xy + 1e-8f);
            dir_diff = n - f;
            dir_y = FMA(dir_diff, w3, dir_y);
            lensat = fmaxf(0.0f, fminf(fabsf(dir_diff) * rcp_len, 1.0f));
            lensat *= lensat; len_val = FMA(lensat, w3, len_val);

            // Q4
            float w4 = fx * fy;
            dc = l - k; cb = k - j;
            len_xy = fmaxf(fabsf(dc), fabsf(cb));
            rcp_len = 1.0f / (len_xy + 1e-8f);
            dir_diff = l - j;
            dir_x = FMA(dir_diff, w4, dir_x);
            lensat = fmaxf(0.0f, fminf(fabsf(dir_diff) * rcp_len, 1.0f));
            lensat *= lensat; len_val = FMA(lensat, w4, len_val);
            ec = o - k; ca = k - g;
            len_xy = fmaxf(fabsf(ec), fabsf(ca));
            rcp_len = 1.0f / (len_xy + 1e-8f);
            dir_diff = o - g;
            dir_y = FMA(dir_diff, w4, dir_y);
            lensat = fmaxf(0.0f, fminf(fabsf(dir_diff) * rcp_len, 1.0f));
            lensat *= lensat; len_val = FMA(lensat, w4, len_val);

            float tmp_dx2 = dir_x * dir_x;
            float dir_len2 = FMA(dir_y, dir_y, tmp_dx2);
            if (dir_len2 > 1.0f / 32768.0f) {
                float dir_rcp = 1.0f / sqrtf(dir_len2);
                dir_x *= dir_rcp; dir_y *= dir_rcp;
            } else { dir_x = 1.0f; dir_y = 0.0f; }

            len_val *= 0.5f; len_val *= len_val;
            float tmp_sx = dir_x * dir_x;
            float stretch = FMA(dir_y, dir_y, tmp_sx) / fmaxf(fabsf(dir_x), fabsf(dir_y));
            float len2_x = FMA(stretch - 1.0f, len_val, 1.0f);
            float len2_y = FMA(-0.5f, len_val, 1.0f);
            float lob = FMA(0.21f - 0.5f, len_val, 0.5f);
            float clp = 1.0f / lob;
            float mn = fminf(fminf(fminf(f, g), j), k);
            float mx = fmaxf(fmaxf(fmaxf(f, g), j), k);

            float aC = 0, aW = 0;

            #define TAP(off_x, off_y, tv) do { \
                float _t1 = (off_x) * dir_x; \
                float rot_x = FMA((off_y), dir_y, _t1); \
                float _t2 = (off_x) * (-dir_y); \
                float rot_y = FMA((off_y), dir_x, _t2); \
                float rx = rot_x * len2_x; \
                float ry = rot_y * len2_y; \
                float _d2x = rx * rx; \
                float d2 = fminf(FMA(ry, ry, _d2x), clp); \
                float wB = FMA(0.4f, d2, -1.0f); \
                float wA = FMA(lob, d2, -1.0f); \
                wB = wB * wB; wA = wA * wA; \
                wB = FMA(1.5625f, wB, -0.5625f); \
                float w_ = wB * wA; \
                aC = FMA((tv), w_, aC); aW += w_; \
            } while(0)

            TAP(0.0f - fx, -1.0f - fy, b);
            TAP(1.0f - fx, -1.0f - fy, c);
            TAP(-1.0f - fx, 0.0f - fy, e);
            TAP(0.0f - fx, 0.0f - fy, f);
            TAP(1.0f - fx, 0.0f - fy, g);
            TAP(2.0f - fx, 0.0f - fy, hh);
            TAP(-1.0f - fx, 1.0f - fy, ii);
            TAP(0.0f - fx, 1.0f - fy, j);
            TAP(1.0f - fx, 1.0f - fy, k);
            TAP(2.0f - fx, 1.0f - fy, l);
            TAP(0.0f - fx, 2.0f - fy, n);
            TAP(1.0f - fx, 2.0f - fy, o);

            #undef TAP

            float r = aC / aW;
            r = fmaxf(mn, r);
            r = fminf(mx, r);
            ref[oy * OW + ox] = r;
        }
    }

    wp("C:\\TSS\\src\\ref_easu_fsr1.ppm", ref, OW, OH);
    {
        FILE *rf = fopen("C:\\TSS\\src\\ref_easu_raw.f32", "wb");
        if (rf) { fwrite(ref, sizeof(float), (size_t)OW * OH, rf); fclose(rf); }
    }

    // Read DX12 output (raw float32)
    float *dx = (float *)malloc(OW * OH * sizeof(float));
    {
        FILE *f = fopen("C:\\TSS\\src\\dx12_test_output.f32", "rb");
        if (!f) { fprintf(stderr, "FAIL: can't open dx12_test_output.f32\n"); return 1; }
        size_t nr = fread(dx, sizeof(float), (size_t)OW * OH, f);
        if ((int)nr != OW * OH) {
            fprintf(stderr, "FAIL: read %zu/%d floats\n", nr, OW * OH);
            free(dx); free(in); free(ref); return 1;
        }
        fclose(f);
    }

    double psnr_sum = 0, max_err = 0;
    float rmin = 1, rmax = 0, dmin = 1, dmax = 0;
    double rsum = 0, dsum = 0;
    int rnan = 0, rinf = 0, dnan = 0, dinf = 0;

    for (int i = 0; i < OW * OH; i++) {
        float vr = ref[i], vd = dx[i];
        if (isnan(vr)) rnan++;
        if (isinf(vr)) rinf++;
        if (isnan(vd)) dnan++;
        if (isinf(vd)) dinf++;
        if (vr < rmin) rmin = vr;
        if (vr > rmax) rmax = vr;
        if (vd < dmin) dmin = vd;
        if (vd > dmax) dmax = vd;
        rsum += vr; dsum += vd;
        double diff = (double)vr - (double)vd;
        psnr_sum += diff * diff;
        double ad = fabs(diff);
        if (ad > max_err) max_err = ad;
    }

    double mse = psnr_sum / (OW * OH);
    double psnr = (psnr_sum < 1e-15) ? 999.0 : 10.0 * log10((double)(OW * OH) / psnr_sum);

    printf("=== EASU: DX12 vs FSR1 Reference ===\n");
    printf("Input: %dx%d => Output: %dx%d\n", IW, IH, OW, OH);
    printf("Ref:     [%.6f, %.6f] mean=%.6f NaN=%d Inf=%d\n", rmin, rmax, rsum / (OW * OH), rnan, rinf);
    printf("DX12:    [%.6f, %.6f] mean=%.6f NaN=%d Inf=%d\n", dmin, dmax, dsum / (OW * OH), dnan, dinf);
    printf("MSE:     %.10f\n", mse);
    printf("PSNR:    %.2f dB\n", psnr);
    printf("MaxErr:  %.6f\n", max_err);

    int ok = 0;
    if (rnan == 0 && rinf == 0) ok++;
    if (dnan == 0 && dinf == 0) ok++;
    if (psnr > 30.0) ok++;
    if (max_err < 0.2) ok++;
    printf("Checks: %d/4 passed\n", ok);
    fprintf(stderr, "Note: CPU ref uses FMA() to match GPU FMA\n");

    float *diff = (float *)malloc(OW * OH * sizeof(float));
    for (int i = 0; i < OW * OH; i++) diff[i] = fabs(ref[i] - dx[i]);
    wp("C:\\TSS\\src\\diff_easu.ppm", diff, OW, OH);

    free(in); free(ref); free(dx); free(diff);
    return (ok == 4) ? 0 : 1;
}
