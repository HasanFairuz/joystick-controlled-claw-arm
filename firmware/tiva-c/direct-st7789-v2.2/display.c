/*
 * Tiva Claw Arm Controller v2.2
 * display.c - TFT arm animation (ST7789V3 on TM4C123GXL, SPI via SSI2)
 * Call Display_Init() once, then ArmDrawFrame(theta, phi, clawClosed)
 * whenever there's a new pose to show.
 *
 * Wiring:
 *   VCC->3.3V  GND->GND  SCL->PB4  SDA->PB7  RES->PA7  DC->PA6  CS->PB5
 *   BLK->3.3V (or a PWM pin for dimming)
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/ssi.h"
#include "driverlib/pin_map.h"

#define ST7789_WIDTH   240
#define ST7789_HEIGHT  280
#define ST7789_XSTART  0
#define ST7789_YSTART  20   /* some 240x280 panels have a 20px offset in
                                driver RAM - if the image is shifted/cut
                                off, try 0 */

#define CS_LOW()   GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5, 0)
#define CS_HIGH()  GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5, GPIO_PIN_5)
#define DC_CMD()   GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_6, 0)
#define DC_DATA()  GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_6, GPIO_PIN_6)
#define RST_LOW()  GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7, 0)
#define RST_HIGH() GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7, GPIO_PIN_7)

/* colors, RGB565 */
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_TEAL    0x0451
#define COLOR_SKY     0x5D9C
#define COLOR_DARKBG  0x18E3
#define COLOR_ORANGE  0xFC60
#define COLOR_ORANGE_HI 0xFDA0
#define COLOR_BLUE2   0x2C7F
#define COLOR_BLUE2_HI 0x461F
#define COLOR_SILVER  0xC618
#define COLOR_DARKGREY 0x39E7
#define COLOR_GOLD    0xFEA0

