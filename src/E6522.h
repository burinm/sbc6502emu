#ifndef __6522EMU_H__
#define __6522EMU_H__


// Emulated 6522 devices x 2
#define VIA_0_START     0x8010
#define VIA_1_START     0x8020


// Interface implementation
void device_6522_init(uint8_t *a);
//end interface




#endif
