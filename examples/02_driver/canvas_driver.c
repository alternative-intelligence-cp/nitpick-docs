#include "npkdrv.h"

static const char *const CANON[] = {
    "ping=int64(int64)",
    "render_ppm=int64(int32,int32,int32)",
    "filter_image=int64(uint8[],int32,int32)",
    NULL,
};

int main(void) {
    npkdrv d;
    if (npkdrv_init(&d, npkdrv_iface_hash(CANON)) != 0) return 10;
    for (;;) {
        npkdrv_desc req;
        int r = npkdrv_next(&d, &req);
        if (r == 0) return 0;
        if (r < 0) return 11;
        switch (req.kernel_id) {
        case 0: {
            int64_t x;
            memcpy(&x, d.shm + req.arg_off, 8);
            int64_t reply = x + 1;
            memcpy(d.shm + req.resp_off, &reply, 8);
            break;
        }
        case 1: {
            int64_t ws, hs, cs;
            memcpy(&ws, d.shm + req.arg_off, 8);
            memcpy(&hs, d.shm + req.arg_off + 8, 8);
            memcpy(&cs, d.shm + req.arg_off + 16, 8);
            int width = (int)ws, height = (int)hs;
            FILE *f = fopen("canvas_output.ppm", "wb");
            if (!f) return 13;
            fprintf(f, "P6\n%d %d\n255\n", width, height);
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    fputc(x * 255 / width, f);
                    fputc(y * 255 / height, f);
                    fputc((int)(cs & 255), f);
                }
            }
            if (fclose(f) != 0) return 14;
            int64_t pixels = (int64_t)width * height;
            memcpy(d.shm + req.resp_off, &pixels, 8);
            break;
        }
        case 2: {
            uint64_t rel_off, len;
            memcpy(&rel_off, d.shm + req.arg_off, 8);
            memcpy(&len, d.shm + req.arg_off + 8, 8);
            uint8_t *pixels = d.shm + d.bulk_offset + rel_off;
            int64_t sum = 0;
            for (uint64_t i = 0; i < len; i++) {
                pixels[i] = 255 - pixels[i];
                sum += pixels[i];
            }
            memcpy(d.shm + req.resp_off, &sum, 8);
            break;
        }
        default:
            return 15;
        }
        if (npkdrv_complete(req.seq, 0) != 0) return 12;
    }
}
