#include "GUI_Paint.h"
#include <cstring> //memset()

#include "EPD_13in3e.h"

GuiPaint::GuiPaint()
    : _image(nullptr), _width(0), _height(0), _widthMemory(0), _heightMemory(0), _color(0), _rotate(0), _mirror(MIRROR_NONE), _widthByte(0), _heightByte(0), _scale(2) {
}

void GuiPaint::newImage(UBYTE *image, UWORD width, UWORD height, UWORD rotate, UWORD color) {
    _image = image;
    _widthMemory = width;
    _heightMemory = height;
    _color = color;
    _scale = 2;
    _widthByte = (width % 8 == 0) ? (width / 8) : (width / 8 + 1);
    _heightByte = height;
    _rotate = rotate;
    _mirror = MIRROR_NONE;

    if (rotate == ROTATE_0 || rotate == ROTATE_180) {
        _width = width;
        _height = height;
    } else {
        _width = height;
        _height = width;
    }
}

void GuiPaint::selectImage(UBYTE *image) {
    _image = image;
}

void GuiPaint::setRotate(const UWORD rotate) {
    if (rotate == ROTATE_0 || rotate == ROTATE_90 || rotate == ROTATE_180 || rotate == ROTATE_270) {
        _rotate = rotate;
        if (rotate == ROTATE_0 || rotate == ROTATE_180) {
            _width = _widthMemory;
            _height = _heightMemory;
        } else {
            _width = _heightMemory;
            _height = _widthMemory;
        }
    } else {
        Debug("rotate = 0, 90, 180, 270\r\n");
    }
}

void GuiPaint::setMirroring(const UBYTE mirror) {
    if (mirror == MIRROR_NONE || mirror == MIRROR_HORIZONTAL ||
        mirror == MIRROR_VERTICAL || mirror == MIRROR_ORIGIN) {
        _mirror = mirror;
    } else {
        Debug("mirror should be MIRROR_NONE, MIRROR_HORIZONTAL, MIRROR_VERTICAL or MIRROR_ORIGIN\r\n");
    }
}

void GuiPaint::setScale(const UBYTE scale) {
    if (scale == 2) {
        _scale = scale;
        _widthByte = (_widthMemory % 8 == 0) ? (_widthMemory / 8) : (_widthMemory / 8 + 1);
    } else if (scale == 4) {
        _scale = scale;
        _widthByte = (_widthMemory % 4 == 0) ? (_widthMemory / 4) : (_widthMemory / 4 + 1);
    } else if (scale == 6 || scale == 7) {
        _scale = 7;
        _widthByte = (_widthMemory % 2 == 0) ? (_widthMemory / 2) : (_widthMemory / 2 + 1);
    } else {
        Debug("Set Scale Input parameter error\r\n");
        Debug("Scale Only support: 2 4 7\r\n");
    }
}

void GuiPaint::setPixel(const UWORD xPoint, UWORD yPoint, UWORD color) const {
    if (xPoint > _width || yPoint > _height) {
        Debug("Exceeding display boundaries\r\n");
        return;
    }
    UWORD x, y;
    switch (_rotate) {
        case ROTATE_0:
            x = xPoint;
            y = yPoint;
            break;
        case ROTATE_90:
            x = yPoint;
            y = _widthMemory - xPoint - 1;
            break;
        case ROTATE_180:
            x = _widthMemory - xPoint - 1;
            y = _heightMemory - yPoint - 1;
            break;
        case ROTATE_270:
            x = _heightMemory - yPoint - 1;
            y = xPoint;
            break;
        default:
            return;
    }

    switch (_mirror) {
        case MIRROR_NONE:
            break;
        case MIRROR_HORIZONTAL:
            x = _widthMemory - x - 1;
            break;
        case MIRROR_VERTICAL:
            y = _heightMemory - y - 1;
            break;
        case MIRROR_ORIGIN:
            x = _widthMemory - x - 1;
            y = _heightMemory - y - 1;
            break;
        default:
            return;
    }

    size_t imageSize = _widthMemory * _heightMemory / 2; // Assuming max byte density

    if (_scale == 2) {
        const UDOUBLE Addr = x / 8 + y * _widthByte;
        if (Addr >= imageSize) return;
        const UBYTE rData = _image[Addr];
        if (color == BLACK)
            _image[Addr] = rData & ~(0x80 >> (x % 8));
        else
            _image[Addr] = rData | (0x80 >> (x % 8));
    } else if (_scale == 4) {
        const UDOUBLE Addr = x / 4 + y * _widthByte;
        if (Addr >= imageSize) return;
        color = color % 4;
        UBYTE rData = _image[Addr];
        rData = rData & (~(0xC0 >> ((x % 4) * 2)));
        _image[Addr] = rData | ((color << 6) >> ((x % 4) * 2));
    } else if (_scale == 6 || _scale == 7 || _scale == 16) {
        const UDOUBLE Addr = x / 2 + y * _widthByte;
        if (Addr >= imageSize) return;
        UBYTE rData = _image[Addr];
        rData = rData & (~(0xF0 >> ((x % 2) * 4)));
        _image[Addr] = rData | ((color << 4) >> ((x % 2) * 4));
    }
}

