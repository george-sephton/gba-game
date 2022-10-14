#include "project.h"

#include "include/keys.h"
#include "include/sprites.h"
#include "include/text.h"

#include "assets/font.h"
#include "assets/demo_project.h"

/*********************************************************************************
	Debugging Definitions
*********************************************************************************/
#define map_coordinates                 false	// Displays the coordinates of the display
#define map_rendering_offsets           false	// Displays player position on map and no of calculated empty rows & columns (for initial draw)
#define map_rendering_scroll            false	// Displays player position on map and no of calculated extra pixels to render 
#define player_movement                 false	// Displays player position
#define player_position                 false	// Draws coloured boxes to indicate if player can move or if there are obstacles, edge of map or exit tiles
#define interaction_info                false	// Displays information about interaction tiles
#define exit_map_info                   false	// Displays information about the map to load if exiting current map
#define animation_info                  true	// Displays information about the running animation
#define textbox_info                    false	// Displays information about the current textbox
#define npc_info                        false	// Displays information about the NPC
#define key_info                        false	// Displays information about the keys being pressed
#define tick_info                       false	// Displays various timer ticks

/*********************************************************************************
	Helpful Functions
*********************************************************************************/
/* Sets a tile value from screenblock at x, y */
static inline void set_tile( uint16_t x, uint16_t y, uint16_t tile, bool h_flip, bool v_flip, uint8_t screenblock ) {
	se_mem[ screenblock ][ x + ( y * 32 ) ] = tile | ( h_flip ? SE_HFLIP : 0 ) | ( v_flip ? SE_VFLIP : 0 );
}

/* Converts 32-bit RGB to 15-bit BGR */
static inline uint16_t rgb(uint32_t colour) {
	return 0x0 | ( ( ( ( ( colour >> 16 ) & 0xFF) / 8 ) << 0 ) | ( ( ( ( colour >> 8 ) & 0xFF) / 8 ) << 5 ) | ( ( ( colour & 0xFF) / 8 ) << 10 ) );
}

static inline int g_mod( int k, int n ) {
	return( ( k %= n ) < 0 ) ? k + n : k;
}

/*********************************************************************************
	Global Variables
*********************************************************************************/
uint16_t __key_curr;

/* Game ticks */
struct game_ticks ticks;

/* Map */
struct map *_current_map;
int16_t map_bg_texture_offset;

/* Player */
struct player_struct player;

/* Player movement variables */
OBJ_ATTR *player_sprite;
struct dir_vec map_pos; /* Map Position is defined as the position of the top left tile of the player */
struct dir_vec scroll_pos;
struct offset_vec _screen_offsets;
//struct dir_en allowed_movement, exit_tile, interaction_tile;
//int16_t interaction_tile_id[4];
uint8_t player_animation_tick, player_movement_delay;

/* Animation */
struct animation_settings animation;

#define move_multiplier                 2 // Defines how many spaces to move at a time
#define player_movement_delay_val       5 // Delay value before a player will start walking, to allow them to change the direction they're facing without walking

/* Movement scrolling animation */
bool scroll_movement;
uint16_t scroll_counter, move_tile_counter;
struct dir_vec upcoming_map_pos;
int8_t _scroll_x, _scroll_y;

char write_text[128];
OBJ_ATTR obj_buffer[128];


/*********************************************************************************
	Timer Variables
*********************************************************************************/
/* Timer IRQ Prototypes */
__attribute__((section(".iwram"), long_call)) void isr_master();
__attribute__((section(".iwram"), long_call)) void timr0_callback();

void timr0_callback( void ) {
	
	/* Increment our ticks */
	ticks.game++;
	ticks.animation++;
	ticks.player++;
}

const fnptr master_isrs[2] = {
	(fnptr)isr_master,
	(fnptr)timr0_callback 
};

