#include "animations.h"

void start_animation( animation_function func_ptr, uint8_t occurance, uint8_t repeat, bool reverse ) {
	
	animation.running = true;
	animation.finished = false;
	animation.reverse = reverse;
	animation.animation_ptr = fade_screen;
	animation.step = 0;
	animation.tick = 0;
	animation.occurance = occurance;
	animation.repeat = repeat;
}

void repeat_animation( void ) {
	
	animation.repeat--;
	animation.reverse = !animation.reverse;
	animation.step = 0;
	animation.tick = 0;
}

void stop_animation( void ) {
	
	animation.finished = true;
}

void reset_animation( void ) {
	
	animation.running = false;
	animation.finished = false;
	animation.reverse = false;
	animation.animation_ptr = 0;
	animation.step = 0;
	animation.tick = 0;
	animation.occurance = 0;
	animation.repeat = 0;
}

void fade_screen( void ) {

	uint16_t i;
	uint8_t step;
	uint8_t hexRed, hexGreen, hexBlue;
	uint8_t hexDRed, hexDGreen, hexDBlue;

	/* Get the current step */
	if( animation.reverse ) step = animation.step;
	else step = ( 0x7 ^ animation.step ) - 2;

	if( ( animation.step >= 0 ) && ( animation.step <= 5 ) ) {
		
		/* Alter texture colour palette brightness */
		for( i = 0; i < _texture_colour_palette_length; i++ ) {

			hexRed = ( ( ( _texture_colour_palette[ i ] ) & 0x1F) * 8 );
			hexGreen = ( ( ( _texture_colour_palette[ i ] >> 5 ) & 0x1F) * 8 );
			hexBlue = ( ( ( _texture_colour_palette[ i ] >> 10 ) & 0x1F) * 8 );

			hexDRed = hexRed - ( hexRed * ( 20 * step ) ) / 100;
			hexDGreen = hexGreen - ( hexGreen * ( 20 * step ) ) / 100;
			hexDBlue = hexBlue - ( hexBlue * ( 20 * step ) ) / 100;

			pal_bg_mem[ i ] = rgb( ( hexDRed << 16 ) | ( hexDGreen << 8 ) | ( hexDBlue ) );
		}

		/* Alter sprite colour palette brightness */
		for( i = 0; i < _sprite_colour_palette_length; i++ ) {

			hexRed = ( ( ( _sprite_colour_palette[ i ] ) & 0x1F) * 8 );
			hexGreen = ( ( ( _sprite_colour_palette[ i ] >> 5 ) & 0x1F) * 8 );
			hexBlue = ( ( ( _sprite_colour_palette[ i ] >> 10 ) & 0x1F) * 8 );

			hexDRed = hexRed - ( hexRed * ( 20 * step ) ) / 100;
			hexDGreen = hexGreen - ( hexGreen * ( 20 * step ) ) / 100;
			hexDBlue = hexBlue - ( hexBlue * ( 20 * step ) ) / 100;

			pal_obj_mem[ i ] = rgb( ( hexDRed << 16 ) | ( hexDGreen << 8 ) | ( hexDBlue ) );
		}
			
		/* Advance the animation, unless it's finished */
		if( animation.step == 5 ) {

			/* Repeat animation if needed, otherwise mark as finished */
			if( animation.repeat > 0 )
				repeat_animation();
			else
				stop_animation();

		} else {

			/* Increment animation step */
			animation.step++;
		}
	}
}