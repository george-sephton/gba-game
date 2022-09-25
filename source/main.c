#include "project.h"

#include "demo_project.h"
#include "font.h"
#include "keys.h"
#include "sprites.h"
#include "text.h"

#include "tonc_utils.h"
#include "tonc_irq.h"


/*********************************************************************************
	Helpful Functions
*********************************************************************************/
/* Sets a tile value from screenblock at x, y */
static inline void set_tile( uint16_t x, uint16_t y, uint16_t tile, uint8_t screenblock ) {
	se_mem[ screenblock ][ x + ( y * 32 ) ] = tile;
}

/* Converts 32-bit RGB to 15-bit BGR */
static inline uint16_t rgb(uint32_t colour) {
	return 0x0 | ( ( ( ( ( colour >> 16 ) & 0xFF) / 8 ) << 0 ) | ( ( ( ( colour >> 8 ) & 0xFF) / 8 ) << 5 ) | ( ( ( colour & 0xFF) / 8 ) << 10 ) );
}

/*********************************************************************************
	Global Variables
*********************************************************************************/
uint16_t __key_curr, __key_prev;
struct REPEAT_REC __key_rpt = { 0, KEY_MASK, 60, 60, 30 };

/* Map */
struct map *_current_map;

/* Player movement variables */
struct dir_vec map_pos;
struct offset_vec _screen_offsets;

char write_text[128];
OBJ_ATTR obj_buffer[128];

/*********************************************************************************
	Timer Variables
*********************************************************************************/
uint16_t timer_count;
uint32_t game_tick;

/* Timer IRQ Prototypes */
__attribute__((section(".iwram"), long_call)) void isr_master();
__attribute__((section(".iwram"), long_call)) void timr0_callback();

void timr0_callback()
{
	timer_count++;
}

const fnptr master_isrs[2]= 
{
	(fnptr)isr_master,
	(fnptr)timr0_callback 
};

/*********************************************************************************
	Game Functions
*********************************************************************************/
void debug( void ) {
	
	/* Debug */
	sprintf( write_text, "012345678901234567890123456789" );
	write_string( write_text, 0, 0, 0 );
	sprintf( write_text, "C(%d)", (int)game_tick );
	write_string( write_text, 0, 1, 0 );

	//sprintf( write_text, "P(%d,%d)O(%d,%d,%d,%d)", map_pos.x, map_pos.y, _screen_offsets.n, _screen_offsets.e, _screen_offsets.s, _screen_offsets.w );
	//write_string( write_text, 0, 1, 0 );
	//sprintf( write_text, "B(%d,%d,%d,%d)  ", (int)key_held(KEY_UP), (int)key_held(KEY_LEFT), (int)key_held(KEY_DOWN), (int)key_held(KEY_RIGHT) );
	//write_string( write_text, 0, 2, 0 );
	
	uint8_t i;
	for( i = 0; i < 10; i++ ) {
		write_character( ( 48 + i ), 0, i );
		write_character( ( 48 + i ), 0, ( i + 10 ) );
	}
}

void update_map( void ) {

	int16_t _draw_x, _draw_y;

	/* Calculate how many tiles in each direction we need to draw */
	_screen_offsets = (struct offset_vec){ 0, 0, 0, 0 };

	if( map_pos.x < 15 ) {
		_screen_offsets.w = ( 15 - map_pos.x );
	}

	if( ( map_pos.x + 17 ) > _current_map->map_width ) {
		_screen_offsets.e = ( map_pos.x + 17 - _current_map->map_width );
	}

	if( map_pos.y < 11 ) {
		_screen_offsets.n = ( 10 - map_pos.y );
	}

	if( ( map_pos.y + 12 ) > _current_map->map_height ) {
		_screen_offsets.s = ( map_pos.y + 12 - _current_map->map_height );
	}

	/* Clear background map at screenblocks 16 and 17 (memory address 0x0600:8000 and 0x0600:8800) */
	toncset16( &se_mem[16][0], 0, 4096 );

	// Get the current map tile pointer, starting at (0, 0)
  	const struct map_tile (*map_tiles_ptr) = _current_map->map_tiles_ptr;

  	// Next let's move the map pointer to the column first tile to draw
	if( _screen_offsets.w == 0 )
		map_tiles_ptr += ( map_pos.x - 10 );
	// If there's a gap between the left side of the display and the start of the drawn map, we don't need to move the pointer
	
	// and now move the pointer to the row of the first tile to draw
	if( _screen_offsets.n == 0 )
		 map_tiles_ptr += ((map_pos.y - 7) * _current_map->map_width );
	// If there's a gap between the top of the display and the start of the drawn map, we don't need to move the pointer

	// Draw the map tiles, start by looping through the 20 rows of the display (y-direction)
	for( _draw_y=0; _draw_y < 22; _draw_y++ ) {
		
		// Only draw if there's map tiles to be drawn
		if( ( _draw_y >= _screen_offsets.n ) && ( _draw_y < ( 22 - _screen_offsets.s ) ) ) {
			
			// Next loop through the 30 columns of the display (x-direction)
			for( _draw_x=0; _draw_x < 32; _draw_x++ ) {

				// Again, make sure we're drawing the map titles in the correct places
				if( ( _draw_x >= _screen_offsets.w ) && ( ( _draw_x ) <= ( 31 - _screen_offsets.e ) ) ) {
					
					//First draw the background sprite, if any
					//if( _current_map->bg_texture != -1 )
					set_tile( _draw_x, _draw_y, 0x1, 16 );
						//draw_sprite((uint16_t*)_texture_map[_current_map->bg_sprite], _current_map->bg_sprite_offset, 0, 0, ((_draw_x * 8) - _scroll_x), ((_draw_y * 8) + _scroll_y), 8);

					//Make sure there's a sprite to draw
					//if( ( (*map_tiles_ptr).sprite != -1 ) && ( (*map_tiles_ptr).top_layer == false) )
					//	draw_sprite((uint16_t*)_texture_map[(*map_tiles_ptr).sprite], (*map_tiles_ptr).sprite_offset, (*map_tiles_ptr).sprite_reverse_x, (*map_tiles_ptr).sprite_reverse_y, ((_draw_x * 8) - _scroll_x), ((_draw_y * 8) + _scroll_y), 8);

					// If we've drawn a tile, increment the pointer (unless we're at the last column of the display, don't want to accidentally cause a hard fault)
					if(_draw_x != 14) map_tiles_ptr++;
				}
			}
			// Increment the pointer as we move to the next row on the dislay, but don't move the pointers if the map width fits entirely on the display
			if( ( _screen_offsets.e == 0 ) || ( _screen_offsets.w == 0 ) ) {
				// Also if we're drawing the last row, then don't move the pointer either
				if( _draw_y != 14 ) map_tiles_ptr += ( _current_map->map_width - 14 + _screen_offsets.w + ( ( _screen_offsets.e > 1) ? ( _screen_offsets.e - 1 ) : 0 ) );
			}
		}
	}
}



