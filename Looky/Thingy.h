#ifndef THINGY_H
#define THINGY_H

#include "Frame.h"
#include "fsl_ft5406.h"

// Abstract class for renderable elements
class Thingy {
public:
    virtual int init(const Frame &f) { return 0; }
    virtual void look(const Frame &f, int dt) {}
    virtual void touch(const Frame &f, int x, int y) {}
};

#endif
