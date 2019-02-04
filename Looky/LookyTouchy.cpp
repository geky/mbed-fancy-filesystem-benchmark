#include "mbed.h"
#include <stdio.h>
#include <string.h>
#include "stdio_thread.h"
#include "fsl_lcdc.h"
#include "fsl_ft5406.h"
#include "fsl_sctimer.h"
#include "fsl_gpio.h"
#include "fsl_i2c.h"
#include "board.h"
#include "pin_mux.h"
#include "image.h"
#include "LookyTouchy.h"
#include "font.h"
#include "Frame.h"

// LCD stuff
#define LCD_PANEL_CLK 9000000
#define LCD_PPL 480
#define LCD_HSW 2
#define LCD_HFP 8
#define LCD_HBP 43
#define LCD_LPP 272
#define LCD_VSW 10
#define LCD_VFP 4
#define LCD_VBP 12
#define LCD_POL_FLAGS kLCDC_InvertVsyncPolarity | kLCDC_InvertHsyncPolarity
#define LCD_HEIGHT 272
#define LCD_WIDTH 480
#define LCD_INPUT_CLK_FREQ CLOCK_GetFreq(kCLOCK_LCD)
#define I2C_MASTER_CLOCK_FREQUENCY (12000000)
#define I2C_BAUDRATE 100000U


/// Allocator for SDRAM ///
#define SDRAM_ADDR 0xa0000000
#define SDRAM_SIZE 0x01000000
static       uint64_t *sdram_ptr = (uint64_t *)(SDRAM_ADDR);
static const uint64_t *sdram_end = (uint64_t *)(SDRAM_ADDR+SDRAM_SIZE);

// Allocate memory from the external SDRAM
uint64_t *sdram_alloc(size_t size) {
    // round up to nearest uint64_t
    size = (size + (8 - 1)) / 8;

    // enough space?
    if (size > (size_t)(sdram_end - sdram_ptr)) {
        return 0;
    }

    uint64_t *p = sdram_ptr;
    sdram_ptr += size;
    return p;
}


// variables
static bool initialized = false;
static ft5406_handle_t touchy_handle;
static uint64_t *frame_buffers[2];

static EventFlags vsync;
static Thread looky_thread;
static Timer looky_timer;

/// Board-level initialization ///
static status_t LookyTouchy_PWM_Init(void)
{
    sctimer_config_t config;
    sctimer_pwm_signal_param_t pwmParam;
    uint32_t event;

    CLOCK_AttachClk(kMCLK_to_SCT_CLK);

    CLOCK_SetClkDiv(kCLOCK_DivSctClk, 2, true);

    SCTIMER_GetDefaultConfig(&config);

    SCTIMER_Init(SCT0, &config);

    pwmParam.output = kSCTIMER_Out_5;
    pwmParam.level = kSCTIMER_HighTrue;
    pwmParam.dutyCyclePercent = 5;

    SCTIMER_SetupPwm(SCT0, &pwmParam, kSCTIMER_CenterAlignedPwm, 1000U, CLOCK_GetFreq(kCLOCK_Sct), &event);
    return kStatus_Success;
}

// This IRQ links to the NVIC vector in startup.s
extern "C" void LCD_IRQHandler(void)
{
    uint32_t intStatus = LCDC_GetEnabledInterruptsPendingStatus(LCD);

    LCDC_ClearInterruptsStatus(LCD, intStatus);

    if (intStatus & kLCDC_VerticalCompareInterrupt) {
        vsync.set(1);
    }

    __DSB();
}

