#ifndef PROJECT_H_INCLUDED
#define PROJECT_H_INCLUDED

/* Include GBA sources */
#include <gba_console.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef void (*fnptr)(void);

/* Include Tonc sources: https://www.coranac.com/tonc/text/ */
#include "tonc/tonc_utils.h"
#include "tonc/tonc_irq.h"

// This always sets the top left corner tile of the display:
//set_tile( g_mod( scroll_pos.x, 32 ), g_mod( scroll_pos.y, 32 ), 0, 0, 0, 17 );

/*********************************************************************************
	Debugging Definitions
*********************************************************************************/
#define d_map_coordinates                 false	// Displays the coordinates of the display
#define d_map_rendering_offsets           false	// Displays player position on map and no of calculated empty rows & columns (for initial draw)
#define d_player_position                 false	// Displays player position
#define d_player_movement                 false	// Draws coloured boxes to indicate if player can move or if there are obstacles, edge of map or exit tiles
#define d_interaction_info                false	// Displays information about interaction tiles
#define d_exit_map_info                   false	// Displays information about the map to load if exiting current map
#define d_animation_info                  false	// Displays information about the running animation
#define d_textbox_info                    false	// Displays information about the current textbox
#define d_key_info                        false	// Displays information about the keys being pressed
#define d_tick_info                       false	// Displays various timer ticks
#define d_npc_info                        false	// Displays information about the NPC

/*********************************************************************************
	Memory Definitions
*********************************************************************************/
#define block_background_layer		27
#define block_foreground_layer		28
#define block_top_layer				29
#define block_text_layer			30

/*********************************************************************************
	Register Definitions
*********************************************************************************/
#define MEM_IO				0x04000000
#define MEM_PAL				0x05000000
#define MEM_VRAM			0x06000000
#define MEM_OAM				0x07000000

#define PAL_SIZE			0x00400
#define VRAM_SIZE			0x18000

#define PAL_BG_SIZE			0x00200
#define PAL_OBJ_SIZE		0x00200
#define M3_SIZE				0x12C00
#define M4_SIZE				0x09600
#define M5_SIZE				0x0A000
#define CBB_SIZE			0x04000
#define SBB_SIZE			0x00800
#define VRAM_BG_SIZE		0x10000

#define MEM_PAL_OBJ			(MEM_PAL + PAL_BG_SIZE)

/* REG_DISPCNT */
#define DCNT_MODE0			0x0000	// Mode 0; bg 0-4: reg
#define DCNT_MODE1			0x0001	// Mode 1; bg 0-1: reg; bg 2: affine
#define DCNT_MODE2			0x0002	// Mode 2; bg 2-2: affine
#define DCNT_MODE3			0x0003	// Mode 3; bg2: 240x160\@16 bitmap
#define DCNT_MODE4			0x0004	// Mode 4; bg2: 240x160\@8 bitmap
#define DCNT_MODE5			0x0005	// Mode 5; bg2: 160x128\@16 bitmap
#define DCNT_GB				0x0008	// (R) GBC indicator
#define DCNT_PAGE			0x0010	// Page indicator
#define DCNT_OAM_HBL		0x0020	// Allow OAM updates in HBlank
#define DCNT_OBJ_2D			0x0000	// OBJ-VRAM as matrix
#define DCNT_OBJ_1D			0x0040	// OBJ-VRAM as array
#define DCNT_BLANK			0x0080	// Force screen blank
#define DCNT_BG0			0x0100	// Enable bg 0
#define DCNT_BG1			0x0200	// Enable bg 1
#define DCNT_BG2			0x0400	// Enable bg 2
#define DCNT_BG3			0x0800	// Enable bg 3
#define DCNT_OBJ			0x1000	// Enable objects
#define DCNT_WIN0			0x2000	// Enable window 0
#define DCNT_WIN1			0x4000	// Enable window 1
#define DCNT_WINOBJ			0x8000	// Enable object window

