#include "keys.h"

uint16_t _keys_current;

void key_poll( void )
{
	_keys_current = ~REG_KEYINPUT & KEY_MASK;
}