status_t LookyTouchy_LCD_Init(void)
{
    // Setup our internal frames to use SDRAM
    // We have two for double buffering, turns out writes are much
    // faster when memory is not in use by LCD (bus contention?)
    frame_buffers[0] = sdram_alloc(LCD_WIDTH*LCD_HEIGHT);
    frame_buffers[1] = sdram_alloc(LCD_WIDTH*LCD_HEIGHT);
    assert(frame_buffers[0] && frame_buffers[1]);

    // Initialize the display.
    lcdc_config_t lcdConfig;
    LCDC_GetDefaultConfig(&lcdConfig);
    lcdConfig.panelClock_Hz = LCD_PANEL_CLK;
    lcdConfig.ppl = LCD_PPL;
    lcdConfig.hsw = LCD_HSW;
    lcdConfig.hfp = LCD_HFP;
    lcdConfig.hbp = LCD_HBP;
    lcdConfig.lpp = LCD_LPP;
    lcdConfig.vsw = LCD_VSW;
    lcdConfig.vfp = LCD_VFP;
    lcdConfig.vbp = LCD_VBP;
    lcdConfig.polarityFlags = LCD_POL_FLAGS;
    lcdConfig.upperPanelAddr = (uint32_t)frame_buffers[0];
    lcdConfig.bpp = kLCDC_8BPP;
    lcdConfig.display = kLCDC_DisplayTFT;
    lcdConfig.swapRedBlue = true;  //false;
    lcdConfig.dataFormat = kLCDC_WinCeMode;
    LCDC_Init(LCD, &lcdConfig, LCD_INPUT_CLK_FREQ);

    // Generate 16-bit (5:6:5) RGB palette that maps 8-bit pixels (3:3:2) RGB
    // Note these are backed in registers, we can free our local palette immediately
    uint16_t *palette = (uint16_t*)malloc(256*sizeof(uint16_t));
    for (int i = 0; i < 256; i++) {
        palette[i] = (
                (((((i & 0xe0) >> 5)* 9)/2) << 11) |
                (((((i & 0x1c) >> 2)* 9)/1) <<  5) |
                (((((i & 0x03) >> 0)*21)/2) <<  0));
    }
    LCDC_SetPalette(LCD, (uint32_t*)palette, 256/2);
    free(palette);

    // Trigger interrupt at start of every vertical front porch.
    LCDC_SetVerticalInterruptMode(LCD, kLCDC_StartOfFrontPorch);
    LCDC_EnableInterrupts(LCD, kLCDC_VerticalCompareInterrupt);
    NVIC_EnableIRQ(LCD_IRQn);

    LCDC_Start(LCD);
    LCDC_PowerUp(LCD);

    return kStatus_Success;
}

status_t LookyTouchy_I2C_Init(void)
{
    i2c_master_config_t masterConfig;

    I2C_MasterGetDefaultConfig(&masterConfig);

    /* Change the default baudrate configuration */
    masterConfig.baudRate_Bps = I2C_BAUDRATE;

    /* Initialize the I2C master peripheral */
    I2C_MasterInit(I2C2, &masterConfig, I2C_MASTER_CLOCK_FREQUENCY);

    return kStatus_Success;
}

void LookyTouchy_Init() {
    // Prevent multiple initializations (We are NOT RAII)
    if (initialized) {
        return;
    }
    initialized = true;

    // initialize a bunch of board stuff
    // attach 12 MHz clock to FLEXCOMM0 (debug console)
    CLOCK_EnableClock(kCLOCK_InputMux);
    CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);

    BOARD_InitPins();
    BOARD_BootClockFROHF48M();
    // BOARD_InitDebugConsole();

    // bring up RAM here
    BOARD_InitSDRAM();

    // attach clocks to LCD
    CLOCK_AttachClk(kMCLK_to_LCD_CLK);
    // attach 12 MHz clock to FLEXCOMM2 (I2C master for touch controller)
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2);
    CLOCK_EnableClock(kCLOCK_Gpio2);
    CLOCK_SetClkDiv(kCLOCK_DivLcdClk, 1, true);

    // Set the back light PWM.
    status_t status;
    status = LookyTouchy_PWM_Init();
    assert(status == kStatus_Success);

    // Initialize the LCD
    status = LookyTouchy_LCD_Init();
    assert(status == kStatus_Success);

    // Initilaize touch sensor, I2C + FT5406 init
    status = LookyTouchy_I2C_Init();
    assert(status == kStatus_Success);

    gpio_pin_config_t pin_config = { kGPIO_DigitalOutput, 0 };
    GPIO_PinInit(GPIO, 2, 27, &pin_config);
    // GPIO_WritePinOutput(GPIO, 2, 27, 1);
    GPIO->B[2][27] = 1;

    status = FT5406_Init(&touchy_handle, I2C2);
    assert(status == kStatus_Success);
}

