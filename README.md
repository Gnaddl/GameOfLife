# Conway's Game of Life
This Arduino sketch demonstrates John Conway's Game of Life on a monochrome 128x64 pixel graphic display. For a more detailed description of the game see http://en.wikipedia.org/wiki/Conway%27s_Game_of_Life

## Memory Handling
Memory management can be quite tricky if you have to deal with the restricted RAM size of an 8-bit microcontroller like the Atmel ATmega328P on an Arduino. The bitmap for a population of 128 by 64 cells requires 128 * 64 / 8 = 1024 bytes of RAM. Since the next cell generation is calculated from the previous generation, the program normally needs 2 kBytes, which is the whole RAM of the ATmega328P. Therefore, keeping both generations in RAM at the same time is not possible on an Arduino.

To cope with the restrictions of the RAM size we use a simple trick. The calculation is done "in place" by using additional 3 lines for the bitmap. The current population is shifted up by 3 lines, then the next generation is calculated line by line and overwrites the part of the previous generation, which is no longer needed. This trick reduces the needed RAM to 128 * (64 + 3) / 8 = 1072 bytes. Have a look at the source code for more information.

## Appreciation
Special thanks to the people working on the excellent **u8glib** (Universal Graphics Library for 8 bit Embedded Systems). See https://github.com/olikraus/u8glib
