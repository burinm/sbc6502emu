/* E6522.c - (c) 2016 - burin

    65c22 I/O expander
*/

#include "6522.h"
#include "E6522.h"
#include <stdlib.h>
#include <string.h>

void device_6522_init(uint8_t *a) {
   memset(a,0,DEVICE_6522_NUM_REG); 
}