/* REG_BGxCNT */
#define BG_MOSAIC			0x0040	// Enable Mosaic
#define BG_4BPP				0x0000	// 4bpp (16 color) bg (no effect on affine bg)
#define BG_8BPP				0x0080	// 8bpp (256 color) bg (no effect on affine bg)
#define BG_WRAP				0x2000	// Wrap around edges of affine bgs
#define BG_SIZE0			0x0000
#define BG_SIZE1			0x4000
#define BG_SIZE2			0x8000
#define BG_SIZE3			0xC000
#define BG_REG_32x32		0x0000	// Reg bg, 32x32 (256x256 px)
#define BG_REG_64x32		0x4000	// Reg bg, 64x32 (512x256 px)
#define BG_REG_32x64		0x8000	// Reg bg, 32x64 (256x512 px)
#define BG_REG_64x64		0xC000	// Reg bg, 64x64 (512x512 px)
#define BG_AFF_16x16		0x0000	// Affine bg, 16x16 (128x128 px)
#define BG_AFF_32x32		0x4000	// Affine bg, 32x32 (256x256 px)
#define BG_AFF_64x64		0x8000	// Affine bg, 64x64 (512x512 px)
#define BG_AFF_128x128		0xC000	// Affine bg, 128x128 (1024x1024 px)

#define BG_PRIO_MASK		0x0003
#define BG_PRIO_SHIFT		0x0000
#define BG_PRIO(n)			( n << BG_PRIO_SHIFT )

#define BG_CBB_MASK			0x000C
#define BG_CBB_SHIFT		0x0002
#define BG_CBB(n)			( n << BG_CBB_SHIFT )

#define BG_SBB_MASK			0x1F00
#define BG_SBB_SHIFT		0x0008
#define BG_SBB(n)			( n << BG_SBB_SHIFT )

/* OAM */
// oatr_mem[a] = OBJ_ATTR (oam entry a)
#define oam_mem				( (OBJ_ATTR*) MEM_OAM )
#define obj_mem				( (OBJ_ATTR*) MEM_OAM )
#define obj_aff_mem			( (OBJ_AFFINE*) MEM_OAM )

/* OAM attribute 0 */
#define ATTR0_REG			0x0000	// Regular object
#define ATTR0_AFF			0x0100	// Affine object
#define ATTR0_HIDE			0x0200	// Inactive object
#define ATTR0_AFF_DBL		0x0300	// Double-size affine object
#define ATTR0_AFF_DBL_BIT	0x0200
#define ATTR0_BLEND			0x0400	// Enable blend
#define ATTR0_WINDOW		0x0800	// Use for object window
#define ATTR0_MOSAIC		0x1000	// Enable mosaic
#define ATTR0_4BPP			0x0000	// Use 4bpp (16 color) tiles
#define ATTR0_8BPP			0x2000	// Use 8bpp (256 color) tiles
#define ATTR0_SQUARE		0x0000	// Square shape
#define ATTR0_WIDE			0x4000	// Tall shape (height &gt; width)
#define ATTR0_TALL			0x8000	// Wide shape (height &lt; width)

#define ATTR0_Y_MASK		0x00FF
#define ATTR0_Y_SHIFT		0x0000
#define ATTR0_Y(n)			( ( n ) << ATTR0_Y_SHIFT )

#define ATTR0_MODE_MASK		0x0300
#define ATTR0_MODE_SHIFT	0x0008
#define ATTR0_MODE(n)		( ( n ) << ATTR0_MODE_SHIFT )

#define ATTR0_SHAPE_MASK	0xC000
#define ATTR0_SHAPE_SHIFT	0x000E
#define ATTR0_SHAPE(n)		( ( n ) << ATTR0_SHAPE_SHIFT )

#define ATTR0_BUILD(y, shape, bpp, mode, mos, bld, win)				\
(																	\
	((y)&255) | (((mode)&3)<<8) | (((bld)&1)<<10) | (((win)&1)<<11) \
	| (((mos)&1)<<12) | (((bpp)&8)<<10)| (((shape)&3)<<14)			\
)

/* OAM attribute 1 */
#define ATTR1_HFLIP			0x1000	// Horizontal flip (reg obj only)
#define ATTR1_VFLIP			0x2000	// Vertical flip (reg obj only)
// Base sizes
#define ATTR1_SIZE_8		0x0000
#define ATTR1_SIZE_16		0x4000
#define ATTR1_SIZE_32		0x8000
#define ATTR1_SIZE_64		0xC000
// Square sizes
#define ATTR1_SIZE_8x8		0x0000	// Size flag for 8x8 px object
#define ATTR1_SIZE_16x16	0x4000	// Size flag for 16x16 px object
#define ATTR1_SIZE_32x32	0x8000	// Size flag for 32x32 px object
#define ATTR1_SIZE_64x64	0xC000	// Size flag for 64x64 px object
// Tall sizes
#define ATTR1_SIZE_8x16		0x0000	// Size flag for 8x16 px object
#define ATTR1_SIZE_8x32		0x4000	// Size flag for 8x32 px object
#define ATTR1_SIZE_16x32	0x8000	// Size flag for 16x32 px object
#define ATTR1_SIZE_32x64	0xC000	// Size flag for 32x64 px object
// Wide sizes
#define ATTR1_SIZE_16x8		0x0000	// Size flag for 16x8 px object
#define ATTR1_SIZE_32x8		0x4000	// Size flag for 32x8 px object
#define ATTR1_SIZE_32x16	0x8000	// Size flag for 32x16 px object
#define ATTR1_SIZE_64x32	0xC000	// Size flag for 64x64 px object

