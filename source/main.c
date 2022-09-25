#include "project.h"

#include "demo_project.h"
#include "font.h"
#include "keys.h"
#include "sprites.h"
#include "text.h"

#include "tonc_utils.h"
#include "tonc_irq.h"

/*********************************************************************************
	Debugging Definitions
*********************************************************************************/
#define map_coordinates					false	// Displays the coordinates of the display
#define map_rendering_offsets			false	// Displays player position on map and no of calculated empty rows & columns
#define map_rendering_scroll			false	// Displays player position on map and no of calculated extra pixels to render 
#define player_movement					true	// Displays player position
#define player_position					false	// Draws coloured boxes to indicate if player can move or if there are obstacles, edge of map or exit tiles
#define interaction_info				false	// Displays information about interaction tiles
#define exit_map_info					false	// Displays information about the map to load if exiting current map
#define animation_info					false	// Displays information about the running animation
#define textbox_info					false	// Displays information about the current textbox
#define npc_info						false	// Displays information about the NPC
#define key_info						false	// Displays information about the keys being pressed
#define tick_info						false	// Displays various timer ticks

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

/*********************************************************************************
	Global Variables
*********************************************************************************/
uint16_t __key_curr;

/* Map */
struct map *_current_map;

/* Player */
struct player_struct player;

/* Player movement variables */
OBJ_ATTR *player_sprite;
struct dir_vec map_pos;
struct offset_vec _screen_offsets;
//struct dir_en allowed_movement, exit_tile, interaction_tile;
//int16_t interaction_tile_id[4];
uint8_t player_animation_tick, player_movement_delay;

#define player_movement_delay_val     5 // Delay value before a player will start walking, to allow them to change the direction they're facing without walking

/* Movement scrolling animation */
bool scroll_movement;
uint16_t scroll_counter;
struct dir_vec upcoming_map_pos;
int8_t _scroll_x, _scroll_y;


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
void debugging( void ) {

	#if player_movement
		sprintf( write_text,"P(%d,%d)D(%d,%d)F(%d,%d)    ", map_pos.x, map_pos.y, player.walk_dir.x, player.walk_dir.y, player.face_dir.x, player.face_dir.y );
		write_string( write_text, 0, 0, 0 );
	#endif

	#if key_info
		sprintf( write_text, "K(%d,%d,%d,%d,%d,%d)         ", (int)key_down(KEY_UP), (int)key_down(KEY_LEFT), (int)key_down(KEY_DOWN), (int)key_down(KEY_RIGHT), (int)key_down(KEY_A), (int)key_down(KEY_B) );
		write_string( write_text, 0, 1, 0 );
	#endif

	#if tick_info
		sprintf( write_text, "D(%d)T(%d)C(%d)", player_movement_delay, player_animation_tick, (int)game_tick );
		write_string( write_text, 0, 2, 0 );
	#endif

	#if map_rendering_offsets
		sprintf( write_text, "P(%d,%d)O(%d,%d,%d,%d)", map_pos.x, map_pos.y, _screen_offsets.n, _screen_offsets.e, _screen_offsets.s, _screen_offsets.w );
		write_string( write_text, 0, 1, 0 );
	#endif
	
	#if map_coordinates
		sprintf( write_text, "012345678901234567890123456789" );
		write_string( write_text, 0, 0, 0 );

		uint8_t i;
		for( i = 0; i < 10; i++ ) {
			write_character( ( 48 + i ), 0, i );
			write_character( ( 48 + i ), 0, ( i + 10 ) );
		}
	#endif
}