/* 5x7 font, one byte per column, bit0=top row -> bit6=bottom row */
static const uint8_t FONT_H[5]     = {0x7F, 0x08, 0x08, 0x08, 0x7F};
static const uint8_t FONT_E[5]     = {0x7F, 0x49, 0x49, 0x49, 0x41};
static const uint8_t FONT_L[5]     = {0x7F, 0x40, 0x40, 0x40, 0x40};
static const uint8_t FONT_O[5]     = {0x3E, 0x41, 0x41, 0x41, 0x3E};
static const uint8_t FONT_W[5]     = {0x7F, 0x20, 0x18, 0x20, 0x7F};
static const uint8_t FONT_R[5]     = {0x7F, 0x09, 0x19, 0x29, 0x46};
static const uint8_t FONT_D[5]     = {0x7F, 0x41, 0x41, 0x41, 0x3E};
static const uint8_t FONT_SPACE[5] = {0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t FONT_K[5]     = {0x7F, 0x08, 0x14, 0x22, 0x41};
static const uint8_t FONT_G[5]     = {0x3E, 0x41, 0x49, 0x49, 0x2E};
static const uint8_t FONT_P[5]     = {0x7F, 0x09, 0x09, 0x09, 0x06};
static const uint8_t FONT_I[5]     = {0x00, 0x41, 0x7F, 0x41, 0x00};
static const uint8_t FONT_C[5]     = {0x3E, 0x41, 0x41, 0x41, 0x22};
static const uint8_t FONT_A[5]     = {0x7C, 0x12, 0x11, 0x12, 0x7C};
static const uint8_t FONT_T[5]     = {0x01, 0x01, 0x7F, 0x01, 0x01};
static const uint8_t FONT_COLON[5] = {0x00, 0x00, 0x24, 0x00, 0x00};
static const uint8_t FONT_MINUS[5] = {0x08, 0x08, 0x08, 0x08, 0x08};

static const uint8_t FONT_0[5] = {0x3E, 0x51, 0x49, 0x45, 0x3E};
static const uint8_t FONT_1[5] = {0x00, 0x42, 0x7F, 0x40, 0x00};
static const uint8_t FONT_2[5] = {0x62, 0x51, 0x49, 0x49, 0x46};
static const uint8_t FONT_3[5] = {0x22, 0x41, 0x49, 0x49, 0x36};
static const uint8_t FONT_4[5] = {0x18, 0x14, 0x12, 0x7F, 0x10};
static const uint8_t FONT_5[5] = {0x27, 0x45, 0x45, 0x45, 0x39};
static const uint8_t FONT_6[5] = {0x3C, 0x4A, 0x49, 0x49, 0x30};
static const uint8_t FONT_7[5] = {0x01, 0x71, 0x09, 0x05, 0x03};
static const uint8_t FONT_8[5] = {0x36, 0x49, 0x49, 0x49, 0x36};
static const uint8_t FONT_9[5] = {0x06, 0x49, 0x49, 0x29, 0x1E};

/* theta/phi/degree glyphs - hand drawn, no room for real unicode on a
 * 5x7 font. Addressed as chars via these private-use byte codes. */
#define CH_THETA  '\x01'
#define CH_PHI    '\x02'
#define CH_DEG    '\x03'

static const uint8_t FONT_THETA[5] = {0x3E, 0x49, 0x49, 0x49, 0x3E};
static const uint8_t FONT_PHI[5]   = {0x08, 0x14, 0x7F, 0x14, 0x08};
static const uint8_t FONT_DEG[5]   = {0x02, 0x05, 0x02, 0x00, 0x00};

static const uint8_t *GetGlyph(char c)
{
    switch (c) {
        case 'H': case 'h': return FONT_H;
        case 'E': case 'e': return FONT_E;
        case 'L': case 'l': return FONT_L;
        case 'O': case 'o': return FONT_O;
        case 'W': case 'w': return FONT_W;
        case 'R': case 'r': return FONT_R;
        case 'D': case 'd': return FONT_D;
        case 'K': case 'k': return FONT_K;
        case 'G': case 'g': return FONT_G;
        case 'P': case 'p': return FONT_P;
        case 'I': case 'i': return FONT_I;
        case 'C': case 'c': return FONT_C;
        case 'A': case 'a': return FONT_A;
        case 'T': case 't': return FONT_T;
        case ':': return FONT_COLON;
        case '-': return FONT_MINUS;
        case '0': return FONT_0;
        case '1': return FONT_1;
        case '2': return FONT_2;
        case '3': return FONT_3;
        case '4': return FONT_4;
        case '5': return FONT_5;
        case '6': return FONT_6;
        case '7': return FONT_7;
        case '8': return FONT_8;
        case '9': return FONT_9;
        case CH_THETA: return FONT_THETA;
        case CH_PHI:   return FONT_PHI;
        case CH_DEG:   return FONT_DEG;
        case ' ': default:  return FONT_SPACE;
    }
}

/* sin(deg)*1000, 0-90 deg, so we don't need libm just for this */
static const int16_t SIN_TABLE[91] = {
    0,17,35,52,70,87,105,122,139,156,174,191,208,225,242,259,
    276,292,309,326,342,358,375,391,407,423,438,454,469,485,500,
    515,530,545,559,574,588,602,616,629,643,656,669,682,695,707,
    719,731,743,755,766,777,788,799,809,819,829,839,848,857,866,
    875,883,891,899,906,914,921,927,934,940,946,951,956,961,966,
    970,974,978,982,985,988,990,993,995,996,998,999,999,1000,1000
};

static int32_t SinDeg(int32_t deg)
{
    int32_t d = deg % 360;
    if (d < 0) d += 360;
    if (d <= 90)  return SIN_TABLE[d];
    if (d <= 180) return SIN_TABLE[180 - d];
    if (d <= 270) return -SIN_TABLE[d - 180];
    return -SIN_TABLE[360 - d];
}

static int32_t CosDeg(int32_t deg)
{
    return SinDeg(deg + 90);
}

/* integer sqrt, again just to dodge libm */
static uint32_t isqrt(uint32_t n)
{
    uint32_t res = 0;
    uint32_t bit = 1UL << 30;
    while (bit > n) bit >>= 2;
    while (bit != 0) {
        if (n >= res + bit) {
            n -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    return res;
}

static void DelayMs(uint32_t ms)
{
    SysCtlDelay((SysCtlClockGet() / 3000) * ms);
}

static void SPI_WriteByte(uint8_t data)
{
    uint32_t dummy;
    SSIDataPut(SSI2_BASE, data);
    while (SSIBusy(SSI2_BASE)) {}
    while (SSIDataGetNonBlocking(SSI2_BASE, &dummy)) {}  /* drain RX FIFO */
}

static void ST7789_WriteCommand(uint8_t cmd)
{
    CS_LOW();
    DC_CMD();
    SPI_WriteByte(cmd);
    CS_HIGH();
}

static void ST7789_WriteData(uint8_t data)
{
    CS_LOW();
    DC_DATA();
    SPI_WriteByte(data);
    CS_HIGH();
}

static void ST7789_WriteDataBuf(const uint8_t *buf, uint32_t len)
{
    uint32_t i;
    CS_LOW();
    DC_DATA();
    for (i = 0; i < len; i++) {
        SPI_WriteByte(buf[i]);
    }
    CS_HIGH();
}

static void ST7789_InitGPIO(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)) {}
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB)) {}
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_SSI2)) {}

    GPIOPinConfigure(GPIO_PB4_SSI2CLK);
    GPIOPinConfigure(GPIO_PB7_SSI2TX);
    GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_4 | GPIO_PIN_7);

    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_5);
    GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_6 | GPIO_PIN_7);

    CS_HIGH();
    DC_DATA();
    RST_HIGH();
}

