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
#include "math.h"

#include "LittleFileSystem2.h"
#include "QSPIFBlockDevice.h"

LookyTouchy lt;
enum {
    BDOPS_MODE,
    BDWEAR_MODE,
    BDUSE_MODE,
    CONSOLE_MODE,
    MODE_COUNT,

    RAIN_MODE,
    STARS_MODE,
    RAINBOW_MODE,

    //MODE_COUNT,
};
int mode = 0;

void change_mode() {
    mode = (mode+1) % MODE_COUNT;
}

GUI gui(&lt);
GUILabel title(&gui, "LittleFS v2 demo");
GUIFPS fps(&gui);
GUISeparator sep(&gui);
GUIButton button(&gui, "Change view", change_mode);


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

// Passthrough wrapper to avoid copy paste
struct PassthroughBlockDevice : public BlockDevice {
    BlockDevice *_bd;

    PassthroughBlockDevice(BlockDevice *bd)
        : _bd(bd) {
    }

    virtual int init() {
        return _bd->init();
    }

    virtual int deinit() {
        return _bd->deinit();
    }

    virtual int sync() {
        return _bd->sync();
    }

    virtual int read(void *b, bd_addr_t addr, bd_size_t size) {
        return _bd->read(b, addr, size);
    }

    virtual int program(const void *b, bd_addr_t addr, bd_size_t size) {
        return _bd->program(b, addr, size);
    }

    virtual int erase(bd_addr_t addr, bd_size_t size) {
        return _bd->erase(addr, size);
    }

    virtual bd_size_t get_read_size() const {
        return _bd->get_read_size();
    }

    virtual bd_size_t get_program_size() const {
        return _bd->get_program_size();
    }

    virtual bd_size_t get_erase_size() const {
        return _bd->get_erase_size();
    }

    virtual bd_size_t get_erase_size(bd_addr_t addr) const {
        return _bd->get_erase_size(addr);
    }

    virtual int get_erase_value() const {
        return _bd->get_erase_value();
    }

    virtual bd_size_t size() const {
        return _bd->size();
    }

    virtual const char *get_type() const {
        if (_bd != NULL) {
            return _bd->get_type();
        }

        return NULL;
    }
};

struct BDOps : public Thingy, public PassthroughBlockDevice {
    uint32_t w;
    uint32_t h;
    uint32_t size;
    uint8_t *reads;
    uint8_t *progs;
    uint8_t *erases;

    BDOps(BlockDevice *bd)
        : PassthroughBlockDevice(bd) {
    }

    virtual int init(const Frame &f) {
        int err = _bd->init();
        if (err) {
            return err;
        }

        size = _bd->size() / _bd->get_erase_size();

        w = (int)ceil(sqrt(size));
        h = w;
        size = w*h;

        reads  = (uint8_t*)malloc(size);
        memset(reads, 0, size);
        progs  = (uint8_t*)malloc(size);
        memset(progs, 0, size);
        erases = (uint8_t*)malloc(size);
        memset(erases, 0, size);
        return 0;
    }

    virtual void look(const Frame &f, int dt) {
        if (mode != BDOPS_MODE) {
            return;
        }

        int dx = f.w() / w;
        int dy = f.h() / h;

        for (int i = 0; i < w; i++) {
            for (int j = 0; j < h; j++) {
                uint8_t r = reads[i+j*w];
                uint8_t p = progs[i+j*w];
                uint8_t e = erases[i+j*w];
                uint8_t c = ((7&(r>>5))<<3) + ((3&(p>>6))<<0) + ((7&(e>>5))<<5);
                f.putrect(dx*i, dy*j, dx, dy, c);
            }
        }

        for (int i = 0; i < size; i++) {
            reads[i] = reads[i] > 20 ? reads[i] - 20 : 0;
            progs[i] = progs[i] > 20 ? progs[i] - 20 : 0;
            erases[i] = erases[i] > 20 ? erases[i] - 20 : 0;
        }
    }

    virtual int read(void *b, bd_addr_t addr, bd_size_t size) {
        reads[addr / _bd->get_erase_size()] = 0xff;
        return _bd->read(b, addr, size);
    }

    virtual int program(const void *b, bd_addr_t addr, bd_size_t size) {
        progs[addr / _bd->get_erase_size()] = 0xff;
        return _bd->program(b, addr, size);
    }

