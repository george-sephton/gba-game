#ifndef TONC_UTILS_H_INCLUDED
#define TONC_UTILS_H_INCLUDED

// Include assets
#include "project.h"

// Create a bitmask \a len bits long
#define BIT_MASK(len)		( BIT(len)-1 )

// Prototypes
void *tonccpy(void *dst, const void *src, uint size);
void *__toncset(void *dst, u32 fill, uint size);

//! Quadruple a byte to form a word: 0x12 -> 0x12121212.
static inline u32 quad8(u8 x)	{	return x*0x01010101;	}

//! VRAM-safe memset, byte  version. Size in bytes.
static inline void *toncset(void *dst, u8 src, uint count)
{	return __toncset(dst, quad8(src), count);		}

//! VRAM-safe memset, halfword version. Size in hwords.
static inline void *toncset16(void *dst, u16 src, uint count)
{	return __toncset(dst, src|src<<16, count*2);	}

//! VRAM-safe memset, word version. Size in words.
static inline void *toncset32(void *dst, u32 src, uint count)
{	return __toncset(dst, src, count*4);			}

#endif