int main()
{

	/*********************************************************************************
		Copy data to VRAM
	*********************************************************************************/
	/* Load texture colour palette */
	memcpy( pal_bg_mem, _texture_colour_palette, sizeof( _texture_colour_palette ) );
	/* Load sprite colour palette */
    memcpy( pal_obj_mem, _sprite_colour_palette, sizeof( _sprite_colour_palette ) );

	/* Load font into VRAM charblock 0 (memory address 0x0600:0000) */
	memcpy( &tile_mem[0][0], _font_map, sizeof( _font_map ) );
	/* Load tileset into VRAM charblock 1 (memory address 0x0600:4000) */
	memcpy( &tile_mem[1][0], _texture_map, sizeof( _texture_map ) );
	/* Load sprites into VRAM charblock 4 (memory address 0x0601:0000) */
    memcpy( &tile_mem[4][0], _sprite_map, sizeof( _sprite_map ) );

	/* Clear background map at screenblocks 16 and 17 (memory address 0x0600:8000 and 0x0600:8800) */
	toncset16( &se_mem[16][0], 0, 4096 );
	/* Clear debug overlay at screenblock 18 (memory address 0x0600:9000) */
	toncset16( &se_mem[18][0], 0, 2048 );

	/*********************************************************************************
		Initialisation
	*********************************************************************************/
	/* Initialise our variables */
	uint8_t scroll = 0;
	__key_curr = 0;
	__key_prev = 0;

	timer_count = 0;

	/* Configure BG0 using charblock 1 and screenblock 16 and 17 - background */
	REG_BG0CNT = BG_CBB(1) | BG_SBB(16) | BG_8BPP | BG_REG_32x32 | BG_PRIO(1);
	/* Configure BG1 using charblock 0 and screenblock 18 - debug overlay */
	REG_BG1CNT = BG_CBB(0) | BG_SBB(18) | BG_8BPP | BG_REG_32x32 | BG_PRIO(0);

	/* Set BG0 position */
	REG_BG0HOFS = 8;
	REG_BG0VOFS = 8;

	/* Set bitmap mode to 0 and show backgrounds 0 and 1, add sprites */
	REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ | DCNT_OBJ_1D;
	
	/* Initialise our sprites */
	oam_init( obj_buffer, 128 );

	/* Player */
	OBJ_ATTR *player_sprite = &obj_buffer[0];
	obj_set_attr(
		player_sprite, 
		( ATTR0_SQUARE | ATTR0_8BPP),
		( ATTR1_SIZE_16 ),
		( ATTR2_ID( 0 ) )
	);

	/* Put him in the centre of the screen */
	obj_set_pos( player_sprite, 112, 72 );

	/* Copy buffer to object memory */
	oam_copy( oam_mem, obj_buffer, 1 );

	/* Load the map and set our initial location */
	_current_map = &map_list[0];
	map_pos = (struct dir_vec){ 0, 0 };
	update_map();

	/* Enable the timer */
	//REG_TM0CNT = TM_FREQ_1 | TM_IRQ | TM_ENABLE;

	/* Enabled timer interrupts */
	//irq_init(master_isrs[0]);
	//irq_add(II_TIMER0, timr0_callback);

	/*********************************************************************************
		Game Loop
	*********************************************************************************/
	while(1) {

		/* Poll input keys */
		key_poll();
	
		/* Show debugger */
		debug();	

		/* Deal with key inputs */
		if( key_hit(KEY_UP) )
			map_pos.y -= 2;
		else if( key_hit(KEY_LEFT) )
			map_pos.x -= 2;
		else if( key_hit(KEY_DOWN) )
			map_pos.y += 2;
		else if( key_hit(KEY_RIGHT) )
			map_pos.x += 2;

		if( ( key_hit(KEY_UP) ) || ( key_hit(KEY_LEFT) ) || ( key_hit(KEY_DOWN) ) || ( key_hit(KEY_RIGHT) ) )
			update_map();

		/* Deal with TIMER0 */
		if( timer_count >= 24 ) {
			
			timer_count = 0;

			/* Game Tick */
			game_tick++;

			REG_BG0HOFS = 8 + scroll;
			scroll++;

		}

		/* Wait for video sync */
		vid_vsync();
	}

}