/*********************************************************************************
	Game Functions
*********************************************************************************/
void debugging( void ) {

	#if player_movement
		sprintf( write_text, " P(%d,%d)S(%d,%d)D(%d,%d)F(%d,%d)    ", map_pos.x, map_pos.y, scroll_pos.x, scroll_pos.y, player.walk_dir.x, player.walk_dir.y, player.face_dir.x, player.face_dir.y );
		write_string( write_text, 0, 0, 0 );
	#endif

	#if key_info
		sprintf( write_text, " K(%d,%d,%d,%d,%d,%d)         ", (int)key_down(KEY_UP), (int)key_down(KEY_LEFT), (int)key_down(KEY_DOWN), (int)key_down(KEY_RIGHT), (int)key_down(KEY_A), (int)key_down(KEY_B) );
		write_string( write_text, 0, 1, 0 );
	#endif

	#if tick_info
		sprintf( write_text, " D(%d)T(%d)C(%d)", player_movement_delay, player_animation_tick, (int)game_tick );
		write_string( write_text, 0, 2, 0 );
	#endif

	#if animation_info
		sprintf( write_text,"R(%d,%d)F(%d,%d)S(%d,%d)T(%d,%d,%d)", animation.running, animation.reverse, animation.repeat, animation.finished, animation.step, ( ( 0x7 ^ animation.step ) - 2 ), animation.tick, animation.occurance, ( ( animation.tick % animation.occurance ) ? 0 : 1) );
		write_string( write_text, 0, 0, 0 );
	#endif

	#if map_rendering_offsets
		sprintf( write_text, " P(%d,%d)O(%d,%d,%d,%d)", map_pos.x, map_pos.y, _screen_offsets.n, _screen_offsets.e, _screen_offsets.s, _screen_offsets.w );
		write_string( write_text, 0, 1, 0 );
	#endif
	
	#if map_coordinates
		uint8_t i;
		for( i = 0; i < 10; i++ ) {
			write_character( ( 48 + i ), 0, i );
			write_character( ( 48 + i ), 0, ( i + 10 ) );
		}
		sprintf( write_text, "012345678901234567890123456789" );
		write_string( write_text, 0, 19, 0 );
	#endif
}

void calculate_map_offsets( void ) {

	_screen_offsets = (struct offset_vec){ 0, 0, 0, 0 };

	if( map_pos.y < 9 )
		_screen_offsets.n = ( 9 - map_pos.y );

	if( ( map_pos.x + 16 ) > _current_map->map_width )
		_screen_offsets.e = ( map_pos.x + 16 - _current_map->map_width );

	if( ( map_pos.y + 12 ) > _current_map->map_height )
		_screen_offsets.s = ( map_pos.y + 11 - _current_map->map_height );

	if( map_pos.x < 14 )
		_screen_offsets.w = ( 14 - map_pos.x );
}

void draw_map_tile( int16_t _draw_x, int16_t _draw_y, const struct map_tile (*map_tiles_ptr) ) {

	int16_t _texture_offset, i;

	if( _current_map->bg_texture != -1 )
		set_tile( _draw_x, _draw_y, map_bg_texture_offset, 0, 0, 16 );

	/* Make sure there's a texture to draw */
	if( (*map_tiles_ptr).texture < sizeof( _texture_lengths ) ) {

		/* Calculate the texture offset */
		_texture_offset = 0;
		for( i = 0; i < (*map_tiles_ptr).texture; i++ ) {
			_texture_offset += _texture_lengths[i];
		}
		_texture_offset += (*map_tiles_ptr).texture_offset;

		if( (*map_tiles_ptr).top_layer ) {

			/* Top Layer texture, draw to the top layer map */
			set_tile( _draw_x, _draw_y, _texture_offset, (*map_tiles_ptr).texture_reverse_x, (*map_tiles_ptr).texture_reverse_y, 18 );
		} else {

			/* Bottom Layer texture, draw to the normal map */
			set_tile( _draw_x, _draw_y, _texture_offset, (*map_tiles_ptr).texture_reverse_x, (*map_tiles_ptr).texture_reverse_y, 17 );
		}
	} else {

		/* If not, draw a transparent tile to ensure we're not showing old tiles in the buffer */
		set_tile( _draw_x, _draw_y, 0, 0, 0, 17 ); // Foreground Layer - transparent
	}
}