    virtual int erase(bd_addr_t addr, bd_size_t size) {
        erases[addr / _bd->get_erase_size()] = 0xff;
        return _bd->erase(addr, size);
    }
};

struct BDWear : public Thingy, public PassthroughBlockDevice {
    uint32_t w;
    uint32_t h;
    uint32_t size;
    uint32_t *wear;

    BDWear(BlockDevice *bd)
        : PassthroughBlockDevice(bd) {
    }

    virtual int init(const Frame &f) {
        int err = _bd->init();
        if (err) {
            return err;
        }

        size = _bd->size() / _bd->get_erase_size();

        w = (int)ceil(sqrt(size));
        h = w;
        size = w*h;

        wear = (uint32_t*)malloc(size*sizeof(uint32_t));
        memset(wear, 0, size*sizeof(uint32_t));
        return 0;
    }

    virtual void look(const Frame &f, int dt) {
        if (mode != BDWEAR_MODE) {
            return;
        }

        int dx = f.w() / w;
        int dy = f.h() / h;

        uint32_t min = 0xffffffff;
        uint32_t max = 0;

        for (int i = 0; i < size; i++) {
            if (wear[i] > max) max = wear[i];
            if (wear[i] < min) min = wear[i];
        }

        for (int i = 0; i < w; i++) {
            for (int j = 0; j < h; j++) {
                uint8_t C[16] = {
                    0x24, 0x24, 0x48, 0x6c,
                    0x90, 0xb4, 0xd8, 0xfc,
                    0xf8, 0xf4, 0xf0, 0xec,
                    0xe8, 0xe4, 0xe0, 0xe0,
                };

                //uint8_t w = (uint8_t)((15-1)*((wear[i+j*w]-min) / (float)(max-min)));
                uint8_t c = (uint8_t)(7*((wear[i+j*w]-min) / (float)(max-min))) << 5;
                f.putrect(dx*i, dy*j, dx, dy, wear[i+j*w] == 0 ? 0 : C[c >> 4]);
            }
        }
    }

    virtual int erase(bd_addr_t addr, bd_size_t size) {
        wear[addr / _bd->get_erase_size()] += 1;
        return _bd->erase(addr, size);
    }
};

struct BDUse : public Thingy {
    uint32_t w;
    uint32_t h;
    uint32_t size;
    uint8_t *use;
    BlockDevice *_bd;
    LittleFileSystem2 *_lfs;

    BDUse(BlockDevice *bd, LittleFileSystem2 *lfs)
        : _bd(bd), _lfs(lfs) {
    }

    virtual int init(const Frame &f) {
        int err = _bd->init();
        if (err) {
            return err;
        }

        size = _bd->size() / _bd->get_erase_size();

        w = (int)ceil(sqrt(size));
        h = w;
        size = w*h;

        use = (uint8_t*)malloc(size);
        return 0;
    }

    static int scan(void *p, lfs2_block_t block) {
        uint8_t *use = (uint8_t*)p;
        use[block] = true;
        return 0;
    }

    virtual void look(const Frame &f, int dt) {
        if (mode != BDUSE_MODE) {
            return;
        }

        int dx = f.w() / w;
        int dy = f.h() / h;

        memset(use, 0, size);
        _lfs->_mutex.lock();
        int err = lfs2_fs_traverse(&_lfs->_lfs, scan, use);
        _lfs->_mutex.unlock();
        assert(!err);

        for (int i = 0; i < w; i++) {
            for (int j = 0; j < h; j++) {
                if (use[i+j*w]) {
                    f.putrect(dx*i, dy*j, dx, dy, 0xb7);
                }
            }
        }
    }
};

//QSPIFBlockDevice rawbd(
//    QSPI_FLASH1_IO0,
//    QSPI_FLASH1_IO1,
//    QSPI_FLASH1_IO2,
//    QSPI_FLASH1_IO3,
//    QSPI_FLASH1_SCK,
//    QSPI_FLASH1_CSN,
//    QSPIF_POLARITY_MODE_0);
HeapBlockDevice rawbd(800*128, 1, 1, 128);
BDOps bdops(&rawbd);
BDWear bdwear(&bdops);
BlockDevice &bd = bdwear;

