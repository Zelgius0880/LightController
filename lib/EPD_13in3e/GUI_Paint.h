/******************************************************************************
* | File      	:   GUI_Paint.h
* | Author      :   Waveshare electronics
* | Function    :	Achieve drawing: draw points, lines, boxes, circles and
*                   their size, solid dotted line, solid rectangle hollow
*                   rectangle, solid circle hollow circle.
* | Info        :
*   Achieve display characters: Display a single character, string, number
*   Achieve time display: adaptive size display time minutes and seconds
*----------------
* |	This version:   V3.2
* | Date        :   2020-07-23
* | Info        :
* -----------------------------------------------------------------------------
* V3.2(2020-07-23):
* 1. Change: Paint_SetScale(UBYTE scale)
*			Add scale 7 for 5.65f e-Parper
* 2. Change: Paint_SetPixel(UWORD Xpoint, UWORD Ypoint, UWORD Color)  
* 			Add the branch for scale 7
* 3. Change: Paint_Clear(UWORD Color)
*			Add the branch for scale 7
*
* V3.1(2019-10-10):
* 1. Add gray level
*   PAINT Add Scale
* 2. Add void Paint_SetScale(UBYTE scale);
* 
* V3.0(2019-04-18):
* 1.Change: 
*    Paint_DrawPoint(..., DOT_STYLE DOT_STYLE)
* => Paint_DrawPoint(..., DOT_STYLE Dot_Style)
*    Paint_DrawLine(..., LINE_STYLE Line_Style, DOT_PIXEL Dot_Pixel)
* => Paint_DrawLine(..., DOT_PIXEL Line_width, LINE_STYLE Line_Style)
*    Paint_DrawRectangle(..., DRAW_FILL Filled, DOT_PIXEL Dot_Pixel)
* => Paint_DrawRectangle(..., DOT_PIXEL Line_width, DRAW_FILL Draw_Fill)
*    Paint_DrawCircle(..., DRAW_FILL Draw_Fill, DOT_PIXEL Dot_Pixel)
* => Paint_DrawCircle(..., DOT_PIXEL Line_width, DRAW_FILL Draw_Filll)
*
* -----------------------------------------------------------------------------
* V2.0(2018-11-15):
* 1.add: Paint_NewImage()
*    Create an image's properties
* 2.add: Paint_SelectImage()
*    Select the picture to be drawn
* 3.add: Paint_SetRotate()
*    Set the direction of the cache    
* 4.add: Paint_RotateImage() 
*    Can flip the picture, Support 0-360 degrees, 
*    but only 90.180.270 rotation is better
* 4.add: Paint_SetMirroring() 
*    Can Mirroring the picture, horizontal, vertical, origin
* 5.add: Paint_DrawString_CN() 
*    Can display Chinese(GB1312)   
*
* ----------------------------------------------------------------------------- 
* V1.0(2018-07-17):
*   Create library
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documnetation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to  whom the Software is
* furished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*
******************************************************************************/
#ifndef __GUI_PAINT_H
#define __GUI_PAINT_H

#include "Debug.h"
#include "DEV_Config.h"
#include "fonts.h"

/**
 * Display rotate
**/
#define ROTATE_0            0
#define ROTATE_90           90
#define ROTATE_180          180
#define ROTATE_270          270

/**
 * Display Flip
**/
typedef enum {
    MIRROR_NONE = 0x00,
    MIRROR_HORIZONTAL = 0x01,
    MIRROR_VERTICAL = 0x02,
    MIRROR_ORIGIN = 0x03,
} MIRROR_IMAGE;

#define MIRROR_IMAGE_DFT MIRROR_NONE

/**
 * image color
**/
#define WHITE          0xFF
#define BLACK          0x00
#define RED            BLACK

#define IMAGE_BACKGROUND    WHITE
#define FONT_FOREGROUND     BLACK
#define FONT_BACKGROUND     WHITE

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

//4 Gray level
#define  GRAY1 0x03 //Blackest
#define  GRAY2 0x02
#define  GRAY3 0x01 //gray
#define  GRAY4 0x00 //white
/**
 * The size of the point
**/
typedef enum {
    DOT_PIXEL_1X1 = 1, // 1 x 1
    DOT_PIXEL_2X2, // 2 X 2
    DOT_PIXEL_3X3, // 3 X 3
    DOT_PIXEL_4X4, // 4 X 4
    DOT_PIXEL_5X5, // 5 X 5
    DOT_PIXEL_6X6, // 6 X 6
    DOT_PIXEL_7X7, // 7 X 7
    DOT_PIXEL_8X8, // 8 X 8
} DOT_PIXEL;