/* Max map size before we need to add scrolling functionality: width = 18, height = 22 */
void draw_map_init( void ) {

	/* Clears the display display buffer and draws the map */
	int16_t _draw_x, _draw_y, i;

	/* Clear background map at screenblock 16 (memory address 0x0600:8000) */
	toncset16( &se_mem[16][0], 1, 2048 );

	/* Set transparent background for foreground map at screenblock 17 (memory address 0x0600:8800) */
	toncset16( &se_mem[17][0], 0, 2048 );

	/* Set transparent background for top layer at screenblock 18 (memory address 0x0600:9000) */
	toncset16( &se_mem[18][0], 0, 2048 );

	/* Calculate gaps around displayed map */
	calculate_map_offsets();

	/* Calculate the background texture offset */
	map_bg_texture_offset = 0;
	if( _current_map->bg_texture != -1 ) {

		for( i = 0; i < _current_map->bg_texture; i++ ) {
			map_bg_texture_offset += _texture_lengths[i];
		}
		map_bg_texture_offset += _current_map->bg_texture_offset;
	}

	/* Get the current map tile pointer, starting at (0, 0) */
	const struct map_tile (*map_tiles_ptr) = _current_map->map_tiles_ptr;

	/* Move the map pointer to first column to draw, if there's a gap around the edges, we don't need to move anything */
	if( _screen_offsets.w == 0 )
		map_tiles_ptr += ( map_pos.x - 14 );

	/* Move the map pointer to first row to draw, if there's a gap around the edges, we don't need to move anything */
	if( _screen_offsets.n == 0 )
		map_tiles_ptr += ( ( map_pos.y - 9 ) * _current_map->map_width );

	/* Loop through each of the 20 rows of the display (y-direction) */
	for( _draw_y = 0; _draw_y < 20; _draw_y++ ) {
		
		/* Ignore tiles outside of the map limits */
		if( ( _draw_y >= _screen_offsets.n ) && ( _draw_y <= ( 19 - _screen_offsets.s ) ) ) {
			
			/* Loop through each of the 30 columns (x-direction) */
			for( _draw_x = 0; _draw_x < 30; _draw_x++ ) {

				/* Again, ignore tiles outside of the map limits */
				if( ( _draw_x >= _screen_offsets.w ) && ( ( map_pos.x + _draw_x - 13 ) <= _current_map->map_width ) ) {

					/* Draw map tile */
					draw_map_tile( _draw_x, _draw_y, map_tiles_ptr );

					/* If we've drawn a tile, increment the pointer (unless we're at the last column of the display, don't want to accidentally cause a hard fault) */
					if( _draw_x != 29 ) map_tiles_ptr++;
				}
			}

			/* Increment the pointer as we move to the next row on the dislay, but if we're drawing the last row, then don't move the pointer either */
			if( ( _screen_offsets.e == 0 ) || ( _screen_offsets.w == 0 ) ) {

				/* If we're drawing the last row then don't move the pointer either */
				if( _draw_y != 19 ) 
					map_tiles_ptr += ( _current_map->map_width - 29 + _screen_offsets.w + ( ( _screen_offsets.e > 1 ) ? ( _screen_offsets.e - 1 ) : 0 ) );
			}
		}
	}
}

void set_player_sprite( uint8_t offset, bool flip_h ) {

	/* Update the offset of the player sprite */
	obj_set_attr( player_sprite, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_16 | ( flip_h ? ATTR1_HFLIP : 0 ) ), ( ATTR2_ID( offset * 8 ) | ATTR2_PRIO( 1 ) ) );
	obj_set_pos( player_sprite, 112, 72 );
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
		
		for( i = 0; i < sizeof( _texture_colour_palette ); i++ ) {

			hexRed = ( ( ( _texture_colour_palette[ i ] ) & 0x1F) * 8 );
			hexGreen = ( ( ( _texture_colour_palette[ i ] >> 5 ) & 0x1F) * 8 );
			hexBlue = ( ( ( _texture_colour_palette[ i ] >> 10 ) & 0x1F) * 8 );

			hexDRed = hexRed - ( hexRed * ( 20 * step ) ) / 100;
			hexDGreen = hexGreen - ( hexGreen * ( 20 * step ) ) / 100;
			hexDBlue = hexBlue - ( hexBlue * ( 20 * step ) ) / 100;

			pal_bg_mem[ i ] = rgb( ( hexDRed << 16 ) | ( hexDGreen << 8 ) | ( hexDBlue ) );
		}

		for( i = 0; i < sizeof( _sprite_colour_palette ); i++ ) {

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

			if( animation.repeat > 0 ) {
				animation.repeat--;

				animation.reverse = !animation.reverse;
				animation.step = 0;
				animation.tick = 0;
			} else {
				animation.finished = true;
			}


		} else {
			animation.step++;
		}
	}
}