LittleFileSystem2 fs(NULL, NULL, 128, 10, 16, 256);

BDUse bduse(&rawbd, &fs);

int main(void) {
    lt.add(380, 0, lt.w()-380, lt.h(), &gui);
    lt.add(  0, 0,        380, lt.h(), &console);
    lt.add(  0, 0,        380, lt.h(), &bdops);
    lt.add(  0, 0,        380, lt.h(), &bdwear);
    lt.add(  0, 0,        380, lt.h(), &bduse);
//    lt.add(  0, 0,        380, lt.h(), new Stars);
//    lt.add(  0, 0,        380, lt.h(), new Rainbow);
//    lt.add(  0, 0,        380, lt.h(), new Rain);

    int err = lt.start();
    assert(!err);

//    printf("Hello!\n");
//    printf("Test test test\n");
//    printf("Is this thing on?\n");
//
    printf("formatting...\n");
    err = fs.reformat(&bd);
    printf("formatted: %d\n", err);

    printf("mounting...\n");
    err = fs.mount(&bd);
    printf("mounted: %d\n", err);

    printf("sanity test...\n");
    {
//        err = fs.mkdir("tests", 0777);
//        printf("mkdir: %d\n", err);
//
        File f;
        err = f.open(&fs, "test.txt", O_WRONLY | O_CREAT | O_TRUNC);
        printf("open: %d\n", err);
        err = f.write("Hello world!", strlen("Hello World!"));
        printf("write: %d\n", err);
        err = f.close();
        printf("close: %d\n", err);
        err = f.open(&fs, "test.txt", O_RDONLY);
        printf("open: %d\n", err);
        char buffer[64];
        err = f.read(buffer, 64);
        printf("read: %d\n", err);
        err = f.close();
        printf("close: %d\n", err);
        printf("sanity test: %.64s\n", buffer);
    }
//    {
//        err = fs.mkdir("tests", 0777);
//        printf("mkdir %d\n", err);
//        wait_ms(1000);
//    }
//    {
//        File f;
//        err = f.open(&fs, "test2.txt", O_WRONLY | O_CREAT | O_TRUNC);
//        printf("open: %d\n", err);
//        wait_ms(1000);
//        err = f.write("Hello world!", strlen("Hello World!"));
//        printf("write: %d\n", err);
//        err = f.close();
//        printf("close: %d\n", err);
//        err = f.open(&fs, "test.txt", O_RDONLY);
//        printf("open: %d\n", err);
//        char buffer[64];
//        err = f.read(buffer, 64);
//        printf("read: %d\n", err);
//        err = f.close();
//        printf("close: %d\n", err);
//        printf("sanity test: %.64s\n", buffer);
//    }

    //const char *names[] = {"test0", "test1", "test2", "test3", "test4", "test5", "test6", "test7", "test8", "test9"};
    const char *names[] = {
        "test0",
        "test1",
        "test2",
        "test3",
        "test4",
        "test5",
        "test6",
        "test7",
        "test8",
        "test9"
    };

    int i = 0;
    while (true) {
        for (int j = 0; j < 10; j++) {
            if (i % (3*j+1) != 0) {
                continue;
            }
            int size = 1 << ((rand() % 10)+2);
            File f;
            const char *name = names[j];
            err = f.open(&fs, name, O_WRONLY | O_CREAT | O_TRUNC);
            assert(!err);
            for (int k = 0; k < size/32; k++) {
                uint8_t buffer[32];
                for (int l = 0; l < 32; l++) {
                    buffer[l] = rand() & 0xff;
                }
                err = f.write(buffer, 32);
                assert(err >= 0);
            }
            err = f.close();
            assert(!err);
        }

        for (int j = 0; j < 10; j++) {
            if (i % (j+1) != 0) {
                continue;
            }
            File f;
            const char *name = names[j];
            err = f.open(&fs, name, O_RDONLY);
            while (true) {
                uint8_t buffer[32];
                err = f.read(buffer, 32);
                assert(err >= 0);
                if (err < 32) {
                    break;
                }
            }
            err = f.close();
            assert(!err);
        }

        //printf("completed round %d\n", i);
        //wait_ms(1000);
        i++;
    }

    return 0;
}
