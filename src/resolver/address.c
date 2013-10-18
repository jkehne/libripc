#include "address.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>


#define LEN_ADDR 31

static char addr[LEN_ADDR + 1] = { 0 };


const char *address_get()
{
	snprintf(addr, LEN_ADDR, "127.0.0.1:%i", ((uint32_t) rand()) % 1024);
	return addr;
}