void init( void ) {
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

	/* Clear background map at screenblock 16 (memory address 0x0600:8000) */
	toncset16( &se_mem[16][0], 0, 2048 );
	/* Clear foreground map at screenblock 17 (memory address 0x0600:8800) */
	toncset16( &se_mem[17][0], 0, 2048 );
	/* Clear top layer at screenblock 18 (memory address 0x0600:9000) */
	toncset16( &se_mem[18][0], 0, 2048 );
	/* Clear debug overlay at screenblock 19 (memory address 0x0600:9800) */
	toncset16( &se_mem[19][0], 0, 2048 );

	/*********************************************************************************
		Initialisation
	*********************************************************************************/
	/* Initialise our variables */
	__key_curr = 0;
	scroll_pos = (struct dir_vec) { 0, 0 }; 

	/* Initialise our sprites */
	oam_init( obj_buffer, 128 );

	/* Initialise Player */
	player_sprite = &obj_buffer[0];
	set_player_sprite( 0, false );

	player.reverse_walking_render = false;
	player.walk_dir.x = 0;
	player.walk_dir.y = 0;
	/* Facing south */
	player.face_dir.x = 0;
	player.face_dir.y = -1;

	player_animation_tick = 0;
	player_movement_delay = 0;
	scroll_movement = false;
	scroll_counter = 0;
	move_tile_counter = 0;
	upcoming_map_pos.x = 0;
	upcoming_map_pos.y = 0;

	/* Initialise timer and ticks */
	ticks = (struct game_ticks) { 0, 0, 0 };

	/* Configure BG0 using charblock 1 and screenblock 16 - background */
	REG_BG0CNT = BG_CBB(1) | BG_SBB(16) | BG_8BPP | BG_REG_32x32 | BG_PRIO(3);
	/* Configure BG1 using charblock 1 and screenblock 17 - foreground */
	REG_BG1CNT = BG_CBB(1) | BG_SBB(17) | BG_8BPP | BG_REG_32x32 | BG_PRIO(2);
	/* Configure BG2 using charblock 1 and screenblock 18 - top layer */
	REG_BG2CNT = BG_CBB(1) | BG_SBB(18) | BG_8BPP | BG_REG_32x32 | BG_PRIO(0);
	/* Configure BG3 using charblock 0 and screenblock 19 - menu/dialog/debug overlay */
	REG_BG3CNT = BG_CBB(0) | BG_SBB(19) | BG_8BPP | BG_REG_32x32 | BG_PRIO(0);

	/* Set BG0 position */
	REG_BG0HOFS = 0;
	REG_BG0VOFS = 0;
	/* Set BG1 position */
	REG_BG1HOFS = 0;
	REG_BG1VOFS = 0;
	/* Set BG2 position */
	REG_BG2HOFS = 0;
	REG_BG2VOFS = 0;
	/* Set BG3 position */
	REG_BG3HOFS = 0;
	REG_BG3VOFS = 0;

	/* Set bitmap mode to 0 and show all backgrounds and show sprites */
	REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_BG2 | DCNT_BG3 | DCNT_OBJ | DCNT_OBJ_1D;

	/* Load the map and set our initial location */
	map_pos = (struct dir_vec) { 10, 16 };
	_current_map = &map_list[2];

	draw_map_init();

	/* Enable the timer */
	REG_TM0CNT = TM_FREQ_1 | TM_IRQ | TM_ENABLE;

	/* Enabled timer interrupts */
	irq_init(master_isrs[0]);
	irq_add(II_TIMER0, timr0_callback);

	/* Initialise animation variables - none in progress */
	animation.running = false;
}

