#ifdef _WIN32
#include <Adafruit_GFX_mock.h>
#else
#include <Adafruit_GFX.h>
#endif

// #include <Fonts/TomThumb.h>
// #define FONT_HEIGHT 5
// #include <Fonts/Picopixel.h>
// #define FONT_HEIGHT 6
// #include <Fonts/Tiny3x3a2pt7b.h>
// #define FONT_HEIGHT 3

// void drawCentreString(GFXCanvas1 *, const char *, int, int);

void doGraphics(GFXcanvas1 *canvas)
{
    canvas->drawPixel(2, 2, 1);

    // canvas->setFont(&TomThumb);
    canvas->setTextSize(1);
    canvas->setCursor(5, 5);
    canvas->setTextSize(1);
    canvas->print("hi :0");
}

void drawText(GFXcanvas1 *canvas, const char *text)
{
    canvas->fillScreen(0);
    // custom font
    // canvas->setFont(&TomThumb);
    // canvas->setCursor(0, FONT_HEIGHT);

    // default font
    canvas->setCursor(0, 0);

    canvas->setTextSize(1);
    canvas->setTextWrap(false);

    canvas->print(text);
    // drawCentreString(canvas, text, 0, 0);
}

// void drawCentreString(GFXCanvas1 *canvas, const char *buf, int x, int y)
// {
//     int16_t x1, y1;
//     uint16_t w, h;
//     canvas.getTextBounds(buf, x, y, &x1, &y1, &w, &h); //calc width of new string
//     canvas.setCursor(x - w / 2, y);
//     canvas.print(buf);
// }
