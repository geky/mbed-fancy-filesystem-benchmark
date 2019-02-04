/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*  Standard C Included Files */
#include "mbed.h"
#include "LookyTouchy.h"
#include "GUI.h"

LookyTouchy lt;
enum {
    CONSOLE_MODE,
    RAIN_MODE,
    STARS_MODE,
    RAINBOW_MODE,

    MODE_COUNT,
};
int mode = RAIN_MODE;

void change_mode() {
    mode = (mode+1) % MODE_COUNT;
}

GUI gui(&lt);
GUILabel title(&gui, "Hello World!");
GUIFPS fps(&gui);
GUISeparator sep(&gui);
GUIButton button(&gui, "PUSH ME", change_mode);


struct Console : public Thingy, public FileHandle {
    char *buffer;
    int x;
    int y;
    int w;
    int h;

    virtual int init(const Frame &f) {
        x = 0;
        y = 0;
        w = f.w() / 5;
        h = f.h() / 11;

        buffer = (char *)lt.alloc(w*h);
        memset(buffer, 0, w*h);
        return 0;
    }

    virtual void look(const Frame &f, int dt) {
        if (mode != CONSOLE_MODE) {
            return;
        }

        for (int i = 0; i < y; i++) {
            f.puts(10, 5+i*11, &buffer[i*w]);
        }
    }

    virtual ssize_t write(const void *buf, size_t size) {
        for (unsigned i = 0; i < size; i++) {
            if (((const char *)buf)[i] == '\n' || x >= w) {
                x = 0;
                y += 1;
            } else {
                buffer[y*w + x] = ((const char *)buf)[i];
                buffer[y*w + x + 1] = '\0';
                x += 1;
            }
        }

        if (y == h) {
            memmove(buffer, buffer+w, w*h - w);
            y -= 1;
        }

        return size;
    }

    virtual ssize_t read(void *buffer, size_t size) {
        return -ENOSYS;
    }

    virtual off_t seek(off_t offset, int whence = SEEK_SET) {
        return -ESPIPE;
    }

    virtual off_t size() {
        return -ESPIPE;
    }

    virtual int close()
    {
        return 0;
    }
};

Console console;
FileHandle *mbed::mbed_override_console(int fd) {
    return &console;
}

struct Rainbow : public Thingy {
    uint64_t *buffer;
    int ctr;

    virtual int init(const Frame &f) {
        buffer = (uint64_t*)lt.alloc(f.w()*f.h());
        return 0;
    }

    virtual void look(const Frame &f, int dt) {
        if (mode != RAINBOW_MODE) {
            return;
        }

        for (int j = 0; j < (f.w()*f.h())/8; j++) {
            uint64_t x64;
            uint8_t *x8 = (uint8_t*)&x64;
            for (int i = 0; i < 8; i++) {
                x8[i] = ((8*j+i)%f.w() + ((8*j+i)/f.w()) + ctr) / 10;
            }
            buffer[j] = x64;
        }
        ctr += 1;

        f.putbuffer(0, 0, f.w(), f.h(), buffer);
    }
};


struct Stars : public Thingy {
    uint64_t *buffer;
    int ctr;

    virtual int init(const Frame &f) {
        buffer = (uint64_t*)lt.alloc(f.w()*f.h());
        return 0;
    }

    virtual void look(const Frame &f, int dt) {
        if (mode != STARS_MODE) {
            return;
        }

        for (int j = 0; j < (f.w()*f.h())/8; j++) {
            uint64_t x64 = buffer[j];
            if (!x64) { continue; }

            uint8_t *x8 = (uint8_t*)&x64;
            for (int i = 0; i < 8; i++) {
                x8[i] = (
                    ((((x8[i]&0xe0) >> 5) ? ((x8[i]&0xe0) >> 5)-!((ctr++)&0x1) : 0) << 5) |
                    ((((x8[i]&0x1c) >> 2) ? ((x8[i]&0x1c) >> 2)-!((ctr++)&0x1) : 0) << 2) |
                    ((((x8[i]&0x03) >> 0) ? ((x8[i]&0x03) >> 0)-!((ctr++)&0x3) : 0) << 0));
            }

            buffer[j] = x64;
        }

        int x = rand() % (f.w()*f.h());
        uint8_t *b = (uint8_t*)buffer;
        b[(x  )%(f.w()*f.h())] = 0xff;
        b[(x+1)%(f.w()*f.h())] = 0xff;
        b[(x+2)%(f.w()*f.h())] = 0xff;
        b[(x+3)%(f.w()*f.h())] = 0xff;
        b[(x+4)%(f.w()*f.h())] = 0xff;
        b[(x-1)%(f.w()*f.h())] = 0xff;
        b[(x-2)%(f.w()*f.h())] = 0xff;
        b[(x-3)%(f.w()*f.h())] = 0xff;
        b[(x-4)%(f.w()*f.h())] = 0xff;
        b[(x+1*f.w())%(f.w()*f.h())] = 0xff;
        b[(x+2*f.w())%(f.w()*f.h())] = 0xff;
        b[(x+3*f.w())%(f.w()*f.h())] = 0xff;
        b[(x+4*f.w())%(f.w()*f.h())] = 0xff;
        b[(x-1*f.w())%(f.w()*f.h())] = 0xff;
        b[(x-2*f.w())%(f.w()*f.h())] = 0xff;
        b[(x-3*f.w())%(f.w()*f.h())] = 0xff;
        b[(x-4*f.w())%(f.w()*f.h())] = 0xff;

        f.putbuffer(0, 0, f.w(), f.h(), buffer);
    }
};

