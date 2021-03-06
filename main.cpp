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
static std::vector<Image> key;
static std::vector<Image> doorimg;
static std::vector<Image> elemimg;

/* Static loaded map so we can easily share among several subroutines */
static uint8_t mapdata[LEVEL_WIDTH_CELLS][LEVEL_HEIGHT_CELLS] = {0};

void load_map_cells(const uint8_t * map)
{
    /* Just load the cell part of the map into map data
     * Reference algorithm:
     *   byte b = pgm_read_byte(lvl + (x >> 3) + (y * (LEVEL_WIDTH_CELLS >> 3)));
     *   return ((b >> (x % 8)) & 0x01);
     */
    for (size_t y = 0; y < LEVEL_HEIGHT_CELLS; y++)
    {
        for (size_t x = 0; x < LEVEL_WIDTH_CELLS; x++)
        {
            size_t idx = x / 8 + y * LEVEL_WIDTH_CELLS / 8;
            size_t bit = x % 8;
            
            mapdata[x][y] = (map[idx] >> bit) & 1;
        }
    }
}

/* Partially adapted from levels.h */
bool gridGetSolid(int8_t x, int8_t y)
{
    if (x < 0 || x >= LEVEL_WIDTH_CELLS)
        return 1;

    if (y < 0 || y >= LEVEL_HEIGHT_CELLS)
        return 0;

    return mapdata[x][y];
}

/* Adapted from levels.h */
int8_t gridGetTile(int8_t x, int8_t y)
{
    if (!gridGetSolid(x, y)) return 16;

    int8_t l, r, t, b, f;
    l = gridGetSolid(x - 1, y);
    t = gridGetSolid(x, y - 1);
    r = gridGetSolid(x + 1, y);
    b = gridGetSolid(x, y + 1);

    f = 0;
    f = r | (t << 1) | (l << 2) | (b << 3);

    return f;
}

class ObjectPlacer
{
    public:
        ObjectPlacer(const uint8_t * map, size_t & i)
        {
            /* Extract parameters and increment position */
            id = map[i] & 0xE0;
            y = map[i] & 0x1F;
            x = map[i+1] & 0x1F;
            extra = map[i+1] >> 5;
            
            if (id == LFAN)
            {
                extra = map[i+2];
                i += 3;
            }
            else i += 2;
        }
        
        void draw(Image & img)
        {
            switch(id)
            {
            case LCOIN:
                drawCoin(img);
                break;
            case LKEY:
                drawKey(img);
                break;
            case LSTART:
                drawKid(img);
                break;
            case LFINISH:
                drawDoor(img);
                break;
            case LWALKER:
                drawWalker(img);
                break;
            case LFAN:
                drawFan(img);
                break;
            case LSPIKES:
                drawSpikes(img);
                break;
            default:
                break;
            }
        }
        
    protected:
        uint8_t id;
        uint8_t y;
        uint8_t x;
        uint8_t extra;
        
        void drawCoin(Image & img)
        {
            img.composite(elemimg[0], 
                x * LEVEL_CELLSIZE + 3, y * LEVEL_CELLSIZE, 
                AtopCompositeOp);
        }
        
        void drawKey(Image & img)
        {
            img.composite(elemimg[4], 
                x * LEVEL_CELLSIZE + 3, y * LEVEL_CELLSIZE, 
                AtopCompositeOp);
        }
        
        void drawKid(Image & img)
        {
            img.composite(kid[0], 
                x * LEVEL_CELLSIZE + 2, y * LEVEL_CELLSIZE, 
                AtopCompositeOp);
        }
        
        void drawDoor(Image & img)
        {
            img.composite(doorimg[0], 
                x * LEVEL_CELLSIZE, y * LEVEL_CELLSIZE, 
                AtopCompositeOp);
        }
        
        void drawWalker(Image & img)
        {
            img.composite(walker[0], 
                x * LEVEL_CELLSIZE + 4, y * LEVEL_CELLSIZE + 8, 
                AtopCompositeOp);
        }
        
        void drawFan(Image & img)
        {
            /* Default for upwards fans (< 64) */
            size_t imgidx = 0;
            if (extra >= 64 && extra < 192)
            {
                // Right fan
                imgidx = 3;
            }
            else if (extra >= 192)
            {
                // Left fan
                imgidx = 6;
            }
            
            img.composite(fanimg[imgidx], 
                x * LEVEL_CELLSIZE, y * LEVEL_CELLSIZE, 
                AtopCompositeOp);
        }

        /* Some parts of this adapted from enemies.h */
        void drawSpikes(Image & img)
        {
            bool horiz = false;
            size_t dir = 0;
            ssize_t xpix = x * LEVEL_CELLSIZE;
            ssize_t ypix = y * LEVEL_CELLSIZE;
            ssize_t len = 16 * (extra + 1);

            // Solid above
            if (gridGetSolid(x, y - 1))
            {
                horiz = true;
                dir = 3;
            }
            // Solid below
            else if (gridGetSolid(x, y + 1))
            {
                horiz = true;
                ypix += 8;
                dir = 1;
            }
            // Solid left is default, so don't bother checking.
            // Solid right
            else if (gridGetSolid(x + 1, y))
            {
                xpix += 8;
                dir = 2;
            }
            
            if (horiz)
            {
                for (uint8_t xdot = 0; xdot < len; xdot += 8)
                {
                    img.composite(spikes[dir], xpix + xdot, ypix,
                            AtopCompositeOp);
                }
            }
            else
            {
                for (uint8_t ydot = 0; ydot < len; ydot += 8)
                {
                    img.composite(spikes[dir], xpix, ypix + ydot,
                            AtopCompositeOp);
                }
            }




        }
        
};