void GuiPaint::clear(const UWORD color) const {
    if (_scale == 2) {
        for (UWORD y = 0; y < _heightByte; y++) {
            for (UWORD x = 0; x < _widthByte; x++) {
                const UDOUBLE addr = x + y * _widthByte;
                _image[addr] = color;
            }
        }
    } else if (_scale == 4) {
        for (UWORD y = 0; y < _heightByte; y++) {
            for (UWORD x = 0; x < _widthByte; x++) {
                const UDOUBLE addr = x + y * _widthByte;
                _image[addr] = (color << 6) | (color << 4) | (color << 2) | color;
            }
        }
    } else if (_scale == 6 || _scale == 7 || _scale == 16) {
        for (UWORD y = 0; y < _heightByte; y++) {
            for (UWORD x = 0; x < _widthByte; x++) {
                const UDOUBLE addr = x + y * _widthByte;
                _image[addr] = (color << 4) | color;
            }
        }
    }
}

void GuiPaint::clearWindows(const UWORD xStart,const  UWORD yStart,const  UWORD xEnd,const  UWORD yEnd,const  UWORD color) const {
    for (UWORD y = yStart; y < yEnd; y++) {
        for (UWORD x = xStart; x < xEnd; x++) {
            setPixel(x, y, color);
        }
    }
}

void GuiPaint::drawPoint(const UWORD xPoint,const  UWORD yPoint,const  UWORD color,const  DOT_PIXEL dotPixel,const  DOT_STYLE dotStyle) const {
    if (xPoint > _width || yPoint > _height) {
        Debug("Paint_DrawPoint Input exceeds the normal display range\r\n");
        return;
    }

    uint32_t xDirNum, yDirNum;
    if (dotStyle == DOT_FILL_AROUND) {
        for (xDirNum = 0; xDirNum < 2 * dotPixel - 1; xDirNum++) {
            for (yDirNum = 0; yDirNum < 2 * dotPixel - 1; yDirNum++) {
                if (xPoint + xDirNum - dotPixel < 0 || yPoint + yDirNum - dotPixel < 0)
                    break;
                setPixel(xPoint + xDirNum - dotPixel, yPoint + yDirNum - dotPixel, color);
            }
        }
    } else {
        for (xDirNum = 0; xDirNum < dotPixel; xDirNum++) {
            for (yDirNum = 0; yDirNum < dotPixel; yDirNum++) {
                setPixel(xPoint + xDirNum - 1, yPoint + yDirNum - 1, color);
            }
        }
    }
}

void GuiPaint::drawLine(const UWORD xStart,const  UWORD yStart,const  UWORD xEnd,const  UWORD yEnd,const  UWORD color,const  DOT_PIXEL lineWidth,const  LINE_STYLE lineStyle) const {
    if (xStart > _width || yStart > _height || xEnd > _width || yEnd > _height) {
        Debug("Paint_DrawLine Input exceeds the normal display range\r\n");
        return;
    }

    UWORD xPoint = xStart;
    UWORD yPoint = yStart;
    const int dx = static_cast<int>(xEnd) - static_cast<int>(xStart) >= 0 ? xEnd - xStart : xStart - xEnd;
    const int dy = static_cast<int>(yEnd) - static_cast<int>(yStart) <= 0 ? yEnd - yStart : yStart - yEnd;

    const int xAddWay = xStart < xEnd ? 1 : -1;
    const int yAddWay = yStart < yEnd ? 1 : -1;

    int esp = dx + dy;
    char dottedLen = 0;

    for (;;) {
        dottedLen++;
        if (lineStyle == LINE_STYLE_DOTTED && dottedLen % 3 == 0) {
            drawPoint(xPoint, yPoint, IMAGE_BACKGROUND, lineWidth, DOT_STYLE_DFT);
            dottedLen = 0;
        } else {
            drawPoint(xPoint, yPoint, color, lineWidth, DOT_STYLE_DFT);
        }
        if (2 * esp >= dy) {
            if (xPoint == xEnd)
                break;
            esp += dy;
            xPoint += xAddWay;
        }
        if (2 * esp <= dx) {
            if (yPoint == yEnd)
                break;
            esp += dx;
            yPoint += yAddWay;
        }
    }
}

