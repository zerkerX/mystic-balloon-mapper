#include <Magick++.h>
#include <cstdint>
#include <iostream>

using namespace Magick;

// Dummy definitions so I don't need to strip them off or convert them
#define PROGMEM
typedef uint8_t byte;

#include "bitmaps.h"

Image load_arduboy(const unsigned char * data, size_t length)
{
    int width = data[0];
    int height = data[1];
    size_t imglength = width * height;
    const uint8_t * imgreg = data + 2;
    
    std::cout << width << "x" << height << " from " << (length - 2) << " to " << imglength << std::endl;
    
    uint8_t * workmem = new uint8_t[imglength];
    for (size_t x = 0; x < width; x++)
    {
        for (size_t ybyte = 0; ybyte < height / 8; ybyte++)
        {
            uint8_t inbyte = imgreg[width * ybyte + x];
            for (size_t ybit = 0; ybit < 8; ybit++)
            {
                size_t outidx = width * (ybyte * 8 + ybit) + x;
                if (inbyte & 1)
                {
                    workmem[outidx] = 0x00;
                }
                else
                {
                    workmem[outidx] = 0xFF;
                }
                inbyte = inbyte >> 1;
            }
        }
    }

    Blob dblob(workmem, imglength);
    delete[] workmem;

    Image img(dblob, Geometry(width, height), 8, "GRAY");
    return img;
}

// http://www.imagemagick.org/Magick++/Documentation.html for help

int main(int argc,char **argv)
{ 
    InitializeMagick(*argv);
    
    load_arduboy(T_arg, sizeof(T_arg)).write("T_Arg.png");
    load_arduboy(badgeMysticBalloon, sizeof(badgeMysticBalloon)).write("badgeMysticBalloon.png");
    load_arduboy(qrcode, sizeof(qrcode)).write("qrcode.png");
    load_arduboy(titleScreen, sizeof(titleScreen)).write("titleScreen.png");
    load_arduboy(kidSprite, sizeof(kidSprite)).write("kidSprite.png");

    return 0;
}
