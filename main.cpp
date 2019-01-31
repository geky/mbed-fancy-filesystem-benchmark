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

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define APP_LCD LCD
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
#define IMG_HEIGHT 272
#define IMG_WIDTH 480
#define LCD_INPUT_CLK_FREQ CLOCK_GetFreq(kCLOCK_LCD)
#define APP_LCD_IRQHandler LCD_IRQHandler
#define APP_LCD_IRQn LCD_IRQn
#define EXAMPLE_I2C_MASTER_BASE (I2C2_BASE)
#define I2C_MASTER_CLOCK_FREQUENCY (12000000)
#define APP_PIXEL_PER_BYTE 8

#define EXAMPLE_I2C_MASTER ((I2C_Type *)EXAMPLE_I2C_MASTER_BASE)
#define I2C_MASTER_SLAVE_ADDR_7BIT 0x7EU
#define I2C_BAUDRATE 100000U

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

#if (defined(__CC_ARM) || defined(__GNUC__))
__attribute__((aligned(8)))
#elif defined(__ICCARM__)
#pragma data_alignment = 8
#else
#error Toolchain not support.
#endif

void *vram_ptr = s_frameBufs;

/* Frame end flag. */
static volatile bool s_frameEndFlag;

/* Color palette. */
static const uint32_t palette[] = { 0x001F00000 }; //{0x0000001F}; //{0x001F0000U, 0x7C0003E0U};

/* 32x32 pixel cursor image. */
#if (defined(__CC_ARM) || defined(__GNUC__))
__attribute__((aligned(4)))
#elif defined(__ICCARM__)
#pragma data_alignment = 4
#else
#error Toolchain not support.
#endif
static const uint8_t cursor32Img0[] = {
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, /* Line 1.  */
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, /* Line 2.  */
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, /* Line 3.  */
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, /* Line 4.  */
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, /* Line 5.  */
    0xAA, 0xAA, 0xAA, 0xFA, 0xAA,
    0xAA, 0xAA, 0xAA, /* Line 6.  */
    0xAA, 0xAA, 0xAB, 0xFE, 0xAA,
    0xAA, 0xAA, 0xAA, /* Line 7.  */
    0xAA, 0xAA, 0xAB, 0xFE, 0xAA,
    0xAA, 0xAA, 0xAA, /* Line 8.  */
    0xAA, 0xAA, 0xAB, 0xFE, 0xAA,
    0xAA, 0xAA, 0xAA, /* Line 9.  */
    0xAA, 0xAA, 0xAB, 0xFE, 0xAA,
    0xAA, 0xAA, 0xAA, /* Line 10  */
    0xAA, 0xAA, 0xAB, 0xFF, 0xEA,
    0xAA, 0xAA, 0xAA, /* Line 11. */
    0xAA, 0xAA, 0xAB, 0xFF, 0xFF,
    0xAA, 0xAA, 0xAA, /* Line 12. */
    0xAA, 0xAA, 0xAB, 0xFF, 0xFF,
    0xFA, 0xAA, 0xAA, /* Line 13. */
    0xAA, 0xAA, 0xAB, 0xFF, 0xFF,
    0xFE, 0xAA, 0xAA, /* Line 14. */
    0xAA, 0xAB, 0xFB, 0xFF, 0xFF,
    0xFF, 0xAA, 0xAA, /* Line 15. */
    0xAA, 0xAB, 0xFF, 0xFF, 0xFF,
    0xFF, 0xAA, 0xAA, /* Line 16. */
    0xAA, 0xAB, 0xFF, 0xFF, 0xFF,
    0xFF, 0xAA, 0xAA, /* Line 17. */
    0xAA, 0xAA, 0xFF, 0xFF, 0xFF,
    0xFF, 0xAA, 0xAA, /* Line 18. */
    0xAA, 0xAA, 0xBF, 0xFF, 0xFF,
    0xFF, 0xAA, 0xAA, /* Line 19. */
    0xAA, 0xAA, 0xBF, 0xFF, 0xFF,
    0xFF, 0xAA, 0xAA, /* Line 20. */
    0xAA, 0xAA, 0xAF, 0xFF, 0xFF,
    0xFF, 0xAA, 0xAA, /* Line 21. */
    0xAA, 0xAA, 0xAF, 0xFF, 0xFF,
    0xFE, 0xAA, 0xAA, /* Line 22. */
    0xAA, 0xAA, 0xAB, 0xFF, 0xFF,
    0xFE, 0xAA, 0xAA, /* Line 23. */
    0xAA, 0xAA, 0xAB, 0xFF, 0xFF,
    0xFE, 0xAA, 0xAA, /* Line 24. */
    0xAA, 0xAA, 0xAA, 0xFF, 0xFF,
    0xFA, 0xAA, 0xAA, /* Line 25. */
    0xAA, 0xAA, 0xAA, 0xFF, 0xFF,
    0xFA, 0xAA, 0xAA, /* Line 26. */
    0xAA, 0xAA, 0xAA, 0xFF, 0xFF,
    0xFA, 0xAA, 0xAA, /* Line 27. */
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, /* Line 28. */
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, /* Line 29. */
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, /* Line 30. */
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, /* Line 31. */
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA /* Line 32. */
};

/*******************************************************************************
 * Code
 ******************************************************************************/
static void BOARD_InitPWM(void)
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
}

