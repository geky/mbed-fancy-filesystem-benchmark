#include "mbed.h"
#include "Frame.h"
#include "font.h"
#include "assert.h"

// general pixel-level stuff
void Frame::putp(int x, int y, uint8_t p) const {
    assert(x + y*_w < _w*_h);
    ((uint8_t*)_frame)[transform(x, y)] = p;
}

// font stuff for printing, font is encoded as bit-per-pixel
void Frame::putc(int x, int y, int c, uint8_t p) const {
    c -= ' ';
    for (int i = 0; i < FONT_WIDTH; i++) {
        for (int j = 0; j < FONT_HEIGHT; j++) {
            if ((font[c*FONT_WIDTH + i] >> j) & 1) {
                putp(x+i, y+j, p);
            }
        }
    }
}

void Frame::puts(int x, int y, const char *s, uint8_t p) const {
    for (; *s; s++) {
        putc(x, y, *s, p);
        x += FONT_WIDTH;
    }
}

// incremental error algorithm for rasterizing a line
void Frame::putline(int x1, int y1, int x2, int y2, uint8_t p) const {
    int dx = (x1 < x2) ? x2-x1 : x1-x2;
    int dy = (y1 < y2) ? y2-y1 : y1-y2;
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        putp(x1, y1, p);

        int err2 = 2*err;

        if (x1 == x2 && y1 == y2) {
            break;
        }

        if (err2 > -dy) {
            err -= dy;
            x1 += sx;
        }

        if (x1 == x2 && y1 == y2) {
            break;
        }

        if (err2 < dx) {
            err += dx;
            y1 += sy;
        }
    }

    putp(x2, y2, p);
}

// color rect in strips
void Frame::putrect(int x1, int y1, int dx, int dy, uint8_t p) const {
    for (int i = 0; i < dy; i++) {
        memset(&((uint8_t*)_frame)[transform(x1, y1+i)], p, dx);
    }
}

void Frame::putbuffer(int x1, int y1, int dx, int dy, void *ps) const {
    for (int i = 0; i < dy; i++) {
        memcpy(&((uint8_t*)_frame)[transform(x1, y1+i)],
                &((uint8_t*)ps)[i*dx], dx);
    }
}

// color entire frame, this is _slightly_ faster
void Frame::clear(uint8_t p) const {
    if (_w == _fwidth) {
        // fast if we're not a slice
        memset(_frame, p, _w*_h);
    } else {
        // fallback to putrect
        putrect(0, 0, _w, _h, p);
    }
}
