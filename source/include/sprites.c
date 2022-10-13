#include "sprites.h"

void oam_init( OBJ_ATTR *obj, uint32_t count )
{
	uint32_t nn = count;
	uint32_t *dst = (uint32_t*)obj;

	/* Hide each object */
	while( nn-- )
	{
		*dst ++= ATTR0_HIDE;
		*dst ++= 0;
	}

	/* init oam */
	oam_copy(oam_mem, obj, count);
}

void oam_copy( OBJ_ATTR *dst, const OBJ_ATTR *src, uint32_t count ) {

// NOTE: while struct-copying is the Right Thing to do here, 
//   there's a strange bug in DKP that sometimes makes it not work
//   If you see problems, just use the word-copy version.
#if 1
	while( count-- )
		*dst++ = *src++;
#else
	uint32_t *dstw = (uint32_t*)dst, *srcw= (uint32_t*)src;
	while(count--)
	{
		*dstw++ = *srcw++;
		*dstw++ = *srcw++;
	}
#endif

}