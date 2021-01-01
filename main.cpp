#include <Magick++.h>
#include <vector>
#include <cstdint>
#include <iostream>

using namespace Magick;

// Dummy definitions so I don't need to strip them off or convert them
#define PROGMEM
typedef uint8_t byte;

#include "bitmaps.h"
#include "globals.h"

/* Some useful information:
 * Refer to the following for API help:
 *   http://www.imagemagick.org/Magick++/Documentation.html
 * 
 * Also, Team ARG has vanished, so there is some discussion on the
 * archive and license for their games:
 * 
 *   https://community.arduboy.com/t/team-arg-disappeared-how-to-get-their-games/8891
 */

/** Loads a single frame from an Arduboy multi-frame sprite. */
Image load_arduboy_frame(const uint8_t * imgreg, size_t width, size_t height, bool masked=false)
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
                    if (masked) workmem[outidx] = 0x00;
                    else        workmem[outidx] = 0xFF;
                }
                else
                {
                    if (masked) workmem[outidx] = 0xFF;
                    else        workmem[outidx] = 0x00;
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

/** Loads a full multi-frame Arduboy sprite as a list of images */
std::vector<Image> load_arduboy(const uint8_t * data, size_t length, bool masked=false)
{
    const size_t width = data[0];
    const size_t height = data[1];
    const uint8_t * imgreg = data + 2;
    const size_t frame_size = (width * height) / 8;
    const size_t data_length = length - 2;
    const size_t num_frames = data_length / frame_size;
    
    std::vector<Image> result;
    
    for (size_t i = 0; i < num_frames; i++)
    {
        const uint8_t * framereg = imgreg + frame_size * i;
        result.push_back(load_arduboy_frame(framereg, width, height, masked));
    }

    return result;
}

/* Statically store key images to use when generating maps.
 * These will be assigned after init */
static std::vector<Image> tiles;
static std::vector<Image> kid;
static std::vector<Image> walker;
static std::vector<Image> fanimg;
static std::vector<Image> spikes;

Image generate_map(const uint8_t * map, size_t length)
{
    /* Image format is a block of tile data, followed by
     * packged information on objects within the map.
     * 
     * Although the map has a scheme ending in 0xff, this
     * function still takes the array length as a parameter for safety */
     
    /* Load the map first, because we will eventually need to compare
     * adjacent tiles when rendering. 
     * 
     * Reference algorithm:
     *   byte b = pgm_read_byte(lvl + (x >> 3) + (y * (LEVEL_WIDTH_CELLS >> 3)));
     *   return ((b >> (x % 8)) & 0x01);
     * */
       
    uint8_t mapdata[LEVEL_WIDTH_CELLS][LEVEL_HEIGHT_CELLS] = {0};
    
    for (size_t y = 0; y < LEVEL_HEIGHT_CELLS; y++)
    {
        for (size_t x = 0; x < LEVEL_WIDTH_CELLS; x++)
        {
            size_t idx = x / 8 + y * LEVEL_WIDTH_CELLS / 8;
            size_t bit = x % 8;
            
            mapdata[x][y] = (map[idx] >> bit) & 1;
        }
    }
    
    /* Generate map image now */
    Image mapimg(Geometry(LEVEL_WIDTH, LEVEL_HEIGHT), Color("white"));
    
    for (size_t y = 0; y < LEVEL_HEIGHT_CELLS; y++)
    {
        for (size_t x = 0; x < LEVEL_WIDTH_CELLS; x++)
        {
            if (mapdata[x][y])
            {
                mapimg.composite(tiles[15], 
                    x * LEVEL_CELLSIZE, y * LEVEL_CELLSIZE, 
                    AtopCompositeOp);
            }
            else
            {
                mapimg.composite(tiles[16], 
                    x * LEVEL_CELLSIZE, y * LEVEL_CELLSIZE, 
                    AtopCompositeOp);
            }
        }
    }
    
    
    return mapimg;
}


int main(int argc,char **argv)
{ 
    InitializeMagick(*argv);
    
    /* Load static sprites now */
    tiles = load_arduboy(tileSetTwo, sizeof(tileSetTwo));
    kid = load_arduboy(kidSprite, sizeof(kidSprite), true);
    walker = load_arduboy(walkerSprite, sizeof(walkerSprite));
    fanimg = load_arduboy(fan, sizeof(fan));
    spikes = load_arduboy(sprSpikes, sizeof(sprSpikes));
    
    /* TODO: Re-combine the title screen image? */
    std::vector<Image> title = load_arduboy(titleScreen, sizeof(titleScreen));
    writeImages(title.begin(), title.end(), "titleScreen.gif");

    /* Save sprites to disk to confirm behaviour */
    writeImages(kid.begin(), kid.end(), "kidSprite.gif");
    writeImages(walker.begin(), walker.end(), "walkerSprite.gif");
    writeImages(spikes.begin(), spikes.end(), "sprSpikes.gif");
    writeImages(fanimg.begin(), fanimg.end(), "fan.gif");
    writeImages(tiles.begin(), tiles.end(), "tileSetTwo.gif");
    
    /* Generate image for map 1 */
    generate_map(level1, sizeof(level1)).write("level1.png");
    generate_map(level2, sizeof(level2)).write("level2.png");

    return 0;
}
