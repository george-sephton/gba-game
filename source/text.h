#ifndef TEXT_H_INCLUDED
#define TEXT_H_INCLUDED

// Include assets
#include "project.h"

/* Text functions */
void write_character( char character, uint16_t x, uint16_t y );
void write_string( char *char_array, int32_t x, int32_t y, uint16_t char_offset );

#endif