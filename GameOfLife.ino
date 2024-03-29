/**
 *  This Arduino sketch demonstrates John Conway's Game of Life on a monochrome
 *  128x64 pixel graphic display. For a more detailed description of the game see
 *  http://en.wikipedia.org/wiki/Conway%27s_Game_of_Life
 *
 *  We use a simple trick to cope with the restricted RAM of the Arduino. The
 *  calculation is done "in place" by using additional 3 lines for the bitmap.
 *  The current population is shifted up by 3 lines, then the next generation
 *  is calculated and overwrites the part of the previous generation, which is
 *  no longer needed.
 *
 *  Thanks to the people working on the excellent u8g2 library (Universal Graphics
 *  Library for 8 bit Embedded Systems).
 *  See https://github.com/olikraus/u8g2
 *
 *  Author:      Gnaddl
 *  Date:        03-AUG-2013
 *  Last change: 08-JAN-2023
 */

#include "Arduino.h"
#include "U8g2lib.h"

// Remove the comment from the following #define if you want the coordinates to wrap around.
//#define USE_TORUS

#define DISPLAY_X      128    // Display resolution horizontally
#define DISPLAY_Y      64     // Display resolution vertically

#define MAX_X          128    // size of the cell area; both values must be a power of 2
#define MAX_Y          64

#define Y_SHIFT        3      // Number of pixel lines to shift up before calculating the next cell generation
#define SHIFT_BYTES    ((Y_SHIFT) * (MAX_X)/8)


/**
 * A few predefined patterns for experimenting; found in the Wikipedia article.
 */

 // Glider Gun
static const uint8_t glidergun[] =
{
  5, 9, 8, 6,
  0b00000000, 0b00000000, 0b00000000, 0b01000000, 0b00000000,
  0b00000000, 0b00000000, 0b00000001, 0b01000000, 0b00000000,
  0b00000000, 0b00000110, 0b00000110, 0b00000000, 0b00011000,
  0b00000000, 0b00001000, 0b10000110, 0b00000000, 0b00011000,
  0b01100000, 0b00010000, 0b01000110, 0b00000000, 0b00000000,
  0b01100000, 0b00010001, 0b01100001, 0b01000000, 0b00000000,
  0b00000000, 0b00010000, 0b01000000, 0b01000000, 0b00000000,
  0b00000000, 0b00001000, 0b10000000, 0b00000000, 0b00000000,
  0b00000000, 0b00000110, 0b00000000, 0b00000000, 0b00000000
};


static const uint8_t gun2[] =
{
  1, 5, 32, 32,
  0b01110100,
  0b01000000,
  0b00001100,
  0b00110100,
  0b01010100
};


static const uint8_t gun3[] =
{
  5, 1, 32, 32,
  0b01111111, 0b10111110, 0b00111000, 0b00011111, 0b11011111
};



/**
 * Setup u8g2 object for the graphic display with 180 degree rotation.
 * Use another constructor if you have a different display type connected to your Arduino.
 */
U8G2_SSD1306_128X64_NONAME_2_HW_I2C glcd (U8G2_R2);  // Fast I2C / TWI

static uint8_t population[MAX_X/8 * (MAX_Y + Y_SHIFT)];
static int generation = 0;


// "Torus" functions for coordinate wrap-around.
inline uint8_t torx (uint8_t x) { return (x + MAX_X) & (MAX_X - 1); }
inline uint8_t tory (uint8_t y) { return (y + MAX_Y) & (MAX_Y - 1); }


// Note: the following functions assume that the x and y coordinates are always
//       in the allowed range. No further checks are done.
inline void setCell (uint8_t x, uint8_t y)
{
  population[(x >> 3) + y * MAX_X/8] |= 0x80 >> (x & 0x07);
}


inline void clearCell (uint8_t x, uint8_t y)
{
  population[(x >> 3) + y * MAX_X/8] &= ~(0x80 >> (x & 0x07));
}


inline bool isCellAlive (uint8_t x, uint8_t y)
{
  return population[(x >> 3) + (y + Y_SHIFT) * MAX_X/8] & (0x80 >> (x & 0x07));
}


/**
 *  Initialize the population randomly.
 */
void init_population_random (unsigned int count)
{
  // clear the population
  memset (population, sizeof (population), 0);

  // create a new pattern
  for (; count > 0; count--)
  {
    int x, y;

    x = random (MAX_X - 2) + 1;
    y = random (MAX_Y - 2) + 1;
    
    setCell (x, y);
  }
}


