#include "text.h"

void write_character( char character, uint16_t x, uint16_t y ) {

	se_mem[19][ x + ( y * 32 ) ] = ( character - 31 );
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