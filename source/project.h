#pragma once

/* https://www.coranac.com/tonc/text/first.htm */
#include <gba_console.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "font.h"

/*********************************************************************************
  Global Definitions
*********************************************************************************/
#define SCREEN_WIDTH   240
#define SCREEN_HEIGHT  160

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

/* REG_KEYINPUT */
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