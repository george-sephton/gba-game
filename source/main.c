#include "project.h"

#include "include/animations.h"
#include "include/keys.h"
#include "include/sprites.h"
#include "include/text.h"

#include "assets/font.h"
#include "assets/demo_project.h"

/*********************************************************************************
	Global Variables
*********************************************************************************/
/* Game ticks */
struct game_ticks                       ticks;

/* Map */
int16_t                                 map_bg_texture_offset;
struct map                             *_current_map;
struct exit_map_info                    exit_map;

/* Player */
struct player_struct                    player;

/* Player movement variables */
OBJ_ATTR                               *player_sprite;
struct dir_vec                          map_pos; /* Map Position is defined as the position of the top left tile of the player */
struct dir_vec                          scroll_pos;
struct offset_vec                       _screen_offsets;
struct dir_en                           allowed_movement, exit_tile, interaction_tile;
uint8_t                                 player_animation_tick, player_movement_delay;
int16_t                                 interaction_tile_id[4];

#define player_movement_delay_val       5 // Delay value before a player will start walking, to allow them to change the direction they're facing without walking

/* Movement scrolling animation */
struct dir_vec                          upcoming_map_pos;
bool                                    scroll_movement;
uint16_t                                scroll_counter, move_tile_counter;
int8_t                                  _scroll_x, _scroll_y;

/* Sprites */
OBJ_ATTR                               *debug_colour_n_1, *debug_colour_n_2, *debug_colour_e_1, *debug_colour_e_2, *debug_colour_s_1, *debug_colour_s_2, *debug_colour_w_1, *debug_colour_w_2;
OBJ_ATTR                                obj_buffer[ 128 ];

char demo_text1[][29] = { "This is a test of the", "textbox mechanism. Does it", "work alright? Or could we", "improve it?", "This is a test of the", "textbox mechanism. Does it", "work alright? Or could we", "improve it?" };
//char demo_text1[][29] = { "This is a test of the", "textbox mechanism." };

/*********************************************************************************
	Timer IRQ Functions
*********************************************************************************/
__attribute__ ( ( section( ".iwram" ), long_call ) ) void isr_master();
__attribute__ ( ( section( ".iwram" ), long_call ) ) void timr0_callback();

void timr0_callback( void ) {
	
	/* Increment our ticks */
	ticks.game++;
	ticks.animation++;
	ticks.player++;
}

const fnptr master_isrs[2] = {
	(fnptr) isr_master,
	(fnptr) timr0_callback 
};

/*********************************************************************************
	Game Functions
*********************************************************************************/
void init( void ) {
	
	/*********************************************************************************
		Copy data to VRAM
	*********************************************************************************/
	/* Load texture colour palette */
	tonccpy( pal_bg_mem, _texture_colour_palette, sizeof( _texture_colour_palette ) );
	/* Load sprite colour palette */
	tonccpy( pal_obj_mem, _sprite_colour_palette, sizeof( _sprite_colour_palette ) );

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
	_keys_current = 0;

	/* Initialise our sprites */
	oam_init( obj_buffer, 128 );

	/* Initialise Player */
	player_sprite = &obj_buffer[0];
	set_player_sprite( 0, false );

	/* Initialise Debug Sprites */
	#if d_player_movement
		debug_colour_n_1 = &obj_buffer[2];
		debug_colour_n_2 = &obj_buffer[3];
		debug_colour_e_1 = &obj_buffer[4];
		debug_colour_e_2 = &obj_buffer[5];
		debug_colour_s_1 = &obj_buffer[6];
		debug_colour_s_2 = &obj_buffer[7];
		debug_colour_w_1 = &obj_buffer[8];
		debug_colour_w_2 = &obj_buffer[9];
	#endif

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
	allowed_movement = ( struct dir_en ) { true, true, true, true };

	exit_map.waiting_load = false;
	exit_map.waiting_show = false;
	exit_map.exit_map_id = 0;
	exit_map.exit_map_pos = (struct dir_vec) {0, 0};
  
	/* Initialise textbox variables */
	textbox_running = false;
	textbox_update = false;

	scroll_arrow_count = 0;
	scroll_arrow_offset = 0;
	textbox_line = 0;
	textbox_button_delay = 0;

	write_text_line_count = 0;
	write_text_char_count = 1;

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

	/* Set bitmap mode to 0 and show all backgrounds and show sprites */
	REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_BG2 | DCNT_BG3 | DCNT_OBJ | DCNT_OBJ_1D;

	/* Load the map and set our initial location */
	map_pos = (struct dir_vec) { 2, 4 };
	_current_map = &map_list[1];

	/* Draw the map */
	draw_map_init( true );

	/* Calculate our initial movement restrictions */
	calculate_move_restrictions();

	/* Enable the timer */
	REG_TM0CNT = TM_FREQ_1 | TM_IRQ | TM_ENABLE;

	/* Enabled timer interrupts */
	irq_init(master_isrs[0]);
	irq_add(II_TIMER0, timr0_callback);

	/* Initialise animation variables - none in progress */
	animation.running = false;
}