#define ATTR1_X_MASK		0x01FF
#define ATTR1_X_SHIFT		0x0000
#define ATTR1_X(n)			( ( n ) << ATTR1_X_SHIFT )

#define ATTR1_AFF_ID_MASK	0x3E00
#define ATTR1_AFF_ID_SHIFT	0x0009
#define ATTR1_AFF_ID(n)		( ( n ) << ATTR1_AFF_ID_SHIFT )

#define ATTR1_FLIP_MASK		0x3000
#define ATTR1_FLIP_SHIFT	0x000C
#define ATTR1_FLIP(n)		( ( n ) << ATTR1_FLIP_SHIFT )

#define ATTR1_SIZE_MASK		0xC000
#define ATTR1_SIZE_SHIFT	0x000E
#define ATTR1_SIZE(n)		( ( n ) << ATTR1_SIZE_SHIFT )

#define ATTR1_BUILDR(x, size, hflip, vflip)	\
( ((x)&511) | (((hflip)&1)<<12) | (((vflip)&1)<<13) | (((size)&3)<<14) )

#define ATTR1_BUILDA(x, size, affid)			\
( ((x)&511) | (((affid)&31)<<9) | (((size)&3)<<14) )


/* OAM attribute 2 */
#define ATTR2_ID_MASK		0x03FF
#define ATTR2_ID_SHIFT		0x0000
#define ATTR2_ID(n)			( ( n ) << ATTR2_ID_SHIFT )

#define ATTR2_PRIO_MASK		0x0C00
#define ATTR2_PRIO_SHIFT	0x000A
#define ATTR2_PRIO(n)		( ( n ) << ATTR2_PRIO_SHIFT )

#define ATTR2_PALBANK_MASK	0xF000
#define ATTR2_PALBANK_SHIFT	0x000C
#define ATTR2_PALBANK(n)	( ( n ) << ATTR2_PALBANK_SHIFT )

#define ATTR2_BUILD(id, pbank, prio)			\
( ((id)&0x3FF) | (((pbank)&15)<<12) | (((prio)&3)<<10) )

/* REG_TMxCNT */
#define REG_TM				( (volatile TMR_REC*)( REG_BASE + 0x0100 ) )	// Timers as TMR_REC array

#define REG_TM0D			*(volatile uint16_t*)( REG_BASE + 0x0100 )					// Timer 0 data
#define REG_TM0CNT			*(volatile uint16_t*)( REG_BASE + 0x0102 )					// Timer 0 control
#define REG_TM1D			*(volatile uint16_t*)( REG_BASE + 0x0104 )					// Timer 1 data
#define REG_TM1CNT			*(volatile uint16_t*)( REG_BASE + 0x0106 )					// Timer 1 control
#define REG_TM2D			*(volatile uint16_t*)( REG_BASE + 0x0108 )					// Timer 2 data
#define REG_TM2CNT			*(volatile uint16_t*)( REG_BASE + 0x010A )					// Timer 2 control
#define REG_TM3D			*(volatile uint16_t*)( REG_BASE + 0x010C )					// Timer 3 data
#define REG_TM3CNT			*(volatile uint16_t*)( REG_BASE + 0x010E )					// Timer 3 control

#define TM_FREQ_SYS			0x0000	// System clock timer (16.7 Mhz)
#define TM_FREQ_1			0x0000	// 1 cycle/tick (16.7 Mhz)
#define TM_FREQ_64			0x0001	// 64 cycles/tick (262 kHz)
#define TM_FREQ_256			0x0002	// 256 cycles/tick (66 kHz)
#define TM_FREQ_1024		0x0003	// 1024 cycles/tick (16 kHz)
#define TM_CASCADE			0x0004	// Increment when preceding timer overflows
#define TM_IRQ				0x0040	// Enable timer irq
#define TM_ENABLE			0x0080	// Enable timer