LookyTouchy::LookyTouchy() {
    LookyTouchy_Init();
}

/// Main rendering thread ///
void LookyTouchy::loop() {
    int fi = 0;
    while (true) {
        // find time a frame takes
        int dt = looky_timer.read_ms();
        looky_timer.reset();

        // render a frame
        uint64_t *frame_buffer = frame_buffers[fi & 1];
        fi += 1;

        Frame f(frame_buffer, LCD_WIDTH, LCD_HEIGHT);
        f.clear();

        for (unsigned i = 0; i < _frames.size(); i++) {
            _frames[i].setframebuffer(f);
            _things[i]->look(_frames[i], dt);
        }

        // begin frame update
        LCDC_SetPanelAddr(LCD, kLCDC_UpperPanel, (uint32_t)frame_buffer);
        vsync.clear(1);

        // during frame update lets check for touch panel updates?
        int tx, ty; // yes these are flipped!
        touch_event_t touch_event;
        status_t success = FT5406_GetSingleTouch(&touchy_handle, &touch_event, &ty, &tx);
        if (success == kStatus_Success) {
            for (unsigned i = 0; i < _frames.size(); i++) {
                if (_frames[i].inbounds(tx, ty) &&
                        (touch_event == kTouch_Down || touch_event == kTouch_Contact)) {
                    _things[i]->touch(_frames[i],
                            _frames[i].transformx(tx), _frames[i].transformy(ty));
                } else {
                    _things[i]->touch(_frames[i], -1, -1);
                }
            }
        }

        // wait for vsync before continuing to next frame, this signals
        // our new buffer is actually on screen
        vsync.wait_all(1);
    }
}

int LookyTouchy::start() {
    for (unsigned i = 0; i < _frames.size(); i++) {
        // init can register new things, need to copy
        // frame in case vector updates
        int err = _things[i]->init(Frame(_frames[i]));
        if (err) {
            return err;
        }
    }

    looky_timer.start();
    int err = looky_thread.start(callback(this, &LookyTouchy::loop));
    if (err) {
        return err;
    }

    return 0;
}

void LookyTouchy::add(int x, int y, int w, int h, Thingy *thingy) {
    _frames.push_back(Frame(x, y, w, h));
    _things.push_back(thingy);
}

void LookyTouchy::add(const Frame &f, int x, int y, int w, int h, Thingy *thingy) {
    _frames.push_back(Frame(f, x, y, w, h));
    _things.push_back(thingy);
}

struct LookyThingy : public Thingy {
    Callback<void(const Frame &f, int dt)> cb;

    virtual void look(const Frame &f, int dt) {
        cb(f, dt);
    }

    LookyThingy(Callback<void(const Frame &f, int dt)> cb)
            : cb(cb) {};
};

void LookyTouchy::add_looky(int x, int y, int w, int h,
        Callback<void(const Frame &f, int dt)> cb) {
    add(x, y, w, h, new LookyThingy(cb));
}

struct TouchyThingy : public Thingy {
    Callback<void(const Frame &f, int x, int y)> cb;

    virtual void touch(const Frame &f, int x, int y) {
        cb(f, x, y);
    }

    TouchyThingy(Callback<void(const Frame &f, int x, int y)> cb)
            : cb(cb) {};
};

void LookyTouchy::add_touchy(int x, int y, int w, int h,
        Callback<void(const Frame &f, int x, int y)> cb) {
    add(x, y, w, h, new TouchyThingy(cb));
}

int LookyTouchy::w() const {
    return LCD_WIDTH;
}

int LookyTouchy::h() const {
    return LCD_HEIGHT;
}

void *LookyTouchy::alloc(size_t size) {
    return sdram_alloc(size);
}