int main( void ) {

	int16_t _draw_x, _draw_y;

	/* Initialise GBA */
	init();

	/*********************************************************************************
		Game Loop
	*********************************************************************************/
	while(1) {

		/* Poll input keys */
		key_poll();

		/* Development - Refresh display when B is pressed */
		if( key_down( KEY_B ) ) {

		}

		/* Development - Fade display out when A is pressed */
		if( key_down( KEY_A ) ) {

		}

		/*********************************************************************************
			Movement
		*********************************************************************************/
		/* Check input presses as they happen, unless there's an animation running, in which case we don't care */
		if( ( !animation.running ) && ( !scroll_movement ) && ( !textbox_running ) ) {

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

		/* Move the player according to button inputs and movement restrictions */
		if( ( !animation.running ) && ( !textbox_running ) && ( !scroll_movement ) && ( player_movement_delay >= player_movement_delay_val ) ) { // Don't alter movement if an animation is running

			bool start_exit_animation = false;

			/* Player is moving north */
			if( ( player.walk_dir.x == 0 ) && ( player.walk_dir.y == 1 ) ) {

				/* Map exit, start animation and load new map */
				if( exit_tile.travel_n ) {  
					start_exit_animation = true;
				} else {

					/* Move to a new tile, check if they're allowed there */
					if( allowed_movement.travel_n ) {

						/* Start the scroll animation and set the new tile position */
						upcoming_map_pos.y = -1;
						scroll_movement = true;
						player_animation_tick = 0;
					}
				}

			/* Player is moving east */
			} else if( ( player.walk_dir.x == 1 ) && ( player.walk_dir.y == 0 ) ) {
				
				/* Map exit, start animation and load new map */
				if( exit_tile.travel_e ) {
					start_exit_animation = true;
				} else {

					/* Move to a new tile, check if they're allowed there */
					if( allowed_movement.travel_e ) {
						
						/* Start the scroll animation and set the new tile position */
						upcoming_map_pos.x = 1;
						scroll_movement = true;
						player_animation_tick = 0;
					}
				}
			
			/* Player is moving south */
			} else if( ( player.walk_dir.x == 0 ) && ( player.walk_dir.y == -1 ) ) {
			
				/* Map exit, start animation and load new map */
				if( exit_tile.travel_s ) {
					start_exit_animation = true;
				} else {

					/* Move to a new tile, check if they're allowed there */
					if( allowed_movement.travel_s)  {
						
						/* Start the scroll animation and set the new tile position */
						upcoming_map_pos.y = 1;
						scroll_movement = true;
						player_animation_tick = 0;
					}
				}

			/* Player is moving west */
			} else if( ( player.walk_dir.x == -1 ) && ( player.walk_dir.y == 0 ) ) {

				/* Map exit, start animation and load new map */
				if( exit_tile.travel_w ) {
					start_exit_animation = true;
				} else {

					/* Move to a new tile, check if they're allowed there */
					if( allowed_movement.travel_w ) {
						
						/* Start the scroll animation and set the new tile position */
						upcoming_map_pos.x = -1;
						scroll_movement = true;
						player_animation_tick = 0;
					}
				}
			}

			/* Check if it's time to start the fade out animation */
			if( start_exit_animation ) {

				exit_map.waiting_load = true;
				start_animation( fade_screen, 3, 0, true );
			}
		}
		
		/* Stop scrolling motion if we're can't continue, either because of allowed movement or an exit tile */
		if( ( ( ( player.walk_dir.x == 0 ) && ( player.walk_dir.y == 1 ) && ( !allowed_movement.travel_n ) ) || 
			( ( player.walk_dir.x == 1 ) && ( player.walk_dir.y == 0 ) && ( !allowed_movement.travel_e ) ) ||
			( ( player.walk_dir.x == 0 ) && ( player.walk_dir.y == -1 ) && ( !allowed_movement.travel_s ) ) ||
			( ( player.walk_dir.x == -1 ) && ( player.walk_dir.y == 0 ) && ( !allowed_movement.travel_w ) ) ) ||
		  ( ( ( player.walk_dir.x == 0 ) && ( player.walk_dir.y == 1 ) && ( exit_tile.travel_n ) ) || 
			( ( player.walk_dir.x == 1 ) && ( player.walk_dir.y == 0 ) && ( exit_tile.travel_e ) ) ||
			( ( player.walk_dir.x == 0 ) && ( player.walk_dir.y == -1 ) && ( exit_tile.travel_s ) ) ||
			( ( player.walk_dir.x == -1 ) && ( player.walk_dir.y == 0 ) && ( exit_tile.travel_w ) ) )
		) {

			scroll_counter = 0;  
			scroll_movement = false;
			upcoming_map_pos.x = 0;
			upcoming_map_pos.y = 0;
		}

		/*********************************************************************************
			Interactions
		*********************************************************************************/
		if( ( ( ( interaction_tile.travel_n ) && ( player.face_dir.x == 0 ) && ( player.face_dir.y == 1 ) ) ||
			  ( ( interaction_tile.travel_e ) && ( player.face_dir.x == 1 ) && ( player.face_dir.y == 0 ) ) ||
			  ( ( interaction_tile.travel_s ) && ( player.face_dir.x == 0 ) && ( player.face_dir.y == -1 ) ) ||
			  ( ( interaction_tile.travel_w ) && ( player.face_dir.x == -1 ) && ( player.face_dir.y == 0 ) ) ) &&
			  ( key_down( KEY_A ) ) && ( textbox_button_delay == 0 ) && ( !textbox_running ) ) {

			/* User pressed the A Button at an interaction */
			open_textbox( demo_text1, ( sizeof( demo_text1 ) / 29 ) );
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
					if( animation.finished ) {

						/* Is it time to load up the new map or show the one we've loaded */
						if( exit_map.waiting_load ) {

							/* Move the player to the correct position */
							map_pos = (struct dir_vec) { exit_map.exit_map_pos.x, exit_map.exit_map_pos.y };

							/* Set the new map */
							_current_map = &map_list[ exit_map.exit_map_id ];

							/* Render new map whilst the screen is black */
							draw_map_init( true );

							/* Set map as loaded but not shown */
							exit_map.waiting_load = false;
							exit_map.waiting_show = true;

							/* and trigger the fade in animation */
							start_animation( fade_screen, 2, 0, false );

						} else if ( exit_map.waiting_show ) {

							/* Time to show the newly loaded map */
							reset_animation();

							/* Make sure we don't get stuck in the doorway */
							exit_map.waiting_show = false;
							exit_map.exit_map_pos = (struct dir_vec) { 0, 0 };

							/* Calculate our new move restrictions */
							calculate_move_restrictions();
						}
					}
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
				_scroll_x = ( player.walk_dir.x * ( scroll_counter % 8 ) ) + player.walk_dir.x;
				_scroll_y = ( -player.walk_dir.y * ( scroll_counter % 8 ) ) - player.walk_dir.y;

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

					if( move_tile_counter == 2 ) {

						/* Time to stop moving, reset the counters */
						scroll_counter = 0;
						move_tile_counter = 0;

						/* Calculate new movement restrictions */
						calculate_move_restrictions();

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
			if( ( !animation.running ) && ( ( player.walk_dir.x != 0 ) || ( player.walk_dir.y != 0 ) ) ) {

				player_animation_tick++;

				if( player_animation_tick >= 6 ) {
					player_animation_tick = 0;
					player.reverse_walking_render = !player.reverse_walking_render;
				}
			}
		}

		/* Show debugger */
		debugging();

		/* Draw the textbox, if running */
		draw_textbox();

		/* Update player sprite */
		oam_copy( oam_mem, obj_buffer, 9 );

		/* Wait for video sync */
		vid_vsync();
	}
}

void debugging( void ) {

	#if d_player_position
		sprintf( write_text, "P(%d,%d)S(%d,%d)D(%d,%d)F(%d,%d)    ", map_pos.x, map_pos.y, scroll_pos.x, scroll_pos.y, player.walk_dir.x, player.walk_dir.y, player.face_dir.x, player.face_dir.y );
		write_string( write_text, 0, 0, 0 );
	#endif

	#if d_textbox_info
		sprintf( write_text,"R(%d,%d)C(%d/%d)T(%d,%d)D(%d)", textbox_running, textbox_update, textbox_line, total_textbox_lines, write_text_char_count, write_text_line_count, textbox_button_delay );
		write_string( write_text, 0, 0, 0 );
	#endif

	#if d_exit_map_info
		sprintf( write_text, "E(%d,%d,%d)P(%d %d)", exit_map.waiting_load, exit_map.waiting_show, exit_map.exit_map_id, exit_map.exit_map_pos.x, exit_map.exit_map_pos.y );
		write_string( write_text, 0, 1, 0 );
	#endif

	#if d_key_info
		sprintf( write_text, "K(%d,%d,%d,%d,%d,%d)         ", (int)key_down(KEY_UP), (int)key_down(KEY_LEFT), (int)key_down(KEY_DOWN), (int)key_down(KEY_RIGHT), (int)key_down(KEY_A), (int)key_down(KEY_B) );
		write_string( write_text, 0, 1, 0 );
	#endif

	#if d_tick_info
		sprintf( write_text, "D(%d)T(%d)C(%d)", player_movement_delay, player_animation_tick, (int)ticks.game );
		write_string( write_text, 0, 2, 0 );
	#endif

	#if d_animation_info
		sprintf( write_text,"R(%d,%d)F(%d,%d)S(%d,%d)T(%d,%d,%d)", animation.running, animation.reverse, animation.repeat, animation.finished, animation.step, ( ( 0x7 ^ animation.step ) - 2 ), animation.tick, animation.occurance, ( ( animation.tick % animation.occurance ) ? 0 : 1) );
		write_string( write_text, 0, 0, 0 );
	#endif

	#if d_map_rendering_offsets
		sprintf( write_text, "P(%d,%d)O(%d,%d,%d,%d)", map_pos.x, map_pos.y, _screen_offsets.n, _screen_offsets.e, _screen_offsets.s, _screen_offsets.w );
		write_string( write_text, 0, 1, 0 );
	#endif

	#if d_player_movement
		/* Draw coloured squares to indicate player's allowed movement */
		if( exit_tile.travel_n ) {
			
			/* Blue tile since this is an exit tile */
			obj_set_attr( debug_colour_n_1, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 4 ) | ATTR2_PRIO( 2 ) ) );
			obj_set_attr( debug_colour_n_2, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 4 ) | ATTR2_PRIO( 2 ) ) );
		} else {

			if( map_pos.y != 0 ) {

				/* Green tile as we are allow to walk in this direction */
				if(allowed_movement.travel_n) {
					obj_set_attr( debug_colour_n_1, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 2 ) | ATTR2_PRIO( 2 ) ) );
					obj_set_attr( debug_colour_n_2, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 2 ) | ATTR2_PRIO( 2 ) ) );
				} else {
					/* Red tile as we're not allowed to walk in this direction */
					obj_set_attr( debug_colour_n_1, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 0 ) | ATTR2_PRIO( 2 ) ) );
					obj_set_attr( debug_colour_n_2, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 0 ) | ATTR2_PRIO( 2 ) ) );
				}
			} else {

				/* Yellow tile as this is the edge of the map */
				obj_set_attr( debug_colour_n_1, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 6 ) | ATTR2_PRIO( 2 ) ) );
				obj_set_attr( debug_colour_n_2, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 6 ) | ATTR2_PRIO( 2 ) ) );
			}
		}

		if( exit_tile.travel_e ) {

			obj_set_attr( debug_colour_e_1, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 4 ) | ATTR2_PRIO( 2 ) ) );
			obj_set_attr( debug_colour_e_2, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 4 ) | ATTR2_PRIO( 2 ) ) );
		} else {

			if( map_pos.x != ( _current_map->map_width - 2 ) ) {

				if( allowed_movement.travel_e ) {
					obj_set_attr( debug_colour_e_1, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 2 ) | ATTR2_PRIO( 2 ) ) );
					obj_set_attr( debug_colour_e_2, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 2 ) | ATTR2_PRIO( 2 ) ) );
				} else {
					obj_set_attr( debug_colour_e_1, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 0 ) | ATTR2_PRIO( 2 ) ) );
					obj_set_attr( debug_colour_e_2, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 0 ) | ATTR2_PRIO( 2 ) ) );
				}
			} else {
				obj_set_attr( debug_colour_e_1, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 4 ) | ATTR2_PRIO( 2 ) ) );
				obj_set_attr( debug_colour_e_2, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 4 ) | ATTR2_PRIO( 2 ) ) );
			}
		}

		if( exit_tile.travel_s ) {

			obj_set_attr( debug_colour_s_1, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 4 ) | ATTR2_PRIO( 2 ) ) );
			obj_set_attr( debug_colour_s_2, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 4 ) | ATTR2_PRIO( 2 ) ) );
		} else {

			if( map_pos.y != ( _current_map->map_height - 2 ) ) {

				if( allowed_movement.travel_s ) {
					obj_set_attr( debug_colour_s_1, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 2 ) | ATTR2_PRIO( 2 ) ) );
					obj_set_attr( debug_colour_s_2, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 2 ) | ATTR2_PRIO( 2 ) ) );
				} else {
					obj_set_attr( debug_colour_s_1, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 0 ) | ATTR2_PRIO( 2 ) ) );
					obj_set_attr( debug_colour_s_2, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 0 ) | ATTR2_PRIO( 2 ) ) );
				}
			} else {
				obj_set_attr( debug_colour_s_1, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 6 ) | ATTR2_PRIO( 2 ) ) );
				obj_set_attr( debug_colour_s_2, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 6 ) | ATTR2_PRIO( 2 ) ) );
			}
		}

		if( exit_tile.travel_w ) {

			obj_set_attr( debug_colour_w_1, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 4 ) | ATTR2_PRIO( 2 ) ) );
			obj_set_attr( debug_colour_w_2, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 4 ) | ATTR2_PRIO( 2 ) ) );
		} else {

			if( map_pos.x != 0 ) {

				if( allowed_movement.travel_w ) {
					obj_set_attr( debug_colour_w_1, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 2 ) | ATTR2_PRIO( 2 ) ) );
					obj_set_attr( debug_colour_w_2, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 2 ) | ATTR2_PRIO( 2 ) ) );
				} else {
					obj_set_attr( debug_colour_w_1, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 0 ) | ATTR2_PRIO( 2 ) ) );
					obj_set_attr( debug_colour_w_2, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 0 ) | ATTR2_PRIO( 2 ) ) );
				}
			} else {
				obj_set_attr( debug_colour_w_1, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 6 ) | ATTR2_PRIO( 2 ) ) );
				obj_set_attr( debug_colour_w_2, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_8 ), ( ATTR2_ID( 6 ) | ATTR2_PRIO( 2 ) ) );
			}
		}

		/* Set positions */
		obj_set_pos( debug_colour_n_1, 112, 64 );
		obj_set_pos( debug_colour_n_2, 120, 64 );
		obj_set_pos( debug_colour_e_1, 128, 72 );
		obj_set_pos( debug_colour_e_2, 128, 80 );
		obj_set_pos( debug_colour_s_1, 112, 88 );
		obj_set_pos( debug_colour_s_2, 120, 88 );
		obj_set_pos( debug_colour_w_1, 104, 72 );
		obj_set_pos( debug_colour_w_2, 104, 80 );
	#endif

	#if d_interaction_info
		if( ( interaction_tile.travel_n ) && ( player.face_dir.x == 0 ) && ( player.face_dir.y == 1 ) ) {
			sprintf( write_text,"ID(%d)", interaction_tile_id[0] );
			write_string( write_text, 0, 0, 0 );
		} else if( ( interaction_tile.travel_e ) && ( player.face_dir.x == 1 ) && ( player.face_dir.y == 0 ) ) {
			sprintf( write_text,"ID(%d)", interaction_tile_id[1] );
			write_string( write_text, 0, 0, 0 );
		} else if( ( interaction_tile.travel_s ) && ( player.face_dir.x == 0 ) && ( player.face_dir.y == -1 ) ) {
			sprintf( write_text,"ID(%d)", interaction_tile_id[2] );
			write_string( write_text, 0, 0, 0 );
		} else if( ( interaction_tile.travel_w ) && ( player.face_dir.x == -1 ) && ( player.face_dir.y == 0 ) ) {
			sprintf( write_text,"ID(%d)", interaction_tile_id[3] );
			write_string( write_text, 0, 0, 0 );
		} else {
			sprintf( write_text,"          " );
			write_string( write_text, 0, 0, 0 );
		}
	#endif
	
	#if d_map_coordinates
		uint8_t i;
		for( i = 0; i < 10; i++ ) {
			write_character( ( 48 + i ), 0, i );
			write_character( ( 48 + i ), 0, ( i + 10 ) );
		}
		sprintf( write_text, "012345678901234567890123456789" );
		write_string( write_text, 0, 19, 0 );
	#endif
}