static void ST7789_InitSSI(void)
{
    /* Conservative ST7789 SPI settings for first hardware bring-up. */
    SSIConfigSetExpClk(SSI2_BASE, SysCtlClockGet(),
                        SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER,
                        8000000, 8);
    SSIEnable(SSI2_BASE);
}

static void ST7789_HWReset(void)
{
    RST_HIGH(); DelayMs(10);
    RST_LOW();  DelayMs(10);
    RST_HIGH(); DelayMs(150);
}

static void ST7789_Init(void)
{
    static const uint8_t gammaPos[14] = {
        0xD0,0x04,0x0D,0x11,0x13,0x2B,0x3F,0x54,0x4C,0x18,0x0D,0x0B,0x1F,0x23
    };
    static const uint8_t gammaNeg[14] = {
        0xD0,0x04,0x0C,0x11,0x13,0x2C,0x3F,0x44,0x51,0x2F,0x1F,0x1F,0x20,0x23
    };
    uint8_t i;

    ST7789_WriteCommand(0x01);
    DelayMs(150);

    ST7789_WriteCommand(0x11);
    DelayMs(150);

    ST7789_WriteCommand(0x3A);
    ST7789_WriteData(0x55);

    ST7789_WriteCommand(0x36);
    ST7789_WriteData(0x00);

    ST7789_WriteCommand(0xB2);
    ST7789_WriteData(0x0C); ST7789_WriteData(0x0C);
    ST7789_WriteData(0x00); ST7789_WriteData(0x33); ST7789_WriteData(0x33);

    ST7789_WriteCommand(0xB7); ST7789_WriteData(0x35);
    ST7789_WriteCommand(0xBB); ST7789_WriteData(0x19);
    ST7789_WriteCommand(0xC0); ST7789_WriteData(0x2C);
    ST7789_WriteCommand(0xC2); ST7789_WriteData(0x01);
    ST7789_WriteCommand(0xC3); ST7789_WriteData(0x12);
    ST7789_WriteCommand(0xC4); ST7789_WriteData(0x20);
    ST7789_WriteCommand(0xC6); ST7789_WriteData(0x0F);

    ST7789_WriteCommand(0xD0);
    ST7789_WriteData(0xA4); ST7789_WriteData(0xA1);

    ST7789_WriteCommand(0xE0);
    for (i = 0U; i < 14U; i++) ST7789_WriteData(gammaPos[i]);

    ST7789_WriteCommand(0xE1);
    for (i = 0U; i < 14U; i++) ST7789_WriteData(gammaNeg[i]);

    ST7789_WriteCommand(0x21);
    ST7789_WriteCommand(0x13);
    DelayMs(10);

    ST7789_WriteCommand(0x29);
    DelayMs(120);
}

static void ST7789_SetAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    x0 += ST7789_XSTART; x1 += ST7789_XSTART;
    y0 += ST7789_YSTART; y1 += ST7789_YSTART;

    ST7789_WriteCommand(0x2A);
    uint8_t caset[4] = { (uint8_t)(x0 >> 8), (uint8_t)(x0 & 0xFF),
                          (uint8_t)(x1 >> 8), (uint8_t)(x1 & 0xFF) };
    ST7789_WriteDataBuf(caset, 4);

    ST7789_WriteCommand(0x2B);
    uint8_t raset[4] = { (uint8_t)(y0 >> 8), (uint8_t)(y0 & 0xFF),
                          (uint8_t)(y1 >> 8), (uint8_t)(y1 & 0xFF) };
    ST7789_WriteDataBuf(raset, 4);

    ST7789_WriteCommand(0x2C);
}