#define TM_FREQ_MASK		0x0003
#define TM_FREQ_SHIFT		0x0000
#define TM_FREQ(n)			( ( n ) << TM_FREQ_SHIFT )

/* Interrupt Regisers */
#define REG_WAITCNT			*(volatile uint16_t*)( REG_BASE + 0x0204 )	// Waitstate control
#define REG_PAUSE			*(volatile uint16_t*)( REG_BASE + 0x0300 )	// Pause system (?)
#define REG_IFBIOS			*(volatile uint16_t*)( REG_BASE - 0x0008 )	// IRQ ack for IntrWait functions
#define REG_RESET_DST		*(volatile uint16_t*)( REG_BASE - 0x0006 )	// Destination for after SoftReset
#define REG_ISR_MAIN		*(fnptr*)( REG_BASE - 0x0004 )				// IRQ handler address

/* REG_IE, REG_IF, REG_IF_BIOS */
typedef void (*fnptr)(void);

#define IRQ_VBLANK			0x0001	// Catch VBlank irq
#define IRQ_HBLANK			0x0002	// Catch HBlank irq
#define IRQ_VCOUNT			0x0004	// Catch VCount irq
#define IRQ_TIMER0			0x0008	// Catch timer 0 irq
#define IRQ_TIMER1			0x0010	// Catch timer 1 irq
#define IRQ_TIMER2			0x0020	// Catch timer 2 irq
#define IRQ_TIMER3			0x0040	// Catch timer 3 irq
#define IRQ_SERIAL			0x0080	// Catch serial comm irq
#define IRQ_DMA0			0x0100	// Catch DMA 0 irq
#define IRQ_DMA1			0x0200	// Catch DMA 1 irq
#define IRQ_DMA2			0x0400	// Catch DMA 2 irq
#define IRQ_DMA3			0x0800	// Catch DMA 3 irq
#define IRQ_KEYPAD			0x1000	// Catch key irq
#define IRQ_GAMEPAK			0x2000	// Catch cart irq

/* Button Input Registers */
#define KEY_A				0x0001	// Button A
#define KEY_B				0x0002	// Button B
#define KEY_SELECT			0x0004	// Select button
#define KEY_START			0x0008	// Start button
#define KEY_RIGHT			0x0010	// Right D-pad
#define KEY_LEFT			0x0020	// Left D-pad
#define KEY_UP				0x0040	// Up D-pad
#define KEY_DOWN			0x0080	// Down D-pad
#define KEY_R				0x0100	// Shoulder R
#define KEY_L				0x0200	// Shoulder L

#define KEY_ANY				0x03FF	// Any key
#define KEY_DIR				0x00F0	// Any-dpad
#define KEY_SHOULDER		0x0300	// L or R

#define KEY_RESET			0x000F	// St+Se+A+B
#define KEY_MASK			0x03FF

/*********************************************************************************
	VRAM Definitions
*********************************************************************************/
#define SCREEN_WIDTH		240
#define SCREEN_HEIGHT		160

typedef uint16_t			COLOR;
typedef uint16_t			SCR_ENTRY, SE;
typedef struct {
	uint32_t data[8];
} TILE, TILE4;
typedef struct {
	uint32_t data[16];
} TILE8;

typedef SCR_ENTRY			SCREENBLOCK[1024];
typedef TILE				CHARBLOCK[512];
typedef TILE8				CHARBLOCK8[256];

#define tile_mem			( (CHARBLOCK*) MEM_VRAM )
#define tile8_mem			( (CHARBLOCK8*) MEM_VRAM )

#define se_mem				( (SCREENBLOCK*) MEM_VRAM )

#define pal_bg_mem			( (COLOR*) MEM_PAL )
#define pal_obj_mem			( (COLOR*) MEM_PAL_OBJ )

/* Reg screen entries */
#define SE_HFLIP			0x0400	// Horizontal flip
#define SE_VFLIP			0x0800	// Vertical flip

#define SE_ID_MASK			0x03FF
#define SE_ID_SHIFT			0x0000
#define SE_ID(n)			( ( n ) << SE_ID_SHIFT )

#define SE_FLIP_MASK		0x0C00
#define SE_FLIP_SHIFT		0x000A
#define SE_FLIP(n)	 		( ( n ) << SE_FLIP_SHIFT )