void calculate_move_restrictions( void ) {

	const struct map_tile (*current_map_tile_ptr) = _current_map->map_tiles_ptr; /* Get a pointer to the current tile */
	current_map_tile_ptr += map_pos.y * _current_map->map_width; /* Move to the correct y value */
	current_map_tile_ptr += map_pos.x; /* Move to the correct x value */

	/* See if we're stood on an exit tile or not */
	exit_tile = ( struct dir_en ) { false, false, false, false };

	/* We're stood on an exit tile, let's check which direction the exit is */
	if( (*current_map_tile_ptr).exit_tile ) {

		if( (*current_map_tile_ptr).exit_map_dir.y == 1 )
			exit_tile.travel_n = true;

		if( (*current_map_tile_ptr).exit_map_dir.x == -1 )
			exit_tile.travel_w = true;

		/* Now store the details of the map to load if we exit this one */
		exit_map.exit_map_id = (*current_map_tile_ptr).exit_map_id;
		exit_map.exit_map_pos = (*current_map_tile_ptr).exit_map_pos;
	}

	/* Move the pointer right 1 tile otherwise we won't see exit tiles to the east */
	current_map_tile_ptr++;
	if( (*current_map_tile_ptr).exit_tile ) {

		if( (*current_map_tile_ptr).exit_map_dir.x == 1 )
			exit_tile.travel_e = true;

		/* Now store the details of the map to load if we exit this one */
		exit_map.exit_map_id = (*current_map_tile_ptr).exit_map_id;
		exit_map.exit_map_pos = (*current_map_tile_ptr).exit_map_pos;
	}
	current_map_tile_ptr--;

	/* Move the pointer down 1 tile otherwise we won't see exit tiles to the south */
	current_map_tile_ptr += _current_map->map_width;
	if( (*current_map_tile_ptr).exit_tile ) {

		if( (*current_map_tile_ptr).exit_map_dir.y == -1 )
			exit_tile.travel_s = true;

		/* Now store the details of the map to load if we exit this one */
		exit_map.exit_map_id = (*current_map_tile_ptr).exit_map_id;
		exit_map.exit_map_pos = (*current_map_tile_ptr).exit_map_pos;
	}
	current_map_tile_ptr -= _current_map->map_width;

	/* Store which directions we're allowed to walk in the nearest tiles and if there are any interaction tiles */
	allowed_movement = ( struct dir_en ) { false, false, false, false };
	interaction_tile = ( struct dir_en ) { false, false, false, false };

	if( map_pos.y != 0 ) {

		current_map_tile_ptr -= _current_map->map_width; // Move pointer up a row
		
		allowed_movement.travel_n = (*current_map_tile_ptr).can_walk_s;
		interaction_tile.travel_n = (*current_map_tile_ptr).interact_tile;

		if( interaction_tile.travel_n ) interaction_tile_id[0] = (*current_map_tile_ptr).interact_id;
		else interaction_tile_id[0] = 0;

		current_map_tile_ptr += _current_map->map_width; // Move pointer back to same row as player
	}
	if( map_pos.x != ( _current_map->map_width - 2 ) ) {

		current_map_tile_ptr += 2; // Move pointer to 2 columns along

		allowed_movement.travel_e = (*current_map_tile_ptr).can_walk_w;
		interaction_tile.travel_e = (*current_map_tile_ptr).interact_tile;

		if( interaction_tile.travel_e ) interaction_tile_id[1] = (*current_map_tile_ptr).interact_id;
		else interaction_tile_id[1] = 0;

		current_map_tile_ptr -= 2; // Move pointer back to same column as player
	}
	if( map_pos.y != ( _current_map->map_height - 2 ) ) {

		current_map_tile_ptr += ( _current_map->map_width * 2); // Move pointer down 2 rows

		allowed_movement.travel_s = (*current_map_tile_ptr).can_walk_n;
		interaction_tile.travel_s = (*current_map_tile_ptr).interact_tile;

		if( interaction_tile.travel_s ) interaction_tile_id[2] = (*current_map_tile_ptr).interact_id;
		else interaction_tile_id[2] = 0;

		current_map_tile_ptr -= ( _current_map->map_width * 2); // Move pointer back to same row as player
	}
	if( map_pos.x != 0 ) {

		current_map_tile_ptr--; // Move pointer back a column

		allowed_movement.travel_w = (*current_map_tile_ptr).can_walk_e;
		interaction_tile.travel_w = (*current_map_tile_ptr).interact_tile;

		if( interaction_tile.travel_w ) interaction_tile_id[3] = (*current_map_tile_ptr).interact_id;
		else interaction_tile_id[3] = 0;

		current_map_tile_ptr++; // Move pointer back to same column as player
	}

	/* Add movement restrictions based on interaction tiles */
	if( interaction_tile.travel_n ) allowed_movement.travel_n = false;
	if( interaction_tile.travel_e ) allowed_movement.travel_e = false;
	if( interaction_tile.travel_s ) allowed_movement.travel_s = false;
	if( interaction_tile.travel_w ) allowed_movement.travel_w = false;
}