struct Rain : public Thingy {
    static const int COUNT = 500;
    struct drop {int x; int y; int vx; int vy;};
    struct drop *drops;
    int ctr;

    virtual int init(const Frame &f) {
        drops = (struct drop*)lt.alloc(COUNT*sizeof(struct drop));
        memset(drops, 0, COUNT*sizeof(struct drop));
        return 0;
    }

    virtual void look(const Frame &f, int dt) {
        if (mode != RAIN_MODE) {
            return;
        }

        //draw all raindrops
        for (int i = 0; i < COUNT; i++) {
            if (drops[i].y <= 0) {
                continue;
            }

            int tx = 0;
            int ty = 0;
            for (int j = 0; j < 8; j++) {
                int x = ((drops[i].x+tx)/1000);
                int y = f.h() - ((drops[i].y+ty)/1000);
                int b = (j > 3) ? 3   : j;
                int w = j;
                uint8_t c = ((w << 5) | (w << 2) | (b << 0));

                if (y >= 0 && y < f.h()) {
                    f.putp((unsigned)x % f.w(), y, c);
                } else if (y >= f.h()) {
                    int d = (y-f.h())/2;
                    f.putp((unsigned)(x-d) % f.w(), f.h()-1, c);
                    f.putp((unsigned)(x+d) % f.w(), f.h()-1, c);
                }

                tx += drops[i].vx;
                ty += drops[i].vy;
            }

            // update based on veloctiy
            drops[i].x += drops[i].vx;
            drops[i].y += drops[i].vy;

            // add in "gravity"
            drops[i].vy -= 10;

            // account for air resistance
            drops[i].vx = drops[i].vx - drops[i].vx/100;
            drops[i].vy = drops[i].vy - drops[i].vy/100;
        }

        // n raindrops so we actually use all memory locations
        // generate them
        for (int i = 0; i < ((COUNT/(f.h()+8)) | 1); i++) {
            drops[ctr].x = (rand() % f.w())*1000;
            drops[ctr].y = (f.h() + 8)*1000;
            drops[ctr].vx = 0;
            drops[ctr].vy = 0;
            ctr = (ctr+1) % COUNT;
        }
    }

    virtual void touch(const Frame &f, int x, int y) {
        if (x == -1) {
            return;
        }

        x = x*1000;
        y = (f.h() - y)*1000;

        for (int i = 0; i < COUNT; i++) {
            int dx = (drops[i].x/1000)-(x/1000);
            int dy = (drops[i].y/1000)-(y/1000);
            int d = dx*dx + dy*dy;

            // find distance
            if (d > 1000) { continue; }

            // add our force
            if (drops[i].x < x) {
                drops[i].vx -= -drops[i].vy/2;
            } else {
                drops[i].vx += -drops[i].vy/2;
            }
            drops[i].vy = 0;
        }
    }
};

int main(void) {
    lt.add(380, 0, lt.w()-380, lt.h(), &gui);
    lt.add(  0, 0,        380, lt.h(), &console);
    lt.add(  0, 0,        380, lt.h(), new Stars);
    lt.add(  0, 0,        380, lt.h(), new Rainbow);
    lt.add(  0, 0,        380, lt.h(), new Rain);

    int err = lt.start();
    assert(!err);

    printf("Hello!\n");
    printf("Test test test\n");
    printf("Is this thing on?\n");

    int i = 0;
    while (true) {
        printf("ping %d\n", i++);
        wait_ms(100);
    }

    return 0;
}