void GuiPaint::drawRectangle(const UWORD xStart, const UWORD yStart,const  UWORD xEnd, const UWORD yEnd, const UWORD color, const DOT_PIXEL lineWidth, const DRAW_FILL drawFill) const {
    if (xStart > _width || yStart > _height || xEnd > _width || yEnd > _height) {
        Debug("Input exceeds the normal display range\r\n");
        return;
    }

    if (drawFill) {
        for (UWORD yPoint = yStart; yPoint < yEnd; yPoint++) {
            drawLine(xStart, yPoint, xEnd, yPoint, color, lineWidth, LINE_STYLE_SOLID);
        }
    } else {
        drawLine(xStart, yStart, xEnd, yStart, color, lineWidth, LINE_STYLE_SOLID);
        drawLine(xStart, yStart, xStart, yEnd, color, lineWidth, LINE_STYLE_SOLID);
        drawLine(xEnd, yEnd, xEnd, yStart, color, lineWidth, LINE_STYLE_SOLID);
        drawLine(xEnd, yEnd, xStart, yEnd, color, lineWidth, LINE_STYLE_SOLID);
    }
}

void GuiPaint::drawCircle(const UWORD xCenter,const  UWORD yCenter, const UWORD radius,const  UWORD color, const DOT_PIXEL lineWidth,const  DRAW_FILL drawFill) const {
    if (xCenter > _width || yCenter >= _height) {
        Debug("Paint_DrawCircle Input exceeds the normal display range\r\n");
        return;
    }

    int16_t xCurrent = 0;
    uint16_t yCurrent = radius;
    int32_t esp = 3 - (radius << 1);

    if (drawFill == DRAW_FILL_FULL) {
        while (xCurrent <= yCurrent) {
            for (uint16_t sCountY = xCurrent; sCountY <= yCurrent; sCountY++) {
                drawPoint(xCenter + xCurrent, yCenter + sCountY, color, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                drawPoint(xCenter - xCurrent, yCenter + sCountY, color, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                drawPoint(xCenter - sCountY, yCenter + xCurrent, color, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                drawPoint(xCenter - sCountY, yCenter - xCurrent, color, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                drawPoint(xCenter - xCurrent, yCenter - sCountY, color, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                drawPoint(xCenter + xCurrent, yCenter - sCountY, color, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                drawPoint(xCenter + sCountY, yCenter - xCenter, color, DOT_PIXEL_DFT, DOT_STYLE_DFT); // BUG in original?
                drawPoint(xCenter + sCountY, yCenter + xCurrent, color, DOT_PIXEL_DFT, DOT_STYLE_DFT);
            }
            if (esp < 0)
                esp += 4 * xCurrent + 6;
            else {
                esp += 10 + 4 * (xCurrent - yCurrent);
                yCurrent--;
            }
            xCurrent++;
        }
    } else {
        while (xCurrent <= yCurrent) {
            drawPoint(xCenter + xCurrent, yCenter + yCurrent, color, lineWidth, DOT_STYLE_DFT);
            drawPoint(xCenter - xCurrent, yCenter + yCurrent, color, lineWidth, DOT_STYLE_DFT);
            drawPoint(xCenter - yCurrent, yCenter + xCurrent, color, lineWidth, DOT_STYLE_DFT);
            drawPoint(xCenter - yCurrent, yCenter - xCurrent, color, lineWidth, DOT_STYLE_DFT);
            drawPoint(xCenter - xCurrent, yCenter - yCurrent, color, lineWidth, DOT_STYLE_DFT);
            drawPoint(xCenter + xCurrent, yCenter - yCurrent, color, lineWidth, DOT_STYLE_DFT);
            drawPoint(xCenter + yCurrent, yCenter - xCurrent, color, lineWidth, DOT_STYLE_DFT);
            drawPoint(xCenter + yCurrent, yCenter + xCurrent, color, lineWidth, DOT_STYLE_DFT);

            if (esp < 0)
                esp += 4 * xCurrent + 6;
            else {
                esp += 10 + 4 * (xCurrent - yCurrent);
                yCurrent--;
            }
            xCurrent++;
        }
    }
}

void GuiPaint::drawChar(const UWORD xStart, const UWORD yStart, const char asciiChar, const sFONT *font,const  UWORD colorForeground,const  UWORD colorBackground) const {
    if (xStart > _width || yStart > _height) {
        Debug("Paint_DrawChar Input exceeds the normal display range\r\n");
        return;
    }

    uint32_t charOffset = (asciiChar - ' ') * font->Height * (font->Width / 8 + (font->Width % 8 ? 1 : 0));
    const unsigned char *ptr = &font->table[charOffset];

    for (UWORD page = 0; page < font->Height; page++) {
        for (UWORD column = 0; column < font->Width; column++) {
            if (FONT_BACKGROUND == colorBackground) {
                if (*ptr & (0x80 >> (column % 8)))
                    setPixel(xStart + column, yStart + page, colorForeground);
            } else {
                if (*ptr & (0x80 >> (column % 8))) {
                    setPixel(xStart + column, yStart + page, colorForeground);
                } else if (colorBackground != colorForeground) {
                    setPixel(xStart + column, yStart + page, colorBackground);
                }
            }
            if (column % 8 == 7)
                ptr++;
        }
        if (font->Width % 8 != 0)
            ptr++;
    }
}

void GuiPaint::drawString( UWORD xStart,  UWORD yStart, const String &pString, const sFONT *font, const UWORD colorForeground, const UWORD colorBackground, const PaintTextOrientation orientation) const {
    if (xStart > _width || yStart > _height) {
        Debug("Paint_DrawString_EN Input exceeds the normal display range\r\n");
        return;
    }

    if (orientation == TEXT_BR || orientation == TEXT_MR || orientation == TEXT_TR) {
        xStart = xStart - font->Width * pString.length();
    } else if (orientation == TEXT_BC || orientation == TEXT_MC || orientation == TEXT_TC) {
        xStart = xStart - font->Width * pString.length() / 2;
    }

    if (orientation == TEXT_BC || orientation == TEXT_BR || orientation == TEXT_BL) {
        yStart = yStart - font->Height;
    } else if (orientation == TEXT_MR || orientation == TEXT_ML || orientation == TEXT_MC) {
        yStart = yStart - font->Height / 2;
    }

    UWORD xPoint = xStart;
    UWORD yPoint = yStart;

    for (uint32_t i = 0; i < pString.length(); ++i) {
        if ((xPoint + font->Width) > _width) {
            xPoint = xStart;
            yPoint += font->Height;
        }
        if ((yPoint + font->Height) > _height) {
            xPoint = xStart;
            yPoint = yStart;
        }
        drawChar(xPoint, yPoint, pString.charAt(i), font, colorForeground, colorBackground);
        xPoint += font->Width;
    }
}

void GuiPaint::drawNum(const UWORD xPoint, const UWORD yPoint, const int32_t number,  sFONT *font, const UWORD colorForeground,const  UWORD colorBackground) const {
    if (xPoint > _width || yPoint > _height) {
        Debug("Paint_DisNum Input exceeds the normal display range\r\n");
        return;
    }
    String s = String(number);
    drawString(xPoint, yPoint, s, font, colorForeground, colorBackground);
}

void GuiPaint::drawTime(const UWORD xStart, const UWORD yStart, const PAINT_TIME *pTime, sFONT *font,const  UWORD colorForeground,const  UWORD colorBackground) const {
    char buf[10];
    sprintf(buf, "%02d:%02d:%02d", pTime->hour, pTime->min, pTime->sec);
    drawString(xStart, yStart, buf, font, colorForeground, colorBackground);
}

void GuiPaint::drawBitMap(const unsigned char *imageBuffer) const {
    UDOUBLE addr = 0;
    for (UWORD y = 0; y < _heightByte; y++) {
        for (UWORD x = 0; x < _widthByte; x++) {
            addr = x + y * _widthByte;
            _image[addr] = static_cast<unsigned char>(imageBuffer[addr]);
        }
    }
}

void GuiPaint::drawBitMapPaste(const unsigned char *imageBuffer,const  UWORD xStart, const UWORD yStart,const  UWORD imageWidth, const UWORD imageHeight, const UBYTE flipColor) const {
    UBYTE color;
    const UWORD width = (imageWidth % 8 == 0 ? imageWidth / 8 : imageWidth / 8 + 1);

    for (UWORD y = 0; y < imageHeight; y++) {
        for (UWORD x = 0; x < imageWidth; x++) {
            const UBYTE srcImage = imageBuffer[y * width + x / 8];
            if (flipColor)
                color = (((srcImage << (x % 8) & 0x80) == 0) ? 1 : 0);
            else
                color = (((srcImage << (x % 8) & 0x80) == 0) ? 0 : 1);
            setPixel(x + xStart, y + yStart, color);
        }
    }
}

void GuiPaint::drawImage(const unsigned char *imageBuffer, const UWORD xStart,const  UWORD yStart,const  UWORD wImage, const UWORD hImage) const {
    const UWORD wByte = (wImage % 8) ? (wImage / 8) + 1 : wImage / 8;
    for (UWORD y = 0; y < hImage; y++) {
        for (UWORD x = 0; x < wByte; x++) {
            const UDOUBLE addr = x + y * wByte;
            const UDOUBLE pAddr = x + (xStart / 8) + ((y + yStart) * _widthByte);
            _image[pAddr] = static_cast<unsigned char>(imageBuffer[addr]);
        }
    }
}
