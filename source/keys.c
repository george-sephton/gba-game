#include "keys.h"

void key_poll( void )
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