static void ST7789_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint32_t total, i;

    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) return;
    if ((x + w - 1) >= ST7789_WIDTH)  w = ST7789_WIDTH - x;
    if ((y + h - 1) >= ST7789_HEIGHT) h = ST7789_HEIGHT - y;

    ST7789_SetAddrWindow(x, y, x + w - 1, y + h - 1);

    CS_LOW();
    DC_DATA();
    total = (uint32_t)w * h;
    for (i = 0; i < total; i++) {
        SPI_WriteByte((uint8_t)(color >> 8));
        SPI_WriteByte((uint8_t)(color & 0xFF));
    }
    CS_HIGH();
}

static void ST7789_FillScreen(uint16_t color)
{
    ST7789_FillRect(0, 0, ST7789_WIDTH, ST7789_HEIGHT, color);
}

static void ST7789_FillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    int16_t dy, dx, rowY, rowX;
    for (dy = -r; dy <= r; dy++) {
        dx = (int16_t)isqrt((uint32_t)(r * r - dy * dy));
        rowY = y0 + dy;
        rowX = x0 - dx;
        if (rowY < 0 || rowY >= ST7789_HEIGHT) continue;
        if (rowX < 0) { dx += rowX; rowX = 0; }
        if (rowX >= ST7789_WIDTH) continue;
        ST7789_FillRect((uint16_t)rowX, (uint16_t)rowY, (uint16_t)(2 * dx + 1), 1, color);
    }
}

/* filled ellipse - base plate reads as a circle viewed at an angle */
static void ST7789_FillEllipse(int16_t cx, int16_t cy, int16_t rx, int16_t ry, uint16_t color)
{
    int16_t dy, dx, rowY, rowX;
    if (rx <= 0 || ry <= 0) return;
    for (dy = -ry; dy <= ry; dy++) {
        dx = (int16_t)(((int32_t)rx * (int32_t)isqrt((uint32_t)(ry * ry - dy * dy))) / ry);
        rowY = cy + dy;
        rowX = cx - dx;
        if (rowY < 0 || rowY >= ST7789_HEIGHT) continue;
        if (rowX < 0) { dx += rowX; rowX = 0; }
        if (rowX >= ST7789_WIDTH) continue;
        ST7789_FillRect((uint16_t)rowX, (uint16_t)rowY, (uint16_t)(2 * dx + 1), 1, color);
    }
}

static void ST7789_DrawChar(uint16_t x, uint16_t y, char c,
                             uint16_t color, uint16_t bg, uint8_t size)
{
    const uint8_t *glyph = GetGlyph(c);
    uint8_t col, row, line;
    uint16_t px, py;

    for (col = 0; col < 5; col++) {
        line = glyph[col];
        for (row = 0; row < 7; row++) {
            px = x + col * size;
            py = y + row * size;
            if (line & 0x01) {
                ST7789_FillRect(px, py, size, size, color);
            } else if (bg != color) {
                ST7789_FillRect(px, py, size, size, bg);
            }
            line >>= 1;
        }
    }
}

static void ST7789_DrawString(uint16_t x, uint16_t y, const char *str,
                               uint16_t color, uint16_t bg, uint8_t size)
{
    uint16_t cx = x;
    while (*str) {
        ST7789_DrawChar(cx, y, *str, color, bg, size);
        cx += (uint16_t)((5 + 1) * size);
        str++;
    }
}

/* thick line = circles stepped along it, cheaper than a real Bresenham+width */
static void ST7789_DrawThickLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                  uint16_t color, int16_t thickness)
{
    int16_t dx = x1 - x0, dy = y1 - y0;
    int16_t steps, i;
    int16_t adx = (dx < 0) ? -dx : dx;
    int16_t ady = (dy < 0) ? -dy : dy;
    steps = (adx > ady) ? adx : ady;
    if (steps == 0) steps = 1;

    for (i = 0; i <= steps; i++) {
        int16_t px = x0 + (dx * i) / steps;
        int16_t py = y0 + (dy * i) / steps;
        ST7789_FillCircle(px, py, thickness, color);
    }
}

static void IntToStr(int32_t val, char *out)
{
    char tmp[8];
    int8_t i = 0, j = 0;
    uint8_t neg = 0;
    uint32_t v;

    if (val < 0) { neg = 1; v = (uint32_t)(-val); } else { v = (uint32_t)val; }
    if (v == 0) { tmp[i++] = '0'; }
    while (v > 0) { tmp[i++] = (char)('0' + (v % 10)); v /= 10; }
    if (neg) out[j++] = '-';
    while (i > 0) { out[j++] = tmp[--i]; }
    out[j] = '\0';
}

