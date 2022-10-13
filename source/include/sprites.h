#ifndef SPRITES_H_INCLUDED
#define SPRITES_H_INCLUDED

#include "project.h"

typedef struct OBJ_ATTR
{
	u16 attr0;
	u16 attr1;
	u16 attr2;
	s16 fill;
} __attribute__((aligned(4))) OBJ_ATTR;

void oam_init(OBJ_ATTR *obj, u32 count);
void oam_copy(OBJ_ATTR *dst, const OBJ_ATTR *src, u32 count);

//! Set the attributes of an object.
static inline OBJ_ATTR *obj_set_attr(OBJ_ATTR *obj, u16 a0, u16 a1, u16 a2)
{
	obj->attr0= a0; obj->attr1= a1; obj->attr2= a2;
	return obj;
}

//! Set the position of \a obj
static inline void obj_set_pos(OBJ_ATTR *obj, int x, int y)
{
	BFN_SET(obj->attr0, y, ATTR0_Y);
	BFN_SET(obj->attr1, x, ATTR1_X);
}

//! Hide an object.
static inline void obj_hide(OBJ_ATTR *obj)
{	BFN_SET2(obj->attr0, ATTR0_HIDE, ATTR0_MODE);		}

//! Unhide an object.
/*! \param obj	Object to unhide.
*	\param mode	Object mode to unhide to. Necessary because this affects
*	  the affine-ness of the object.
*/
static inline void obj_unhide(OBJ_ATTR *obj, u16 mode)
{	BFN_SET2(obj->attr0, mode, ATTR0_MODE);			}

#endif