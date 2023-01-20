#include "text.h"

/* Textbox variables */
bool textbox_running, textbox_update;
uint8_t scroll_arrow_count, scroll_arrow_offset, total_textbox_lines, textbox_line;
uint8_t write_text_line_count, write_text_char_count;
char (*text_ptr)[29], write_text[100];

void write_character( char character, uint16_t x, uint16_t y ) {

	se_mem[block_text_layer][ x + ( y * 32 ) ] = ( character - 31 );
}

void write_string( char *char_array, int32_t x, int32_t y, uint16_t char_offset ) {
	uint8_t i = 0;
	int32_t _x = x, _y = y;
	
	while( ( char_array[i] != '\0' ) && ( ( ( i < char_offset ) && ( char_offset != 0 ) ) || ( char_offset == 0 ) ) )
	{
		write_character( char_array[i], ( _x + i ), _y );
		i++;
	}
}

void draw_textbox( void ) {

	uint8_t box_offset, i, j;
	
	if( textbox_running ) {

		if( textbox_update ) {
			if( total_textbox_lines < 3 )
				/* 2 line textbox */
				box_offset = 2;
			else
				/* 3 line textbox */
				box_offset = 0;

			/* Draw textbox corners */
			set_tile( 0, ( ( total_textbox_lines < 3 ) ? 15 : 13 ), 97, 0, 0, 19 );
			set_tile( 29, ( ( total_textbox_lines < 3 ) ? 15 : 13 ), 97, 1, 0, 19 );
			set_tile( 0, 19, 99, 0, 0, 19 );
			set_tile( 29, 19, 99, 1, 0, 19 );

			/* Draw textbox horizontal borders and filling */
			for( i = 1; i < 29; i++ ) {
				set_tile( i, ( ( total_textbox_lines < 3 ) ? 15 : 13 ), 98, 0, 0, 19 );
				set_tile( i, 19, 100, 0, 0, 19 );

				if( total_textbox_lines >= 3 ) {
					set_tile( i, 14, 1, 0, 1, 19 );
					set_tile( i, 15, 1, 0, 1, 19 );
				}
				set_tile( i, 16, 1, 0, 1, 19 );
				set_tile( i, 17, 1, 0, 1, 19 );
				set_tile( i, 18, 1, 0, 1, 19 );
			}

			/* Draw textbox vertical borders */
			for( i = ( ( total_textbox_lines < 3 ) ? 16 : 14 ); i < 19; i++ ) {
				set_tile( 0, i, 101, 0, 0, 19 );
				set_tile( 29, i, 101, 1, 0, 19 );
			}

			/* Draw up to 3 lines of text */
			if( textbox_line < total_textbox_lines ) {
				
				sprintf( write_text, text_ptr[textbox_line] );
				if( write_text_line_count >= 0 )
					write_string( write_text, 1, ( 14 + box_offset ), ( ( write_text_line_count == 0 ) ? write_text_char_count : 0 ) );
			}

			if( ( textbox_line + 1 ) < total_textbox_lines ) {
				
				sprintf( write_text, text_ptr[textbox_line + 1] );
				if( write_text_line_count >= 1 )
					write_string( write_text, 1, ( 16 + box_offset ), ( ( write_text_line_count == 1 ) ? write_text_char_count : 0 ) );
			}

			if( ( textbox_line + 2 ) < total_textbox_lines ) {
				
				sprintf( write_text, text_ptr[textbox_line + 2] );
				if( write_text_line_count >= 2 )
					write_string( write_text, 1, 18, ( ( write_text_line_count == 2 ) ? write_text_char_count : 0 ) );
			}

			if( write_text_char_count < 29 )
				write_text_char_count++;

			if( ( ( write_text_char_count >= 29 ) || ( text_ptr[write_text_line_count][write_text_char_count] == '\0' ) ) && ( write_text_line_count < ( ( total_textbox_lines < 3 ) ? ( total_textbox_lines - 1 ) : 2 ) ) ) {
				write_text_char_count = 1;
				write_text_line_count++;
			}

			if( ( write_text_line_count == 2 ) && ( write_text_char_count >= 29 ) ) {
				textbox_update = false;
			}
		}

		/* Draw a scroll arrow if needed */
		if( ( textbox_line + 3 ) < total_textbox_lines ) {
			set_tile( 28, 18, ( 102 + scroll_arrow_offset ), 0, 0, 19 );
		}

		/* Animate the scroll arrow */
		scroll_arrow_count++;
		if( scroll_arrow_count > 10 ) {
			scroll_arrow_count = 0;

			scroll_arrow_offset++;
			if( scroll_arrow_offset > 2 ) scroll_arrow_offset = 0;
		}

		/* Advance textbox when either button A or B are pressed */
		if( ( ( key_down( KEY_A ) ) || ( key_down( KEY_B ) ) ) && ( !textbox_update ) ) {

			if( ( textbox_line + 3 ) < total_textbox_lines ) {
				
				/* Move to the next line and type out only the last line */
				textbox_line++;

				write_text_char_count = 1;

				textbox_update = true;
			} else {

				/* Close the textbox */
				for( i = 0; i < 30; i++ ) {
					for( j = 13; j < 20; j++ ) {
						set_tile( i, j, 0, 0, 0, 19 );
					}
				}

				textbox_running = false;

				/* Add delay so the user doesn't immediately restart the interaction */
				ticks.interaction_delay = 50;
			}
		}
	}
}

void open_textbox( char (*_text_ptr)[29], uint8_t _total_lines ) {

	/* Set the pointer to the text to display and set the number of lines */
	text_ptr = _text_ptr;
	total_textbox_lines = _total_lines;

	/* Reset all variables */
	scroll_arrow_count = 0;
	scroll_arrow_offset = 0;
	textbox_line = 0;

	write_text_line_count = 0;
	write_text_char_count = 1;

	/* Stop the player's movement */
	player.walk_dir.x = 0;
	player.walk_dir.y = 0;

	/* Set textbox to active */
	textbox_running = true;
	textbox_update = true;
}