#ifndef GUI_H
#define GUI_H

#include "Frame.h"
#include "Thingy.h"
#include <vector>

class GUI;

class GUIThingy : public Thingy {
public:
    GUIThingy(GUI *gui);

    virtual int h() const = 0;
};

class GUI : public Thingy {
public:
    GUI(LookyTouchy *lt)
        : _lt(lt) {}

    virtual int init(const Frame &f) {
        int y = 5;
        for (unsigned i = 0; i < _things.size(); i++) {
            _lt->add(f, 0, y, f.w(), _things[i]->h(), _things[i]);
            y += _things[i]->h();
        }

        return 0;
    }
    
    virtual void look(const Frame &f, int dt) {
       f.putline(0, 0, 0, f.h()-1);
    }

    void add(GUIThingy *thingy) {
        _things.push_back(thingy);
    }

public:
    // GAEH, such a hack
    LookyTouchy *_lt;

private:
    std::vector<GUIThingy*> _things;
};

GUIThingy::GUIThingy(GUI *gui) {
    gui->add(this);
}

class GUILabel : public GUIThingy {
public:
    GUILabel(GUI *gui, const char *s="")
        : GUIThingy(gui) {
        _text = (char*)gui->_lt->alloc(256);
        strcpy(_text, s);
    }

    virtual void look(const Frame &f, int dt) {
        f.puts(10, 0, _text);
    }

    virtual int h() const {
        return 11;
    }

    int printf(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        int res = vsprintf(_text, fmt, args);
        va_end(args);
        return res;
    }

protected:
    char *_text;
};

class GUIFPS : public GUILabel {
public:
    GUIFPS(GUI *gui)
        : GUILabel(gui) {}

    virtual void look(const Frame &f, int dt) {
        this->printf("FPS: %d (%dms)", 1000/(dt|1), dt);
        GUILabel::look(f, dt);
    }
};

class GUIButton : public GUILabel {
public:
    GUIButton(GUI *gui, const char *s=NULL, Callback<void()> cb=NULL)
        : GUILabel(gui, s), _on(false), _cb(cb) {}

    virtual void look(const Frame &f, int dt) {
        if (_on) {
            f.putrect(1, 2, f.w()-21, 14, 0x25);
            f.puts(10, 5, this->_text, 0x00);
        } else {
            f.puts(10, 5, this->_text);
            f.putline(0,  1, f.w()-21,  1);
            f.putline(0, 16, f.w()-21, 16);
            f.putline(f.w()-20, 2, f.w()-20, 15);
        }
    }

    virtual void touch(const Frame &f, int x, int y) {
        bool prev = _on;
        _on = (x != -1);

        if (prev && !_on) {
            _cb();
        }
    }

    virtual int h() const {
        return 21;
    }

private:
    bool _on;
    Callback<void()> _cb;
};

class GUISpacer : public GUIThingy {
public:
    GUISpacer(GUI *gui)
        : GUIThingy(gui) {}

    virtual int h() const {
        return 7;
    }
};

class GUISeparator : public GUISpacer {
public:
    GUISeparator(GUI *gui)
        : GUISpacer(gui) {}

    virtual void look(const Frame &f, int dt) {
        f.putline(10, 1, f.w()-10, 1, 0x25);
    }
};

#endif