/**
 *  Initialize the population by a predefined bitmap.
 */
void init_population_pattern (const uint8_t *p)
{
  uint8_t x, y, xoffset, yoffset;
  uint8_t xmax, ymax;

  // clear the population
  memset (population, sizeof (population), 0);

  // get the number of horizontal bytes and the number of lines
  xmax = *p++;
  ymax = *p++;
  xoffset = *p++;
  yoffset = *p++;

  // set the cells from the bitmap
  for (y = 0; y < ymax; y++)
    for (x = 0; x < xmax; x++, p++)
      for (uint8_t theBit = 0; theBit < 8; theBit++)
        if (*p & (0x80 >> theBit))
          setCell (x * 8 + theBit + xoffset, y + yoffset);
}


/**
 *  Calculate the next generation of cells by Conway's algorithm.
 */
void next_generation (void)
{
  uint8_t x, y, neighbors;

  // Shift the current population 2 lines down
  memmove (population + SHIFT_BYTES, population, sizeof (population) - SHIFT_BYTES);

#ifdef USE_TORUS
  for (y = 0; y < MAX_Y; y++)
    for (x = 0; x < MAX_X; x++)
    {
      neighbors = 0;

      if (isCellAlive (torx(x-1), tory(y-1))) neighbors++;
      if (isCellAlive (x,         tory(y-1))) neighbors++;
      if (isCellAlive (torx(x+1), tory(y-1))) neighbors++;
      if (isCellAlive (torx(x-1), y        )) neighbors++;
      if (isCellAlive (torx(x+1), y        )) neighbors++;
      if (isCellAlive (torx(x-1), tory(y+1))) neighbors++;
      if (isCellAlive (x,         tory(y+1))) neighbors++;
      if (isCellAlive (torx(x+1), tory(y+1))) neighbors++;
#else
  for (y = 1; y < MAX_Y - 1; y++)
    for (x = 1; x < MAX_X - 1; x++)
    {
      neighbors = 0;

      if (isCellAlive (x-1, y-1)) neighbors++;
      if (isCellAlive (x,   y-1)) neighbors++;
      if (isCellAlive (x+1, y-1)) neighbors++;
      if (isCellAlive (x-1, y  )) neighbors++;
      if (isCellAlive (x+1, y  )) neighbors++;
      if (isCellAlive (x-1, y+1)) neighbors++;
      if (isCellAlive (x,   y+1)) neighbors++;
      if (isCellAlive (x+1, y+1)) neighbors++;
#endif

      if (isCellAlive (x, y))
      {
        // Cell is alive; remains alive with 2 or 3 neighbors.
        if ((neighbors == 2) || (neighbors == 3))
          setCell (x, y);
        else
          clearCell (x, y);
      }
      else
      {
        // Cell is dead; new cell is born when it has exactly 3 neighbors.
        if (neighbors == 3)
          setCell (x, y);
        else
          clearCell (x, y);
      }
    }
}


/**
 *  Draw the populaton and display the number of the generation.
 *  Can be called more than once from the picture loop.
 */
void draw (void)
{
  char buffer[8];

  // draw the population bitmap
  glcd.setColorIndex (1);
  glcd.drawBitmap (0, 0, MAX_X/8, MAX_Y, population);

  // show the generation number in the top right corner
  sprintf (buffer, "%3d", generation);
  glcd.setFont (u8g_font_5x8);
  glcd.drawStr (112, 8, buffer);
}


void setup(void)
{
#ifdef DEBUG
  Serial.begin (115200);

  fdev_setup_stream (&uartout, uart_putchar, NULL, _FDEV_SETUP_WRITE);
  stdout = &uartout;
#endif

  glcd.begin();
  glcd.setFont (u8g2_font_helvB10_tf);

  glcd.firstPage();  
  do
  {
    glcd.drawStr (10, 26, "John Conway's");
    glcd.drawStr (16, 42, "Game of Life");
  } while (glcd.nextPage());
  delay (2000);

  init_population_pattern (glidergun);      // start with a Glider Gun pattern
//  init_population_pattern (gun2);
//  init_population_pattern (gun3);
}


void loop(void)
{
  glcd.firstPage();  
  do
  {
    draw();
  } while (glcd.nextPage());

  if (++generation < 100)
  {
    // calculate the next generation
    next_generation ();
  }
  else
  {
    // After 100 generations create a new random pattern.
    init_population_random (MAX_X * MAX_Y / 8);
    generation = 0;
  }
}
