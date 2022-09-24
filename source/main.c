#include "project.h"

#include "demo_project.h"
#include "font.h"
#include "text.h"

#include "tonc_utils.h"
#include "tonc_irq.h"


/*********************************************************************************
	Global Variables
*********************************************************************************/

__attribute__((section(".iwram"), long_call)) void isr_master();
__attribute__((section(".iwram"), long_call)) void hbl_grad_direct();

/* Map */
struct map *_current_map;

/* Player movement variables */
struct dir_vec map_pos;
struct offset_vec _screen_offsets;

char write_text[128];

typedef enum eKeyIndex
{
	KI_A=0,
	KI_B,
	KI_SELECT,
	KI_START, 
	KI_RIGHT,
	KI_LEFT,
	KI_UP,
	KI_DOWN,
	KI_R,
	KI_L,
	KI_MAX
} eKeyIndex;

#define KEY_FULL	0xFFFFFFFF		//!< Define for checking all keys.

// --- Reg screen entries ----------------------------------------------

#define SE_HFLIP		0x0400	//!< Horizontal flip
#define SE_VFLIP		0x0800	//!< Vertical flip

#define SE_ID_MASK		0x03FF
#define SE_ID_SHIFT			 0
#define SE_ID(n)		((n)<<SE_ID_SHIFT)

#define SE_FLIP_MASK	0x0C00
#define SE_FLIP_SHIFT		10
#define SE_FLIP(n)	 ((n)<<SE_FLIP_SHIFT)

#define SE_PALBANK_MASK		0xF000
#define SE_PALBANK_SHIFT		12
#define SE_PALBANK(n)		((n)<<SE_PALBANK_SHIFT)


#define SE_BUILD(id, pbank, hflip, vflip)	\
( ((id)&0x03FF) | (((hflip)&1)<<10) | (((vflip)&1)<<11) | ((pbank)<<12) )



struct REPEAT_REC
{
	uint16_t	keys;	// Repeated keys
	uint16_t	mask;	// Only check repeats for these keys
	uint8_t		count;	// Repeat counter
	uint8_t		delay;	// Limit for first repeat
	uint8_t		repeat;	// Limit for successive repeats
};
struct REPEAT_REC __key_rpt = { 0, KEY_MASK, 60, 60, 30 };
uint16_t __key_curr = 0, __key_prev = 0;

static inline int bit_tribool(u32 flags, uint plus, uint minus)
{	return ((flags>>plus)&1) - ((flags>>minus)&1);	}

static inline int key_tri_horz(void)		
{	return bit_tribool(__key_curr, KI_RIGHT, KI_LEFT);	}

//! Vertical tribool (down,up)=(+,-)
static inline int key_tri_vert(void)		
{	return bit_tribool(__key_curr, KI_DOWN, KI_UP);		}

//! Shoulder-button tribool (R,L)=(+,-)
static inline int key_tri_shoulder(void)	
{	return bit_tribool(__key_curr, KI_R, KI_L);			}

//! Fire-button tribool (A,B)=(+,-)
static inline int key_tri_fire(void)		
{	return bit_tribool(__key_curr, KI_A, KI_B);			}

static inline uint32_t key_transit(u32 key)
{	return ( __key_curr ^  __key_prev) & key;	}

// Key is held (down now and before).
static inline uint32_t key_held(u32 key)
{   return ( __key_curr &  __key_prev) & key;  }

// Key is being hit (down now, but not before).
static inline uint32_t key_hit(u32 key)
{   return ( __key_curr &~ __key_prev) & key;  }

//! Wait for next VBlank
static inline void vid_vsync() {
	while(REG_VCOUNT >= 160);   // wait till VDraw
	while(REG_VCOUNT < 160);    // wait till VBlank
}

static inline void set_tile( uint16_t x, uint16_t y, uint16_t tile, uint8_t screenblock ) {
	se_mem[ screenblock ][ x + ( y * 32 ) ] = tile;
}