void calculate_map_offsets( void ) {

	/* Calculate how many empty tiles we need to display around the map shown on screen */
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

void draw_map_tile( int16_t _draw_x, int16_t _draw_y, const struct map_tile ( *map_tiles_ptr ) ) {

	int16_t _texture_offset, i;

	/* Check if we have a background texture to draw, and if so add it to the background layer */
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

			/* Top Layer texture, draw to the top layer map and clear foreground layer */
			set_tile( _draw_x, _draw_y, _texture_offset, (*map_tiles_ptr).texture_reverse_x, (*map_tiles_ptr).texture_reverse_y, 18 );
			set_tile( _draw_x, _draw_y, 0, 0, 0, 17 ); // Foreground Layer - transparent
		} else {

			/* Bottom Layer texture, draw to the normal map and clear top layer */
			set_tile( _draw_x, _draw_y, _texture_offset, (*map_tiles_ptr).texture_reverse_x, (*map_tiles_ptr).texture_reverse_y, 17 );
			set_tile( _draw_x, _draw_y, 0, 0, 0, 18 ); // Top Layer - transparent
		}
	} else {

		/* If not, draw a transparent tile to ensure we're not showing old tiles in the buffer */
		set_tile( _draw_x, _draw_y, 0, 0, 0, 17 ); // Foreground Layer - transparent
		set_tile( _draw_x, _draw_y, 0, 0, 0, 18 ); // Top Layer - transparent
	}
}

void draw_map_init( bool reset_scroll ) {

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

	/* Do we need to reset the scroll */
	if( reset_scroll ) {
		
		/* Reset the scroll position variable */
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
}

void set_player_sprite( uint8_t offset, bool flip_h ) {

	/* Update the offset of the player sprite */
	obj_set_attr( player_sprite, ( ATTR0_SQUARE | ATTR0_8BPP), ( ATTR1_SIZE_16 | ( flip_h ? ATTR1_HFLIP : 0 ) ), ( ATTR2_ID( ( 1 + offset ) * 8 ) | ATTR2_PRIO( 1 ) ) );
	obj_set_pos( player_sprite, 112, 72 );
}