/* builds "theta:xxx deg" style label. number field is padded to a fixed
 * 3 chars so the string length never changes frame to frame - otherwise
 * going from a 3-digit angle to a 2-digit one leaves the old degree
 * glyph un-erased (only cells the new string covers get repainted). */
static void BuildAngleLabel(char symbolChar, int32_t num, char *out)
{
    char numBuf[8];
    uint8_t len = 0, i, j = 0;

    IntToStr(num, numBuf);
    while (numBuf[len] != '\0') { len++; }

    out[j++] = symbolChar;
    out[j++] = ':';
    for (i = len; i < 3; i++) { out[j++] = ' '; }
    for (i = 0; i < len; i++) { out[j++] = numBuf[i]; }
    out[j++] = CH_DEG;
    out[j] = '\0';
}

static void BuildLabelNum(const char *prefix, int32_t num, char *out)
{
    uint8_t i = 0, j = 0;
    char numBuf[8];
    while (prefix[i] != '\0') { out[j++] = prefix[i++]; }
    IntToStr(num, numBuf);
    i = 0;
    while (numBuf[i] != '\0') { out[j++] = numBuf[i++]; }
    out[j] = '\0';
}

/* --- arm renderer ---
 * theta: M1 tilt off vertical, 0=up. phi: base/M2 azimuth, 0-360.
 * kinematics (base pivot fixed, upright L1, tilting link L2):
 *   x = L2*sin(theta)*cos(phi)
 *   y = L2*sin(theta)*sin(phi)
 *   z = L1 + L2*cos(theta)
 *
 * projection: oblique/cavalier camera - x maps straight to screen x
 * undistorted, y (depth) only lightly nudges screen y. this is what
 * gives the correct behaviour: phi=0/180 shows full link length
 * swinging left/right, phi=90/270 foreshortens it down to near zero
 * as it points at/away from the viewer, smoothly across all 360.
 */
#define DEPTH_TILT     300   /* 0.30*1000 - how much y nudges screenY */

#define ARM_ORIGIN_X   120   /* base pivot on screen, px */
#define ARM_ORIGIN_Y   250
#define ARM_L1         55    /* base->M1, screen px (rendering scale, not real size) */
#define ARM_L2         80    /* M1->claw, screen px (rendering scale, not real size) */

/* real link lengths (cm) - TODO: measure and fill in, only needed if
 * you want a physical reach readout back, doesn't affect sprite size */
#define PHYSICAL_L1_CM   10
#define PHYSICAL_L2_CM   12

#define CANVAS_X0      4
#define CANVAS_Y0      65
#define CANVAS_W       232
#define CANVAS_H       205

#define BASE_RX        40
#define BASE_RY        18

static void ObliqueProject(int32_t x, int32_t y, int32_t z, int16_t *sx, int16_t *sy)
{
    *sx = (int16_t)(ARM_ORIGIN_X + x);
    *sy = (int16_t)(ARM_ORIGIN_Y - z + (DEPTH_TILT * y) / 1000);
}

