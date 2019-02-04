#ifndef LOOKY_TOUCHY_H
#define LOOKY_TOUCHY_H

#include "Frame.h"
#include "Thingy.h"
#include "Callback.h"
#include "fsl_ft5406.h"
#include <vector>

class LookyTouchy {
public:
    // Note we bring up a lot of board stuff in our constructor.
    // This is kinda bad, but this stuff MUST run first (probably).
    // LookyTouchy::start() just starts the rendering thread and reports
    // any errors.
    LookyTouchy();
    int start();

    // Register lookies/touchies, these take in an x/y frame to
    // transform coordinates to. Called with a Frame object for
    // rendering and a time delta in milliseconds
    void add_looky(int x, int y, int w, int h,
            Callback<void(const Frame &f, int dt)> cb);

    void add_touchy(int x, int y, int w, int h,
            Callback<void(const Frame &f, int x, int y)> cb);

    // Some info
    int w()  const;
    int h() const;

    // Allocates chunks from SRAM (which is mostly used for video-RAM)
    // note! one-time allocation! no free available. Always 64-bit aligned.
    void *alloc(size_t size);

    void add(int x, int y, int w, int h, Thingy *thingy);
    void add(const Frame &f, int x, int y, int w, int h, Thingy *thingy);

private:
    void loop();
    std::vector<Frame>   _frames;
    std::vector<Thingy*> _things;
};

#endif
