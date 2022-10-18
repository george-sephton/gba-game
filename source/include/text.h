#ifndef TEXT_H_INCLUDED
#define TEXT_H_INCLUDED

#include "project.h"
#include "keys.h"

extern struct game_ticks ticks;
extern struct player_struct player;

/* Textbox variables */
extern bool textbox_running, textbox_update;
extern uint8_t scroll_arrow_count, scroll_arrow_offset, total_textbox_lines, textbox_line;
extern uint8_t write_text_line_count, write_text_char_count;
extern char (*text_ptr)[29], write_text[100];

/* Text functions */
void write_character( char character, uint16_t x, uint16_t y );
void write_string( char *char_array, int32_t x, int32_t y, uint16_t char_offset );

void draw_textbox( void );
void open_textbox( char (*_text_ptr)[29], uint8_t _total_lines );

#endif