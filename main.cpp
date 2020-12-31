#include <Magick++.h>
#include <list>
#include <cstdint>
#include <iostream>

using namespace Magick;

// Dummy definitions so I don't need to strip them off or convert them
#define PROGMEM
typedef uint8_t byte;

#include "bitmaps.h"

Image load_arduboy_frame(const uint8_t * imgreg, size_t width, size_t height)
{
    size_t imglength = width * height;
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

std::list<Image> load_arduboy(const uint8_t * data, size_t length)
{
    const size_t width = data[0];
    const size_t height = data[1];
    const uint8_t * imgreg = data + 2;
    const size_t frame_size = (width * height) / 8;
    const size_t data_length = length - 2;
    const size_t num_frames = data_length / frame_size;
    
    std::list<Image> result;
    
    for (size_t i = 0; i < num_frames; i++)
    {
        const uint8_t * framereg = imgreg + frame_size * i;
        result.push_back(load_arduboy_frame(framereg, width, height));
    }

    return result;
}

// http://www.imagemagick.org/Magick++/Documentation.html for help

int main(int argc,char **argv)
{ 
    InitializeMagick(*argv);
    
    std::list<Image> title = load_arduboy(titleScreen, sizeof(titleScreen));
    writeImages(title.begin(), title.end(), "titleScreen.gif");

    std::list<Image> kid = load_arduboy(kidSprite, sizeof(kidSprite));
    writeImages(kid.begin(), kid.end(), "kidSprite.gif");

    std::list<Image> walker = load_arduboy(walkerSprite, sizeof(walkerSprite));
    writeImages(walker.begin(), walker.end(), "walkerSprite.gif");

    std::list<Image> spikes = load_arduboy(sprSpikes, sizeof(sprSpikes));
    writeImages(spikes.begin(), spikes.end(), "sprSpikes.gif");

    std::list<Image> fanimg = load_arduboy(fan, sizeof(fan));
    writeImages(fanimg.begin(), fanimg.end(), "fan.gif");

    std::list<Image> tiles = load_arduboy(tileSetTwo, sizeof(tileSetTwo));
    writeImages(tiles.begin(), tiles.end(), "tileSetTwo.gif");

    return 0;
}
