#include "keys.h"

void key_poll( void )
{
	_keys_current = ~REG_KEYINPUT & KEY_MASK;
}