#define SE_PALBANK_MASK		0xF000
#define SE_PALBANK_SHIFT	0x000C
#define SE_PALBANK(n)		( ( n ) << SE_PALBANK_SHIFT )

#define SE_BUILD(id, pbank, hflip, vflip)	\
( ((id)&0x03FF) | (((hflip)&1)<<10) | (((vflip)&1)<<11) | ((pbank)<<12) )

/* Wait for next VBlank */
static inline void vid_vsync() {
	while( REG_VCOUNT >= 160 );		// wait till VDraw
	while( REG_VCOUNT < 160 );		// wait till VBlank
}

/*********************************************************************************
	Tonc bit macros
*********************************************************************************/
#define BIT_SHIFT(a, n)			( (a)<<(n) )
#define BIT_SET(word, flag)		( word |=  (flag) )
#define BIT_CLEAR(word, flag)	( word &= ~(flag) )
#define BIT_FLIP(word, flag)	( word ^=  (flag) )
#define BIT_EQ(word, flag)		( ((word)&(flag)) == (flag) )

#define BFN_PREP(x, name)		( ((x)<<name##_SHIFT) & name##_MASK )
#define BFN_GET(y, name)		( ((y) & name##_MASK)>>name##_SHIFT )
#define BFN_SET(y, x, name)		(y = ((y)&~name##_MASK) | BFN_PREP(x,name) )

#define BFN_PREP2(x, name)		( (x) & name##_MASK )
#define BFN_GET2(y, name)		( (y) & name##_MASK )
#define BFN_SET2(y, x, name)	(y = ((y)&~name##_MASK) | BFN_PREP2(x,name) )

/*********************************************************************************
	Global Structure Definitions
*********************************************************************************/

/* Offset structures */
struct offset_vec {
	uint16_t n, e, s, w;
};

/* Position structures */
struct dir_vec {
	int16_t x, y;
};
struct dir_en {
	bool travel_n, travel_e, travel_s, travel_w;
};

/* Player structures */
struct player_struct {
	struct dir_vec walk_dir, face_dir;
	bool reverse_walking_render;
};

/* Animation structures */
typedef void ( *animation_function )( void );

struct animation_settings {
	bool running;
	bool finished;
	bool reverse;
	uint8_t tick;
	uint8_t occurance;
	uint8_t step;
	uint8_t repeat;
	animation_function animation_ptr;
};

/* Map structures */
struct map_tile {
	bool top_layer;
	bool animate_en;
	bool bg_animate_en;
	bool can_walk_n;
	bool can_walk_e;
	bool can_walk_s;
	bool can_walk_w; // ie can walk FROM the n/e/s/w
	uint16_t texture;
	uint16_t texture_offset;
	bool texture_reverse_x;
	bool texture_reverse_y;
	uint16_t bg_texture;
	uint16_t bg_texture_offset;
	bool bg_texture_reverse_x;
	bool bg_texture_reverse_y;
	bool interact_tile;
	uint16_t interact_id;
	bool npc_tile;
	uint16_t npc_id;
	bool exit_tile;
	uint16_t exit_map_id;
	struct dir_vec exit_map_dir;
	struct dir_vec exit_map_pos;
};

struct map {
	uint16_t map_id;
	const struct map_tile (*map_tiles_ptr);
	uint16_t map_height;
	uint16_t map_width;
	uint16_t map_setting_id;
};

struct exit_map_info {
	bool waiting_load;
	bool waiting_show;
	uint16_t exit_map_id;
	struct dir_vec exit_map_pos;
} ;

/* Game ticks */
struct game_ticks {
	uint32_t game;
	uint8_t animation;
	uint8_t player;
	uint8_t interaction_delay;
	uint8_t texture_animation_update;
	uint8_t texture_animation_inc;
};

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
	main.c Function Prototypes
*********************************************************************************/
void debugging( void );

void calculate_move_restrictions( void );
void calculate_map_offsets( void );
void show_exit_arrow( void );
void hide_exit_arrow( void );

void draw_map_tile( int16_t _draw_x, int16_t _draw_y, const struct map_tile ( *map_tiles_ptr ) );
void draw_map_init( bool reset_scroll );
void update_texture_animations( void );
void set_player_sprite( uint8_t offset, bool flip_h );

void init( void );

extern void fade_screen( void );

#endif