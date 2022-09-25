#ifndef KEYS_H_INCLUDED
#define KEYS_H_INCLUDED

// Include assets
#include "project.h"

extern uint16_t __key_curr;

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

/* Gives a tribool (-1, 0, or +1) depending on the state of some bits */
static inline int bit_tribool(u32 flags, uint plus, uint minus)
{	return ((flags>>plus)&1) - ((flags>>minus)&1);	}


/* Horizontal tribool (right,left)=(+,-) */
static inline int key_tri_horz(void)		
{	return bit_tribool(__key_curr, KI_RIGHT, KI_LEFT);	}

/* Vertical tribool (down,up)=(+,-) */
static inline int key_tri_vert(void)		
{	return bit_tribool(__key_curr, KI_DOWN, KI_UP);		}

/* Shoulder-button tribool (R,L)=(+,-) */
static inline int key_tri_shoulder(void)	
{	return bit_tribool(__key_curr, KI_R, KI_L);			}

/* Fire-button tribool (A,B)=(+,-) */
static inline int key_tri_fire(void)		
{	return bit_tribool(__key_curr, KI_A, KI_B);			}


/* Key is down */
static inline uint32_t key_down( uint32_t key ) {
	return __key_curr & key;
}

void key_poll( void );

#endif