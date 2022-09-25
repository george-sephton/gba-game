#include "keys.h"

void key_poll( void )
{
	__key_curr= ~REG_KEYINPUT & KEY_MASK;
}