int main( void )
{
	int16_t _draw_x, _draw_y;

	init();

	/*********************************************************************************
		Game Loop
	*********************************************************************************/
	while(1) {

		/* Poll input keys */
		key_poll();
	
		/* Show debugger */
		debugging();

		/* Development - Refresh display when B is pressed */
		if( key_down( KEY_B ) ) {

			draw_map_init();

			/* Re-initialise variables */
			scroll_pos = (struct dir_vec){ 0, 0 }; 

			/* Set BG0 position */
			REG_BG0HOFS = 0;
			REG_BG0VOFS = 0;
			/* Set BG1 position */
			REG_BG1HOFS = 0;
			REG_BG1VOFS = 0;
			/* Set BG2 position */
			REG_BG2HOFS = 0;
			REG_BG2VOFS = 0;
			/* Set BG3 position */
			REG_BG3HOFS = 0;
			REG_BG3VOFS = 0;
		}

		/* Development - Fade display out when A is pressed */
		if( key_down( KEY_A ) ) {

			animation.running = true;
			animation.finished = false;
			animation.reverse = true;
			animation.animation_ptr = fade_screen;
			animation.step = 0;
			animation.tick = 0;
			animation.occurance = 2;
			animation.repeat = 0;
		}

		/* Check input presses as they happen, unless there's an animation running, in which case we don't care */
		if( ( !animation.running ) && ( !scroll_movement ) /*&& ( !textbox_running )*/ ) {

			if( ( key_down( KEY_UP ) ) || ( key_down( KEY_RIGHT ) ) || ( key_down( KEY_DOWN ) ) || ( key_down( KEY_LEFT ) ) ) {

				if( player_movement_delay < player_movement_delay_val ) player_movement_delay++;

				if( key_down( KEY_UP ) ) {

					/* Switch player's direction to N */
					player.walk_dir.x = 0;
					player.walk_dir.y = 1;

					player.face_dir.x = 0;
					player.face_dir.y = 1;
				} else if( key_down( KEY_RIGHT ) ) {

					/* Switch player's direction to E */
					player.walk_dir.x = 1;
					player.walk_dir.y = 0;

					player.face_dir.x = 1;
					player.face_dir.y = 0;
				} else if( key_down( KEY_DOWN ) ) {

					/* Switch player's direction to S */
					player.walk_dir.x = 0;
					player.walk_dir.y = -1;

					player.face_dir.x = 0;
					player.face_dir.y = -1;
				} else if( key_down( KEY_LEFT ) ) {

					/* Switch player's direction to W */
					player.walk_dir.x = -1;
					player.walk_dir.y = 0;

					player.face_dir.x = -1;
					player.face_dir.y = 0;
				}
			} else {

				/* Player's not moving */
				player.walk_dir.x = 0;
				player.walk_dir.y = 0;

				/* Reset the delay counter */
				player_movement_delay = 0;
			}
		}

		/*********************************************************************************
			Drawing
		*********************************************************************************/
		/* Draw the player in the centre square */
		if( ( ( player.walk_dir.x == 0 ) && ( player.walk_dir.y == 0 ) ) || ( player_movement_delay < player_movement_delay_val ) ) {

			/* The player is currently stood still, let's draw them facing the right direction */
			if((player.face_dir.x == 0) && (player.face_dir.y == 1)) { // Facing N
				set_player_sprite( 4, false );
			} else if((player.face_dir.x == 1) && (player.face_dir.y == 0)) { // Facing E
				set_player_sprite( 2, true );
			} else if((player.face_dir.x == -1) && (player.face_dir.y == 0)) { // Facing W
				set_player_sprite( 2, false );
			} else { // Facing S - also default value
				set_player_sprite( 0, false );
			}
		} else {

			if( !scroll_movement ) {
				player_animation_tick = 0;
				player.reverse_walking_render = false;
			}

			/* Player is walking, draw the walking sprite */
			if( ( ( player.walk_dir.x == 0 ) && ( player.walk_dir.y == 1 ) ) ) { // Walking N
				set_player_sprite( 5, player.reverse_walking_render );
			} else if( ( ( player.walk_dir.x == 1 ) && ( player.walk_dir.y == 0 ) ) ) { // Walking E
				set_player_sprite( ( player.reverse_walking_render ? 2 : 3 ), true );
			} else if( ( ( player.walk_dir.x == 0 ) && ( player.walk_dir.y == -1 ) ) ) { // Walking S
				set_player_sprite( 1, player.reverse_walking_render );
			} else if( ( ( player.walk_dir.x == -1 ) && ( player.walk_dir.y == 0 ) ) ) { // Walking W
				set_player_sprite( ( player.reverse_walking_render ? 2 : 3 ), false );
			}
		}

		/* Move the player according to button inputs and movement restrictions */
		if( ( !animation.running ) && /*( !textbox_running ) &&*/ ( !scroll_movement ) && ( player_movement_delay >= player_movement_delay_val ) ) { // Don't alter movement if an animation is running

			/* Player is moving north */
			if( ( player.walk_dir.x == 0 ) && ( player.walk_dir.y == 1 ) ) {

				/* Map exit, start animation and load new map */
				/*if( exit_tile.travel_n ) {  
					start_exit_animation = true;
				} else {*/

					/* Move to a new tile, check if they're allowed there */
					//if( allowed_movement.travel_n ) {

						/* Start the scroll animation and set the new tile position */
						upcoming_map_pos.y = -1;
						scroll_movement = true;
						player_animation_tick = 0;
					//}
				//}

			/* Player is moving east */
			} else if( ( player.walk_dir.x == 1 ) && ( player.walk_dir.y == 0 ) ) {
				
				/* Map exit, start animation and load new map */
				/*if( exit_tile.travel_e ) {
					start_exit_animation = true;
				} else {*/

					/* Move to a new tile, check if they're allowed there */
					//if( allowed_movement.travel_e ) {
						
						/* Start the scroll animation and set the new tile position */
						upcoming_map_pos.x = 1;
						scroll_movement = true;
						player_animation_tick = 0;
					//}
				//}
			
			/* Player is moving south */
			} else if( ( player.walk_dir.x == 0 ) && ( player.walk_dir.y == -1 ) ) {
			
				/* Map exit, start animation and load new map */
				/*if( exit_tile.travel_s ) {
					start_exit_animation = true;
				} else {*/

					/* Move to a new tile, check if they're allowed there */
					//if( allowed_movement.travel_s)  {
						
						/* Start the scroll animation and set the new tile position */
						upcoming_map_pos.y = 1;
						scroll_movement = true;
						player_animation_tick = 0;
					//}
				//}

			/* Player is moving west */
			} else if( ( player.walk_dir.x == -1 ) && ( player.walk_dir.y == 0 ) ) {

				/* Map exit, start animation and load new map */
				/*if( exit_tile.travel_w ) {
					start_exit_animation = true;
				} else {*/

					/* Move to a new tile, check if they're allowed there */
					//if( allowed_movement.travel_w ) {
						
						/* Start the scroll animation and set the new tile position */
						upcoming_map_pos.x = -1;
						scroll_movement = true;
						player_animation_tick = 0;
					//}
				//}
			}
		}

		/* Load in next row/column of tiles if we're moving */
		if( ( upcoming_map_pos.x != 0 ) || ( upcoming_map_pos.y != 0 ) ) {

			/* First calculate gaps around displayed map */
			calculate_map_offsets();

			/* Get the current map tile pointer, starting at (0, 0) */
			const struct map_tile (*map_tiles_ptr) = _current_map->map_tiles_ptr;

			/* Player's walking north, let's add another row of tiles at the top of the buffer */
			if( upcoming_map_pos.y < 0 ) {

				/* Move the map pointer to first column to draw, if there's a gap around the edges, we don't need to move anything */
				if( _screen_offsets.w == 0 )
					map_tiles_ptr += ( map_pos.x - 14 );

				/* Check if we have another column of tiles to show */
				if( map_pos.y >= 10 ) {

					/* Move the map pointer to new row */
					map_tiles_ptr += ( ( map_pos.y - 10 ) * _current_map->map_width );

					/* Loop through the 32 columns of the buffer (x-direction) */
					for( _draw_x = 0; _draw_x < 32; _draw_x++ ) {

						/* Ignore tiles outside of the map limits */
						if( ( _draw_x >= _screen_offsets.w ) && ( _draw_x <= ( 29 - _screen_offsets.e ) ) ) {

							/* Draw map tile */
							draw_map_tile( g_mod( ( _draw_x + scroll_pos.x ), 32 ), g_mod( ( 31 + scroll_pos.y ), 32 ), map_tiles_ptr );

							/* If we've drawn a tile, increment the pointer (unless we're at the last column of the display, don't want to accidentally cause a hard fault) */
							if( _draw_x != 29 ) map_tiles_ptr++;
						}
					}

				/* Nothing to show, so draw black tiles */
				} else {

					/* Loop through the 32 columns of the display (x-direction) */
					for( _draw_x = 0; _draw_x < 32; _draw_x++ ) {
						
						/* Ignore tiles outside of the map limits */
						set_tile( g_mod( ( _draw_x + scroll_pos.x ), 32 ), g_mod( ( 31 + scroll_pos.y ), 32 ), 1, 0, 0, 16 ); // Background Layer - black
						set_tile( g_mod( ( _draw_x + scroll_pos.x ), 32 ), g_mod( ( 31 + scroll_pos.y ), 32 ), 0, 0, 0, 17 ); // Foreground Layer - transparent
					}
				}

			/* Player's walking east, let's add another column of tiles on the right side of the buffer */
			} else if( upcoming_map_pos.x > 0 ) {

				/* Move the map pointer to the first row on the display */
				if( _screen_offsets.n == 0 )
					map_tiles_ptr += ( ( map_pos.y - 9 ) * _current_map->map_width );

				/* Check if we have another column of tiles to show */
				if( ( map_pos.x + 16 ) < _current_map->map_width ) {
					
					/* Move the map pointer to new column */
					map_tiles_ptr += ( 16 + map_pos.x );

					/* Loop through each of the 20 rows of the display (y-direction) */
					for( _draw_y = 0; _draw_y < 20; _draw_y++ ) {
						
						/* Ignore tiles outside of the map limits */
						if( ( _draw_y >= _screen_offsets.n ) && ( _draw_y <= ( 19 - _screen_offsets.s ) ) ) {
							
							draw_map_tile( g_mod( ( 30 + scroll_pos.x ), 32 ), g_mod( ( _draw_y + scroll_pos.y ), 32 ), map_tiles_ptr );

							/* Increment the pointer as we move to the next row on the dislay, but if we're drawing the last row, then don't move the pointer either */
							if( _draw_y != 19 ) 
								map_tiles_ptr += ( _current_map->map_width );
						}
					}

				/* Nothing to show, so draw black tiles */
				} else {

					/* Loop through the 32 rows of the display (y-direction) */
					for( _draw_y = 0; _draw_y < 32; _draw_y++ ) {
						
						/* Ignore tiles outside of the map limits */
						set_tile( g_mod( ( 30 + scroll_pos.x ), 32 ), _draw_y, 1, 0, 0, 16 ); // Background Layer - black
						set_tile( g_mod( ( 30 + scroll_pos.x ), 32 ), _draw_y, 0, 0, 0, 17 ); // Foreground Layer - transparent
					}
				}

			/* Player's walking south, let's add another row of tiles at the bottom of the buffer */
			} else if( upcoming_map_pos.y > 0 ) {

				/* Move the map pointer to first column to draw, if there's a gap around the edges, we don't need to move anything */
				if( _screen_offsets.w == 0 )
					map_tiles_ptr += ( map_pos.x - 14 );

				/* Check if we have another column of tiles to show */
				if( ( map_pos.y + 11 ) < _current_map->map_height ) {

					/* Move the map pointer to new row */
					map_tiles_ptr += ( ( map_pos.y + 11 ) * _current_map->map_width );

					/* Loop through the 32 columns of the buffer (x-direction) */
					for( _draw_x = 0; _draw_x < 32; _draw_x++ ) {

						/* Ignore tiles outside of the map limits */
						if( ( _draw_x >= _screen_offsets.w ) && ( _draw_x <= ( 29 - _screen_offsets.e ) ) ) {

							/* Draw map tile - add 32 to the coordinates to ensure we don't end up with negative values */
							draw_map_tile( g_mod( ( _draw_x + scroll_pos.x ), 32 ), g_mod( ( 20 + scroll_pos.y ), 32 ), map_tiles_ptr );

							/* If we've drawn a tile, increment the pointer (unless we're at the last column of the display, don't want to accidentally cause a hard fault) */
							if( _draw_x != 29 ) map_tiles_ptr++;
						}
					}

				/* Nothing to show, so draw black tiles */
				} else {

					/* Loop through the 32 columns of the display (x-direction) */
					for( _draw_x = 0; _draw_x < 32; _draw_x++ ) {
						
						/* Ignore tiles outside of the map limits */
						set_tile( g_mod( ( _draw_x + scroll_pos.x ), 32 ), g_mod( ( 20 + scroll_pos.y ), 32 ), 1, 0, 0, 16 ); // Background Layer - black
						set_tile( g_mod( ( _draw_x + scroll_pos.x ), 32 ), g_mod( ( 20 + scroll_pos.y ), 32 ), 0, 0, 0, 17 ); // Foreground Layer - transparent
					}
				}

			/* Player's walking west, let's add another column of tiles on the left side of the buffer */
			} else if( upcoming_map_pos.x < 0 ) {

				/* Move the map pointer to the first row on the display */
				if( _screen_offsets.n == 0 )
					map_tiles_ptr += ( ( map_pos.y - 9 ) * _current_map->map_width );

				/* Check if we have another column of tiles to show */
				if( map_pos.x >= 15 ) {

					/* Move the map pointer to new column */
					map_tiles_ptr += ( map_pos.x - 15 );

					/* Loop through each of the 20 rows of the display (y-direction) */
					for( _draw_y = 0; _draw_y < 20; _draw_y++ ) {
						
						/* Ignore tiles outside of the map limits */
						if( ( _draw_y >= _screen_offsets.n ) && ( _draw_y <= ( 19 - _screen_offsets.s ) ) ) {
							
							draw_map_tile( g_mod( ( 31 + scroll_pos.x ), 32 ), g_mod( ( _draw_y + scroll_pos.y ), 32 ), map_tiles_ptr );

							/* Increment the pointer as we move to the next row on the dislay, but if we're drawing the last row, then don't move the pointer either */
							if( _draw_y != 19 ) 
								map_tiles_ptr += ( _current_map->map_width );
						}
					}

				/* Nothing to show, so draw black tiles */
				} else {

					/* Loop through the 32 rows of the display (y-direction) */
					for( _draw_y = 0; _draw_y < 32; _draw_y++ ) {
						
						/* Ignore tiles outside of the map limits */
						set_tile( g_mod( ( 31 + scroll_pos.x ), 32 ), _draw_y, 1, 0, 0, 16 ); // Background Layer - black
						set_tile( g_mod( ( 31 + scroll_pos.x ), 32 ), _draw_y, 0, 0, 0, 17 ); // Foreground Layer - transparent

					}

				}
			}
		}

		/*********************************************************************************
			Ticks
		*********************************************************************************/
		/* Animation Tick */
		if( ticks.animation >= 5 ) {
			
			ticks.animation = 0;

			/* Advance the animation, if running and not finished */
			if( ( animation.running ) && ( !animation.finished ) ) {
				animation.tick++;

				if( animation.tick >= animation.occurance ) {

					animation.tick = 0;

					/* Call animation function */
					animation.animation_ptr();

					/* Check if animation has finished */
					if( animation.finished ) 
						animation.running = false;
				}
			}
		}

		/* Scroll Tick */
		if( ticks.player >= 5 ) {
			
			ticks.player = 0;

			/* Advance scrolling animation */
			if( scroll_movement ) {
			
				/* If holding B, make the player run (move twice as fast), if the map allows running */
				/*if( ( button( B ) ) && ( _current_map->running_en ) ) scroll_counter += 2;
				else*/ scroll_counter++;

				/* Scroll display */
				_scroll_x = player.walk_dir.x * ( scroll_counter % 8 );
				_scroll_y = -player.walk_dir.y * ( scroll_counter % 8 );

				REG_BG0HOFS = ( 8 * scroll_pos.x ) + _scroll_x;
				REG_BG0VOFS = ( 8 * scroll_pos.y ) + _scroll_y;

				REG_BG1HOFS = ( 8 * scroll_pos.x ) + _scroll_x;
				REG_BG1VOFS = ( 8 * scroll_pos.y ) + _scroll_y;

				REG_BG2HOFS = ( 8 * scroll_pos.x ) + _scroll_x;
				REG_BG2VOFS = ( 8 * scroll_pos.y ) + _scroll_y;

				if( ( scroll_counter % 8 ) == 7 ) {

					/* Count how many tiles we've moved so far */
					move_tile_counter++;

					/* Update the scroll as the tiles are updated 1 row/column at a time according to the scroll count */
					scroll_pos.x += upcoming_map_pos.x;
					scroll_pos.y += upcoming_map_pos.y;

					/* Move the player to their new space */
					map_pos.x += upcoming_map_pos.x;
					map_pos.y += upcoming_map_pos.y;

					if( move_tile_counter == move_multiplier ) {

						/* Time to stop moving, reset the counters */
						scroll_counter = 0;
						move_tile_counter = 0;

						/* If the player isn't holding down a button, let's stop moving */
						if( ( ( player.walk_dir.x == 0 ) && ( player.walk_dir.y == 1 ) && ( !key_down( KEY_UP ) ) ) || 
								( ( player.walk_dir.x == 1 ) && ( player.walk_dir.y == 0 ) && ( !key_down( KEY_RIGHT ) ) ) ||
								( ( player.walk_dir.x == 0 ) && ( player.walk_dir.y == -1 ) && ( !key_down( KEY_DOWN ) ) ) ||
								( ( player.walk_dir.x == -1 ) && ( player.walk_dir.y == 0 ) && ( !key_down( KEY_LEFT ) ) )
							) {
				
							scroll_movement = false;
							upcoming_map_pos.x = 0;
							upcoming_map_pos.y = 0;

							player.walk_dir.x = 0;
							player.walk_dir.y = 0;
						}
					}
				}
			}

			/* If the player is walking, let's make their little legs move or make them run if holding B */
			if( ( !animation.running ) || ( ( player.walk_dir.x != 0 ) || ( player.walk_dir.y != 0 ) ) ) {

				player_animation_tick++;

				/* If holding B, make the player run, if the map allows running */
				if( ( ( player_animation_tick >= 6 ) && ( ( ( !key_down( KEY_B ) ) && ( _current_map->running_en ) ) || ( !_current_map->running_en ) ) ) || ( ( player_animation_tick >= 3 ) && ( key_down( KEY_B ) ) && ( _current_map->running_en ) ) ) {
					player_animation_tick = 0;
					player.reverse_walking_render = !player.reverse_walking_render;
				}
			}
		}

		/* Update player sprite */
		oam_copy( oam_mem, obj_buffer, 1 );

		/* Wait for video sync */
		vid_vsync();
	}

}
