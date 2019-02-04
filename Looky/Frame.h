#ifndef FRAME_H
#define FRAME_H

/**
 * General purpose frame class for rendering stuff
 *
 * x/y coordinates up to 32-bits
 * color values are 8-bits, 3:3:2 RGB encoding
 */
class Frame {
public:
    // constructors (supports slicing!), always weak references
    // ain't got no mem for nuthin else
    Frame(int w, int h)
            : _frame(NULL)
            , _fwidth(0)
            , _x(0)
            , _y(0)
            , _w(w)
            , _h(h) {}
    Frame(int x, int y, int w, int h)
            : _frame(NULL)
            , _fwidth(0)
            , _x(x)
            , _y(y)
            , _w(w)
            , _h(h) {}
    Frame(uint64_t *frame, int w, int h)
            : _frame(frame)
            , _fwidth(w)
            , _x(0)
            , _y(0)
            , _w(w)
            , _h(h) {}
    Frame(const Frame &f)
            : _frame(f._frame)
            , _fwidth(f._fwidth)
            , _x(f._x)
            , _y(f._y)
            , _w(f._w)
            , _h(f._h) {}
    Frame(const Frame &f, int x, int y, int w, int h)
            : _frame(f._frame)
            , _fwidth(f._w)
            , _x(f._x + x)
            , _y(f._y + y)
            , _w(w)
            , _h(h) {}

    // drawing operations
    void putp(int x, int y, uint8_t p) const;
    void putc(int x, int y, int c, uint8_t p=0xff) const;
    void puts(int x, int y, const char *s, uint8_t p=0xff) const;

    void putline(int x1, int y1, int x2, int y2, uint8_t p=0xff) const;
    void putrect(int x1, int y1, int dx, int dy, uint8_t p=0xff) const;
    void putbuffer(int x1, int y1, int dx, int dy, void *ps) const;

    void clear(uint8_t p=0) const;

    // useful info
    int x() const { return _x; }
    int y() const { return _y; }
    int w() const { return _w; }
    int h() const { return _h; }

    // bounds checks + transformations
    bool inbounds(int x, int y) const {
        return x >= _x && x < (_x+_w) &&
               y >= _y && y < (_y+_h);
    }

    int transformx(int x) const { return x + _x; }
    int transformy(int y) const { return y + _y; }

    int transform(int x, int y) const {
        return transformx(x) + transformy(y)*_fwidth;
    }

    // modification to the internal frame buffer
    void setframebuffer(const Frame &f) {
        _frame = f._frame;
        _fwidth = f._fwidth;
    }

private:
    uint64_t *_frame;
    int _fwidth;
    int _x;
    int _y;
    int _w;
    int _h;
};

#endif