void draw_map_init( void ) {

	/* Max map size before we need to add scrolling functionality: width = 18, height = 22 */
	int16_t _draw_x, _draw_y, _bg_texture_offset, _texture_offset, i;

	/* Calculate how many tiles in each direction we need to draw */
	_screen_offsets = (struct offset_vec){ 0, 0, 0, 0 };

	/* Clear background map at screenblock 16 (memory address 0x0600:8000) */
	toncset16( &se_mem[16][0], 1, 2048 );

	/* Set transparent background for foreground map at screenblock 17 (memory address 0x0600:8800) */
	toncset16( &se_mem[17][0], 0, 2048 );

	/* Calculate the background texture offset */
	_bg_texture_offset = 0;
	if( _current_map->bg_texture != -1 ) {

		for( i = 0; i < _current_map->bg_texture; i++ ) {
			_bg_texture_offset += _texture_lengths[i];
		}
		_bg_texture_offset += _current_map->bg_texture_offset;
	}

	/* Get the current map tile pointer, starting at (0, 0) */
	const struct map_tile (*map_tiles_ptr) = _current_map->map_tiles_ptr;

	/* Draw the map tiles, start by looping through the 20 rows of the display (y-direction) */
	for( _draw_y=0; _draw_y < _current_map->map_height; _draw_y++ ) {
		
		/* Only draw if there's map tiles to be drawn */
		if( _draw_y < 22 ) {
			
			/* Next loop through the 30 columns of the display (x-direction) */
			for( _draw_x=0; _draw_x < _current_map->map_width; _draw_x++ ) {

				/* Again, make sure we're drawing the map titles in the correct places */
				if( _draw_x < 18 ) {
					
					if( _current_map->bg_texture != -1 )
						set_tile( _draw_x, _draw_y, _bg_texture_offset, 0, 0, 16 );

					/* Make sure there's a texture to draw */
					if( ( (*map_tiles_ptr).texture < sizeof( _texture_lengths ) ) /*&& ( (*map_tiles_ptr).top_layer == false)*/ ) {

						/* Calculate the texture offset */
						_texture_offset = 0;
						for( i = 0; i < (*map_tiles_ptr).texture; i++ ) {
							_texture_offset += _texture_lengths[i];
						}
						_texture_offset += (*map_tiles_ptr).texture_offset;

						set_tile( _draw_x, _draw_y, _texture_offset, (*map_tiles_ptr).texture_reverse_x, (*map_tiles_ptr).texture_reverse_y, 17 );
					}

					/* If we've drawn a tile, increment the pointer (unless we're at the last column of the display, don't want to accidentally cause a hard fault) */
					if( _draw_x < _current_map->map_width ) map_tiles_ptr++;
				}
			}

			/* Increment the pointer as we move to the next row on the dislay, but if we're drawing the last row, then don't move the pointer either */
		}
	}
}