uint16_t rgb(uint32_t colour) {

	/* Converts 32-bit RGB to 15-bit BGR */
	return 0x0 | ( ( ( ( ( colour >> 16 ) & 0xFF) / 8 ) << 0 ) | ( ( ( ( colour >> 8 ) & 0xFF) / 8 ) << 5 ) | ( ( ( colour & 0xFF) / 8 ) << 10 ) );
}

void key_poll(void)
{
	__key_prev= __key_curr;
	__key_curr= ~REG_KEYINPUT & KEY_MASK;

	struct REPEAT_REC *rpt= &__key_rpt;

	rpt->keys= 0;	// Clear repeats again

	if(rpt->delay)
	{
		// Change in masked keys: reset repeat
		// NOTE: this also counts as a repeat!
		if(key_transit(rpt->mask))
		{
			rpt->count= rpt->delay;
			rpt->keys= __key_curr;
		}
		else
			rpt->count--;

		// Time's up: set repeats (for this frame)
		if(rpt->count == 0)
		{
			rpt->count= rpt->repeat;
			rpt->keys= __key_curr & rpt->mask;
		}
	}
}

void update_map() {

	int16_t _draw_x, _draw_y;

	// w = 24, h = 14

	REG_BG0HOFS = 8;
	REG_BG0VOFS = 8;

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
	
	/* Draw where the player will go */
	set_tile( 15, 10, 0x4, 16 );
	set_tile( 16, 10, 0x5, 16 );
	set_tile( 15, 11, 0x6, 16 );
	set_tile( 16, 11, 0x7, 16 );
}

uint32_t _count;

void hbl_grad_routed()
{
	_count++;
}

const fnptr master_isrs[2]= 
{
	(fnptr)isr_master,
	(fnptr)hbl_grad_direct 
};

int main()
{
	_count = 0;

	/* Load colour palette */
	memcpy( pal_bg_mem, _colour_palette, sizeof( _colour_palette ) );

	/* Load font into VRAM charblock 0 (memory address 0x0600:0000) */
	memcpy( &tile_mem[0][0], _font_map, sizeof( _font_map ) );
	/* Load tileset into VRAM charblock 1 (memory address 0x0600:4000) */
	memcpy( &tile_mem[1][0], _texture_map, sizeof( _texture_map ) );

	/* Clear background map at screenblocks 16 and 17 (memory address 0x0600:8000 and 0x0600:8800) */
	toncset16( &se_mem[16][0], 0, 4096 );
	/* Clear debug overlay at screenblock 18 (memory address 0x0600:9000) */
	toncset16( &se_mem[18][0], 0, 2048 );

	/* Configure BG0 using charblock 1 and screenblock 16 and 17 - background */
	REG_BG0CNT = BG_CBB(1) | BG_SBB(16) | BG_8BPP | BG_REG_32x32 | BG_PRIO(1);
	/* Configure BG1 using charblock 0 and screenblock 18 - debug overlay */
	REG_BG1CNT = BG_CBB(0) | BG_SBB(18) | BG_8BPP | BG_REG_32x32 | BG_PRIO(0);

	/* Set bitmap mode to 0 and show backgrounds 0 and 1 */
	REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1;

	/* Load the map and set our initial location */
	_current_map = &map_list[0];
	map_pos = (struct dir_vec){ 0, 0 };

	update_map();

	//REG_TM0D = 1;
	REG_TM0CNT = TM_FREQ_1 | TM_IRQ | TM_ENABLE;

	/* Enabled timer interrupts */
	irq_init(master_isrs[0]);
	irq_add(II_TIMER0, hbl_grad_routed);

	while(1) {

		/* Debug */
		sprintf( write_text, "012345678901234567890123456789" );
		write_string( write_text, 0, 0, 0 );
		sprintf( write_text, "C(%d)", (int)_count );
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


		vid_vsync();
		key_poll();
	}

}