#define DOT_PIXEL_DFT  DOT_PIXEL_1X1  //Default dot pilex

typedef enum {
    TEXT_TR = 1,
    TEXT_TL,
    TEXT_TC,
    TEXT_BR,
    TEXT_BL,
    TEXT_BC,
    TEXT_MR,
    TEXT_ML,
    TEXT_MC,
} PaintTextOrientation;

/**
 * Point size fill style
**/
typedef enum {
    DOT_FILL_AROUND = 1, // dot pixel 1 x 1
    DOT_FILL_RIGHTUP, // dot pixel 2 X 2
} DOT_STYLE;

#define DOT_STYLE_DFT  DOT_FILL_AROUND  //Default dot pilex

/**
 * Line style, solid or dashed
**/
typedef enum {
    LINE_STYLE_SOLID = 0,
    LINE_STYLE_DOTTED,
} LINE_STYLE;

/**
 * Whether the graphic is filled
**/
typedef enum {
    DRAW_FILL_EMPTY = 0,
    DRAW_FILL_FULL,
} DRAW_FILL;

/**
 * Custom structure of a time attribute
**/
typedef struct {
    UWORD year; //0000
    UBYTE month; //1 - 12
    UBYTE day; //1 - 30
    UBYTE hour; //0 - 23
    UBYTE min; //0 - 59
    UBYTE sec; //0 - 59
} PAINT_TIME;

class GuiPaint {
public:
    GuiPaint();

    void newImage(UBYTE *image, UWORD width, UWORD height, UWORD rotate, UWORD color);
    void selectImage(UBYTE *image);
    void setRotate(UWORD rotate);
    void setMirroring(UBYTE mirror);
    void setPixel(UWORD xPoint, UWORD yPoint, UWORD color) const;
    void setScale(UBYTE scale);
    void clear(UWORD color) const;
    void clearWindows(UWORD xStart, UWORD yStart, UWORD xEnd, UWORD yEnd, UWORD color) const;

    void drawPoint(UWORD xPoint, UWORD yPoint, UWORD color, DOT_PIXEL dotPixel, DOT_STYLE dotFillWay) const;
    void drawLine(UWORD xStart, UWORD yStart, UWORD xEnd, UWORD yEnd, UWORD color, DOT_PIXEL lineWidth, LINE_STYLE lineStyle) const;
    void drawRectangle(UWORD xStart, UWORD yStart, UWORD xEnd, UWORD yEnd, UWORD color, DOT_PIXEL lineWidth, DRAW_FILL drawFill) const;
    void drawCircle(UWORD xCenter, UWORD yCenter, UWORD radius, UWORD color, DOT_PIXEL lineWidth, DRAW_FILL drawFill) const;

    void drawChar(uint16_t xStart, uint16_t yStart, char asciiChar, const sFONT *font, uint16_t colorForeground, uint16_t colorBackground) const;
    void drawString(uint16_t xStart, uint16_t yStart, const String &pString, const sFONT *font, uint16_t colorForeground, uint16_t
                    colorBackground, PaintTextOrientation orientation = TEXT_TL) const;

    void drawString(const String &pString, const uint16_t xStart, const uint16_t yStart, const sFONT *font, const UWORD colorForeground, const PaintTextOrientation orientation = TEXT_TL) const {
        drawString(xStart, yStart, pString, font, colorForeground, colorForeground, orientation);
    }


    void drawNum(uint16_t xPoint, uint16_t yPoint, int32_t number, sFONT *font, uint16_t colorForeground, uint16_t colorBackground) const;
    void drawTime(uint16_t xStart, uint16_t yStart, const PAINT_TIME *pTime, sFONT *font, uint16_t colorForeground, uint16_t
                  colorBackground) const;

    void drawBitMap(const unsigned char *imageBuffer) const;
    void drawBitMapPaste(const unsigned char *imageBuffer, UWORD xStart, UWORD yStart, UWORD imageWidth, UWORD imageHeight, UBYTE flipColor) const;
    void drawImage(const unsigned char *imageBuffer, UWORD xStart, UWORD yStart, UWORD wImage, UWORD hImage) const;

    // Getters
    UBYTE* getImage() const { return _image; }
    UWORD getWidth() const { return _width; }
    UWORD getHeight() const { return _height; }


private:
    UBYTE *_image;
    UWORD _width;
    UWORD _height;
    UWORD _widthMemory;
    UWORD _heightMemory;
    UWORD _color;
    UWORD _rotate;
    UWORD _mirror;
    UWORD _widthByte;
    UWORD _heightByte;
    UWORD _scale;
};


#endif