void set_player_sprite( uint8_t offset, bool flip_h ) {

	/* Update the offset of the player sprite */
	obj_set_attr( player_sprite, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_16 | ( flip_h ? ATTR1_HFLIP : 0 ) ), ( ATTR2_ID( offset * 8 ) ) );
	obj_set_pos( player_sprite, 112, 72 );
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

	/* Clear background map at screenblock 16 (memory address 0x0600:8000) */
	toncset16( &se_mem[16][0], 0, 2048 );
	/* Clear foreground map at screenblock 17 (memory address 0x0600:8800) */
	toncset16( &se_mem[17][0], 0, 2048 );
	/* Clear debug overlay at screenblock 18 (memory address 0x0600:9000) */
	toncset16( &se_mem[18][0], 0, 2048 );

	/*********************************************************************************
		Initialisation
	*********************************************************************************/
	/* Initialise our variables */
	uint8_t scroll = 0;
	__key_curr = 0;

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
	bool scroll_movement = false;
	scroll_counter = 0;
	upcoming_map_pos.x = 0;
	upcoming_map_pos.y = 0;

	/* Initialise timer */
	timer_count = 0;

	/* Configure BG0 using charblock 1 and screenblock 16 - background */
	REG_BG0CNT = BG_CBB(1) | BG_SBB(16) | BG_8BPP | BG_REG_32x32 | BG_PRIO(2);
	/* Configure BG1 using charblock 1 and screenblock 17 - foreground */
	REG_BG1CNT = BG_CBB(1) | BG_SBB(17) | BG_8BPP | BG_REG_32x32 | BG_PRIO(1);
	/* Configure BG2 using charblock 0 and screenblock 18 - debug overlay */
	REG_BG2CNT = BG_CBB(0) | BG_SBB(18) | BG_8BPP | BG_REG_32x32 | BG_PRIO(0);

	/* Set BG0 position */
	REG_BG0HOFS = 0;
	REG_BG0VOFS = 0;
	/* Set BG1 position */
	REG_BG1HOFS = 0;
	REG_BG1VOFS = 0;

	/* Set bitmap mode to 0 and show backgrounds 0 and 1, add sprites */
	REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D;

	/* Load the map and set our initial location */
	_current_map = &map_list[0];
	map_pos = (struct dir_vec){ 0, 0 };
	draw_map_init();

	/* Enable the timer */
	REG_TM0CNT = TM_FREQ_1 | TM_IRQ | TM_ENABLE;

	/* Enabled timer interrupts */
	irq_init(master_isrs[0]);
	irq_add(II_TIMER0, timr0_callback);

	/*********************************************************************************
		Game Loop
	*********************************************************************************/
	while(1) {

		/* Poll input keys */
		key_poll();
	
		/* Show debugger */
		debugging();

		/* Check input presses as they happen, unless there's an animation running, in which case we don't care */
		if( /*( !animation.running ) &&*/ ( !scroll_movement ) /*&& ( !textbox_running )*/ ) {

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
		if( /*( !animation.running ) && ( !textbox_running ) &&*/ ( !scroll_movement ) && ( player_movement_delay >= player_movement_delay_val ) ) { // Don't alter movement if an animation is running

			/* Player is moving north */
			if( ( player.walk_dir.x == 0 ) && ( player.walk_dir.y == 1 ) ) {

				/* Map exit, start animation and load new map */
				/*if( exit_tile.travel_n ) {  
					start_exit_animation = true;
				} else {*/

					/* Move to a new tile, check if they're allowed there */
					//if( allowed_movement.travel_n ) {

						/* Start the scroll animation and set the new tile position */
						upcoming_map_pos.y = -2;
						scroll_movement = true;
						player_animation_tick = 0;
						//if( button( B ) ) map_pos.y--;
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
						upcoming_map_pos.x = 2;
						scroll_movement = true;
						player_animation_tick = 0;
						//if( button( B ) ) map_pos.x++;
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
						upcoming_map_pos.y = 2;
						scroll_movement = true;
						player_animation_tick = 0;
						//if( button( B ) ) map_pos.y++;
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
						upcoming_map_pos.x = -2;
						scroll_movement = true;
						player_animation_tick = 0;
						//if( button( B ) ) map_pos.x--;
					//}
				//}
			}
		}

		/* Deal with timer, allows us to do animations etc a bit more precisely and not too fast */
		if( timer_count >= 5 ) {
			
			timer_count = 0;

			/* Game Tick */
			game_tick++;

			/* Advance scrolling animation */
			if( scroll_movement ) {
			
				/* If holding B, make the player run (move twice as fast), if the map allows running */
				/*if( ( button( B ) ) && ( _current_map->running_en ) ) scroll_counter += 2;
				else*/ scroll_counter++;

				if(scroll_counter >= 16) {

					/* Reset the counter and move the player to their new space */
					scroll_counter = 0;
					map_pos.x += upcoming_map_pos.x;
					map_pos.y += upcoming_map_pos.y;

					/* If the player isn't holding down a button, let's stop moving */
					/*if( ( ( player.walk_dir.x == 0 ) && ( player.walk_dir.y == 1 ) && ( !button( UP ) ) ) || 
							( ( player.walk_dir.x == 1 ) && ( player.walk_dir.y == 0 ) && ( !button( RIGHT ) ) ) ||
							( ( player.walk_dir.x == 0 ) && ( player.walk_dir.y == -1 ) && ( !button( DOWN ) ) ) ||
							( ( player.walk_dir.x == -1 ) && ( player.walk_dir.y == 0 ) && ( !button( LEFT ) ) )
						) {*/
			
						scroll_movement = false;
						upcoming_map_pos.x = 0;
						upcoming_map_pos.y = 0;

						player.walk_dir.x = 0;
						player.walk_dir.y = 0;
					//}
				}
			}

			/* If the player is walking, let's make their little legs move or make them run if holding B */
			if( /*( !animation.running ) ||*/ ( ( player.walk_dir.x != 0 ) || ( player.walk_dir.y != 0 ) ) ) {

				player_animation_tick++;

				/* If holding B, make the player run, if the map allows running */
				if( ( ( player_animation_tick >= 6 ) && ( ( ( !key_down( KEY_B ) ) && ( _current_map->running_en ) ) || ( !_current_map->running_en ) ) ) || ( ( player_animation_tick >= 3 ) && ( key_down( KEY_B ) ) && ( _current_map->running_en ) ) ) {
					player_animation_tick = 0;
					player.reverse_walking_render = !player.reverse_walking_render;
				}
			}
		}

		/* Scroll display */
		_scroll_x = player.walk_dir.x * scroll_counter;
		_scroll_y = -player.walk_dir.y * scroll_counter;

		REG_BG0HOFS = ( 8 * ( map_pos.x - 14 ) ) + _scroll_x;
		REG_BG0VOFS = ( 8 * ( map_pos.y - 9 ) ) + _scroll_y;

		REG_BG1HOFS = ( 8 * ( map_pos.x - 14 ) ) + _scroll_x;
		REG_BG1VOFS = ( 8 * ( map_pos.y - 9 ) ) + _scroll_y;

		/* Update player sprite */
		oam_copy( oam_mem, obj_buffer, 1 );

		/* Wait for video sync */
		vid_vsync();
	}

}
