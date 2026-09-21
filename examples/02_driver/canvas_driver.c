/* canvas_driver.c — C out-of-process driver for Nitpick Bridge POC.
 * Implements:
 *   kernel 0: ping(int64) -> int64
 *   kernel 1: render_ppm(int32 width, int32 height, int32 bg_color) -> int64 (pixels rendered)
 *   kernel 2: filter_image(uint8[] pixels, int32 width, int32 height) -> int64 (checksum)
 *   kernel 3: crash_driver() -> NIL
 */

#include "/home/randy/Workspace/REPOS/nitpick/sdk/npkdrv.h"

static const char *const CANON[] = {
    "ping=int64(int64)",
    "render_ppm=int64(int32,int32,int32)",
    "filter_image=int64(uint8[],int32,int32)",
    "crash_driver=NIL()",
    NULL,
};

int main(void) {
    npkdrv d;
    uint64_t hash = npkdrv_iface_hash(CANON);
    if (npkdrv_init(&d, hash) != 0) {
        return 10;
    }

    for (;;) {
        npkdrv_desc req;
        int r = npkdrv_next(&d, &req);
        if (r == 0) return 0; // Clean shutdown from Nitpick
        if (r < 0) {
            fprintf(stderr, "canvas_driver: npkdrv_next failed (%d)\n", r);
            return 11;
        }

        switch (req.kernel_id) {
        case 0: { // ping(int64 x) -> int64
            int64_t x;
            memcpy(&x, d.shm + req.arg_off, 8);
            int64_t ret = x + 1;
            memcpy(d.shm + req.resp_off, &ret, 8);
            if (npkdrv_complete(req.seq, 0) != 0) return 12;
            break;
        }

        case 1: { // render_ppm(int32 width, int32 height, int32 bg_color) -> int64
            int64_t w_slot, h_slot, c_slot;
            memcpy(&w_slot, d.shm + req.arg_off + 0, 8);
            memcpy(&h_slot, d.shm + req.arg_off + 8, 8);
            memcpy(&c_slot, d.shm + req.arg_off + 16, 8);
            int32_t width = (int32_t)w_slot;
            int32_t height = (int32_t)h_slot;
            uint32_t bg = (uint32_t)c_slot;

            // Render a PPM image with a gradient and circle
            FILE *f = fopen("canvas_output.ppm", "wb");
            if (f) {
                fprintf(f, "P6\n%d %d\n255\n", width, height);
                int cx = width / 2;
                int cy = height / 2;
                int r2 = (width / 4) * (width / 4);

                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        int dx = x - cx;
                        int dy = y - cy;
                        uint8_t r_byte, g_byte, b_byte;
                        if (dx * dx + dy * dy <= r2) {
                            // Circle: vibrant cyan/gold
                            r_byte = 255;
                            g_byte = 215;
                            b_byte = 0;
                        } else {
                            // Background gradient
                            r_byte = (uint8_t)((bg >> 16) & 0xFF) ^ (uint8_t)(x * 255 / width);
                            g_byte = (uint8_t)((bg >> 8) & 0xFF) ^ (uint8_t)(y * 255 / height);
                            b_byte = (uint8_t)(bg & 0xFF);
                        }
                        fputc(r_byte, f);
                        fputc(g_byte, f);
                        fputc(b_byte, f);
                    }
                }
                fclose(f);
            }

            int64_t pixels = (int64_t)width * height;
            memcpy(d.shm + req.resp_off, &pixels, 8);
            if (npkdrv_complete(req.seq, 0) != 0) return 12;
            break;
        }

        case 2: { // filter_image(uint8[] pixels, int32 width, int32 height) -> int64
            // Slice layout: slot 0 = rel_off (from bulk), slot 1 = len
            uint64_t rel_off, len;
            int64_t w_slot, h_slot;
            memcpy(&rel_off, d.shm + req.arg_off + 0, 8);
            memcpy(&len, d.shm + req.arg_off + 8, 8);
            memcpy(&w_slot, d.shm + req.arg_off + 16, 8);
            memcpy(&h_slot, d.shm + req.arg_off + 24, 8);

            uint8_t *pixels = d.shm + d.bulk_offset + rel_off;

            // Invert pixels in place in shared memory and compute checksum
            int64_t sum = 0;
            for (uint64_t i = 0; i < len; i++) {
                pixels[i] = 255 - pixels[i];
                sum += pixels[i];
            }

            memcpy(d.shm + req.resp_off, &sum, 8);
            if (npkdrv_complete(req.seq, 0) != 0) return 12;
            break;
        }

        case 3: { // crash_driver() -> NIL (simulates segfault)
            fprintf(stderr, "canvas_driver: intentionally crashing with SIGSEGV to test Nitpick fault recovery...\n");
            volatile int *null_ptr = (volatile int *)0;
            *null_ptr = 42; // crash!
            break;
        }

        default:
            fprintf(stderr, "canvas_driver: unknown kernel %u\n", req.kernel_id);
            if (npkdrv_complete(req.seq, 99) != 0) return 12;
            break;
        }
    }
}
