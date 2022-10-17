#ifndef ANIMATIONS_H_INCLUDED
#define ANIMATIONS_H_INCLUDED

#include "project.h"

extern struct player_struct player;
extern struct animation_settings animation;

extern const uint16_t _sprite_colour_palette_length;
extern const uint16_t _sprite_colour_palette[];
extern const uint16_t _texture_colour_palette_length;
extern const uint16_t _texture_colour_palette[];

void start_animation( animation_function func_ptr, uint8_t occurance, uint8_t repeat, bool reverse );
void repeat_animation( void );
void stop_animation( void );
void reset_animation( void );

void fade_screen( void );

#endif