void APP_LCD_IRQHandler(void)
{
    uint32_t intStatus = LCDC_GetEnabledInterruptsPendingStatus(APP_LCD);

    LCDC_ClearInterruptsStatus(APP_LCD, intStatus);

    if (intStatus & kLCDC_VerticalCompareInterrupt)
    {
        s_frameEndFlag = true;
    }
    __DSB();
}

status_t APP_LCDC_Init(void)
{
    /* Initialize the display. */
    lcdc_config_t lcdConfig;
    lcdc_cursor_config_t cursorConfig;

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
    lcdConfig.upperPanelAddr = (uint32_t)s_frameBufs;
    lcdConfig.bpp = kLCDC_2BPP;
    lcdConfig.display = kLCDC_DisplayTFT;
    lcdConfig.swapRedBlue = true;  //false;
    lcdConfig.dataFormat = kLCDC_WinCeMode;

    LCDC_Init(APP_LCD, &lcdConfig, LCD_INPUT_CLK_FREQ);

    LCDC_SetPalette(APP_LCD, palette, ARRAY_SIZE(palette));

    /* Setup the Cursor. */
    LCDC_CursorGetDefaultConfig(&cursorConfig);

    cursorConfig.size = kLCDC_CursorSize32;
    cursorConfig.syncMode = kLCDC_CursorSync;
    cursorConfig.image[0] = (uint32_t *)cursor32Img0;

    LCDC_SetCursorConfig(APP_LCD, &cursorConfig);
    LCDC_ChooseCursor(APP_LCD, 0);

    /* Trigger interrupt at start of every vertical back porch. */
    LCDC_SetVerticalInterruptMode(APP_LCD, kLCDC_StartOfBackPorch);
    LCDC_EnableInterrupts(APP_LCD, kLCDC_VerticalCompareInterrupt);
    // NVIC_EnableIRQ(APP_LCD_IRQn);

    LCDC_EnableCursor(APP_LCD, true);

    LCDC_Start(APP_LCD);
    LCDC_PowerUp(APP_LCD);

    return kStatus_Success;
}

status_t APP_I2C_Init(void)
{
    i2c_master_config_t masterConfig;

    I2C_MasterGetDefaultConfig(&masterConfig);

    /* Change the default baudrate configuration */
    masterConfig.baudRate_Bps = I2C_BAUDRATE;

    /* Initialize the I2C master peripheral */
    I2C_MasterInit(EXAMPLE_I2C_MASTER, &masterConfig, I2C_MASTER_CLOCK_FREQUENCY);

    return kStatus_Success;
}

void APP_SetCursorPosition(int posX, int posY)
{
    posX -= 12;
    posY -= 5;

    LCDC_SetCursorPosition(APP_LCD, posX, posY);
}

int main(void)
{
    int cursorPosX = 0U;
    int cursorPosY = 0U;

    ft5406_handle_t touch_handle;
    touch_event_t touch_event;

    status_t status;

    gpio_pin_config_t pin_config = {
        kGPIO_DigitalOutput, 0,
    };

    /* Board pin, clock, debug console init */
    /* attach 12 MHz clock to FLEXCOMM0 (debug console) */
    CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);

    /* Route Main clock to LCD. */
    CLOCK_AttachClk(kMCLK_to_LCD_CLK);

    /* attach 12 MHz clock to FLEXCOMM2 (I2C master for touch controller) */
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2);

    CLOCK_EnableClock(kCLOCK_Gpio2);

    CLOCK_SetClkDiv(kCLOCK_DivLcdClk, 1, true);

    BOARD_InitPins();
    BOARD_BootClockFROHF48M();
    // BOARD_InitDebugConsole();

    /* Set the back light PWM. */
    BOARD_InitPWM();

    // APP_FillBuffer((void *)(s_frameBufs));

    status = APP_LCDC_Init();
    if (status != kStatus_Success)
    {
        safe_printf("LCD init failed\n");
    }
    assert(status == kStatus_Success);

    status = APP_I2C_Init();
    if (status != kStatus_Success)
    {
        safe_printf("I2C init failed\n");
    }
    assert(status == kStatus_Success);

    GPIO_PinInit(GPIO, 2, 27, &pin_config);
    // GPIO_WritePinOutput(GPIO, 2, 27, 1);
    GPIO->B[2][27] = 1;

    status = FT5406_Init(&touch_handle, EXAMPLE_I2C_MASTER);
    if (status != kStatus_Success)
    {
        safe_printf("Touch panel init failed\n");
    }
    assert(status == kStatus_Success);

    uint32_t xMin = 0xbd;
    uint32_t xMax = 0xea;
    uint32_t yMin = 0xb7;
    uint32_t yMax = 0x12f;

    for (;;)
    {
        if (kStatus_Success == FT5406_GetSingleTouch(&touch_handle, &touch_event, &cursorPosX, &cursorPosY))
        {
            if ((touch_event == kTouch_Down) || (touch_event == kTouch_Contact))
            {
                /* Update cursor position */
                APP_SetCursorPosition(cursorPosY, cursorPosX);
                if (xMin < cursorPosX && cursorPosX < xMax && yMin < cursorPosY && cursorPosY < yMax) {
                  safe_printf("You've clicked the button! (0x%2x, 0x%2x)\r\n", cursorPosX, cursorPosY);
                }
            }
        }
        else
        {
            safe_printf("error reading touch controller\r\n");
        }
    }
}