Image generate_map(const uint8_t * map, size_t length)
{
    /* Image format is a block of tile data, followed by
     * packged information on objects within the map.
     * 
     * Although the map has a scheme ending in 0xff, this
     * function still takes the array length as a parameter for safety */
     
    /* Load the map first, because we will eventually need to compare
     * adjacent tiles when rendering. */
     load_map_cells(map);
    
    /* Generate map image now */
    Image mapimg(Geometry(LEVEL_WIDTH, LEVEL_HEIGHT), Color("white"));
    
    for (size_t y = 0; y < LEVEL_HEIGHT_CELLS; y++)
    {
        for (size_t x = 0; x < LEVEL_WIDTH_CELLS; x++)
        {
            mapimg.composite(tiles[gridGetTile(x, y)], 
                x * LEVEL_CELLSIZE, y * LEVEL_CELLSIZE, 
                AtopCompositeOp);
        }
    }
    
    /* Now overlay objects onto the map image, ignoring the last 0xff position */
    size_t i = LEVEL_CELL_BYTES;
    while (i < length - 1)
    {
        ObjectPlacer obj(map, i);
        obj.draw(mapimg);        
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
    doorimg = load_arduboy(door, sizeof(door));
    elemimg = load_arduboy(elements, sizeof(elements));
    
    /* Re-combine the title screen image */
    std::vector<Image> title = load_arduboy(titleScreen, sizeof(titleScreen));
    Image completeTitle;
    appendImages(&completeTitle, title.begin(), title.end());
    completeTitle.write("title.png");

    /* Save sprites to disk to confirm behaviour */
    writeImages(kid.begin(), kid.end(), "kidSprite.gif");
    writeImages(walker.begin(), walker.end(), "walkerSprite.gif");
    writeImages(spikes.begin(), spikes.end(), "sprSpikes.gif");
    writeImages(fanimg.begin(), fanimg.end(), "fan.gif");
    writeImages(tiles.begin(), tiles.end(), "tileSetTwo.gif");
    writeImages(doorimg.begin(), doorimg.end(), "door.gif");
    writeImages(elemimg.begin(), elemimg.end(), "elements.gif");
    
    /* Generate map images.
     * Note: Not all maps are used (even numbered ones), and some non-numbered ones are
     * used in the primary sequence. */
    generate_map(level1, sizeof(level1)).write("level1.png");
    generate_map(level1old, sizeof(level1old)).write("level1disabled.png");
    generate_map(level2, sizeof(level2)).write("level2.png");
    generate_map(level3, sizeof(level3)).write("level3.png");
    generate_map(level4, sizeof(level4)).write("level4.png");
    generate_map(level5, sizeof(level5)).write("level5.png");
    generate_map(level6, sizeof(level6)).write("level6.png");
    generate_map(level7, sizeof(level7)).write("level7.png");
    generate_map(level8, sizeof(level8)).write("level8.png");
    generate_map(level9, sizeof(level9)).write("level9.png");
    generate_map(level10, sizeof(level10)).write("level10.png");
    generate_map(jace, sizeof(jace)).write("jace.png");
    generate_map(testhfan, sizeof(testhfan)).write("testhfan.png");
    generate_map(level11hard, sizeof(level11hard)).write("level11hard.png");
    generate_map(level11, sizeof(level11)).write("level11.png"); // hard mode level 11
    generate_map(level12, sizeof(level12)).write("level12.png");
    generate_map(level13, sizeof(level13)).write("level13.png");
    generate_map(level14, sizeof(level14)).write("level14.png");
    generate_map(level15, sizeof(level15)).write("level15.png");
    generate_map(level16, sizeof(level16)).write("level16.png");
    generate_map(level17, sizeof(level17)).write("level17.png");
    generate_map(level18, sizeof(level18)).write("level18.png");
    generate_map(level19, sizeof(level19)).write("level19.png");
    generate_map(level20, sizeof(level20)).write("level20.png");
    generate_map(level21, sizeof(level21)).write("level21.png");
    generate_map(level22, sizeof(level22)).write("level22.png");
    generate_map(level23, sizeof(level23)).write("level23.png");
    generate_map(level24, sizeof(level24)).write("level24.png");
    generate_map(level25, sizeof(level25)).write("level25.png");
    generate_map(level26, sizeof(level26)).write("level26.png");
    generate_map(level27, sizeof(level27)).write("level27.png");
    generate_map(level28, sizeof(level28)).write("level28.png");
    generate_map(level29, sizeof(level29)).write("level29.png");
    generate_map(level30, sizeof(level30)).write("level30.png");
    generate_map(level31, sizeof(level31)).write("level31.png");
    generate_map(level32, sizeof(level32)).write("level32.png");
    generate_map(level33, sizeof(level33)).write("level33.png");
    generate_map(level34, sizeof(level34)).write("level34.png");
    generate_map(level35, sizeof(level35)).write("level35.png");
    generate_map(level36, sizeof(level36)).write("level36.png");
    generate_map(level37, sizeof(level37)).write("level37.png");
    generate_map(level38, sizeof(level38)).write("level38.png");
    generate_map(level39, sizeof(level39)).write("level39.png");
    generate_map(level40, sizeof(level40)).write("level40.png");
    generate_map(levelNarrowWalls, sizeof(levelNarrowWalls)).write("levelNarrowWalls.png");

    return 0;
}
