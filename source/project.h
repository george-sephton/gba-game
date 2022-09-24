#ifndef PROJECT_H_INCLUDED
#define PROJECT_H_INCLUDED

/* https://www.coranac.com/tonc/text/first.htm */
#include <gba_console.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*fnptr)(void);

/*********************************************************************************
	Register Definitions
*********************************************************************************/
#define MEM_IO				0x04000000
#define MEM_PAL				0x05000000
#define MEM_VRAM			0x06000000

#define PAL_SIZE			0x00400
#define VRAM_SIZE			0x18000

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

/* Map structures */
struct map_tile {
	bool top_layer;
	bool can_walk_n;
	bool can_walk_e;
	bool can_walk_s;
	bool can_walk_w; // ie can walk FROM the n/e/s/w
	uint16_t texture;
	uint16_t texture_offset;
	bool texture_reverse_x;
	bool texture_reverse_y;
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
	bool running_en;
	int16_t bg_texture;
	int16_t bg_texture_offset;
};

#endif