void ArmDrawFrame(int32_t theta_deg, int32_t phi_deg, uint8_t clawClosed)
{
    static int32_t prevTheta = -9999, prevPhi = -9999;
    static uint8_t prevClaw = 2;

    int32_t sinT = SinDeg(theta_deg), cosT = CosDeg(theta_deg);
    int32_t sinP = SinDeg(phi_deg),   cosP = CosDeg(phi_deg);

    int32_t horiz = (ARM_L2 * sinT) / 1000;
    int32_t x3 = (horiz * cosP) / 1000;
    int32_t y3 = (horiz * sinP) / 1000;
    int32_t z3 = ARM_L1 + (ARM_L2 * cosT) / 1000;

    int16_t baseSX, baseSY, m1SX, m1SY, clawSX, clawSY;
    int32_t dx, dy, len, ux, uy, px, py;
    int16_t f1X, f1Y, f2X, f2Y;
    int32_t fingerExtend = 12, fingerSpread;
    char lineT[10], linePh[10], lineC[16];

    ObliqueProject(0, 0, 0, &baseSX, &baseSY);
    ObliqueProject(0, 0, ARM_L1, &m1SX, &m1SY);
    ObliqueProject(x3, y3, z3, &clawSX, &clawSY);

    /* heading dot on the base rim, angle = phi */
    int16_t headSX = (int16_t)(baseSX + (BASE_RX * cosP) / 1000);
    int16_t headSY = (int16_t)(baseSY + (BASE_RY * sinP) / 1000);

    /* finger spread in screen space, perpendicular to the M1->claw line */
    dx = clawSX - m1SX; dy = clawSY - m1SY;
    len = (int32_t)isqrt((uint32_t)(dx * dx + dy * dy));
    if (len < 1) len = 1;
    ux = (dx * 1000) / len; uy = (dy * 1000) / len;
    px = -uy; py = ux;
    fingerSpread = clawClosed ? 4 : 15;
    f1X = (int16_t)(clawSX + (fingerExtend * ux) / 1000 + (fingerSpread * px) / 1000);
    f1Y = (int16_t)(clawSY + (fingerExtend * uy) / 1000 + (fingerSpread * py) / 1000);
    f2X = (int16_t)(clawSX + (fingerExtend * ux) / 1000 - (fingerSpread * px) / 1000);
    f2Y = (int16_t)(clawSY + (fingerExtend * uy) / 1000 - (fingerSpread * py) / 1000);

    ST7789_FillRect(CANVAS_X0, CANVAS_Y0, CANVAS_W, CANVAS_H, COLOR_DARKBG);

    /* base plate + heading dot */
    ST7789_FillEllipse(baseSX, baseSY, BASE_RX, BASE_RY, COLOR_DARKGREY);
    ST7789_FillEllipse(baseSX, baseSY, BASE_RX - 8, BASE_RY - 4, COLOR_SILVER);
    ST7789_FillCircle(headSX, headSY, 4, COLOR_ORANGE);
    ST7789_FillEllipse(baseSX, baseSY, 4, 2, COLOR_DARKGREY);

    /* upright link, base->M1 */
    ST7789_DrawThickLine(baseSX, baseSY, m1SX, m1SY, COLOR_ORANGE, 8);
    ST7789_DrawThickLine(baseSX + 2, baseSY, m1SX + 2, m1SY, COLOR_ORANGE_HI, 2);
    ST7789_FillCircle(m1SX, m1SY, 6, COLOR_DARKGREY);
    ST7789_FillCircle(m1SX, m1SY, 2, COLOR_SILVER);

    /* tilting link, M1->claw */
    ST7789_DrawThickLine(m1SX, m1SY, clawSX, clawSY, COLOR_BLUE2, 7);
    ST7789_DrawThickLine(m1SX + 2, m1SY - 2, clawSX + 2, clawSY - 2, COLOR_BLUE2_HI, 2);

    /* gripper */
    ST7789_DrawThickLine(clawSX, clawSY, f1X, f1Y, COLOR_GOLD, 3);
    ST7789_DrawThickLine(clawSX, clawSY, f2X, f2Y, COLOR_GOLD, 3);
    ST7789_FillCircle(clawSX, clawSY, 4, COLOR_DARKGREY);

    /* text readout - only redraw a label when its value actually changed */
    if (theta_deg != prevTheta) {
        BuildAngleLabel(CH_THETA, theta_deg, lineT);
        ST7789_DrawString(15, 15, lineT, COLOR_WHITE, COLOR_DARKBG, 2);
        prevTheta = theta_deg;
    }
    if (phi_deg != prevPhi) {
        BuildAngleLabel(CH_PHI, phi_deg, linePh);
        ST7789_DrawString(15, 40, linePh, COLOR_WHITE, COLOR_DARKBG, 2);
        prevPhi = phi_deg;
    }
    if (clawClosed != prevClaw) {
        BuildLabelNum("CLAW:", (int32_t)clawClosed, lineC);
        ST7789_DrawString(130, 40, lineC, clawClosed ? COLOR_RED : COLOR_GREEN, COLOR_DARKBG, 2);
        prevClaw = clawClosed;
    }
}

/* call once at startup, before the main loop */
void Display_Init(void)
{
    ST7789_InitGPIO();
    ST7789_InitSSI();
    ST7789_HWReset();
    ST7789_Init();

    /* Visible communication test: red, green and blue should each appear. */
    ST7789_FillScreen(COLOR_RED);
    DelayMs(150);
    ST7789_FillScreen(COLOR_GREEN);
    DelayMs(150);
    ST7789_FillScreen(COLOR_BLUE);
    DelayMs(150);
    ST7789_FillScreen(COLOR_DARKBG);
}
