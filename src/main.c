#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "M6502.h"
#include "E6522.h"
#include "6522.h"


// Ram/Rom descriptions, all point to same main memory
typedef struct {
                    uint8_t* memory;
                    uint16_t size;
                    uint16_t start;
                    char*   desc;
                } memory_6502_t;

#define MEM_SIZE    (64 * 1024)

#define SRAM_SIZE   (32 * 1024)
#define SRAM_START  0x0000

#define ROM_SIZE    ( 8 * 1024)
#define ROM_START   0xe000

#define RTS         0x60 //Return from Subroutine


// Emulation memory mapped devices
static memory_6502_t memory_sram;
static memory_6502_t memory_rom;
static memory_6502_t via0_ram;
uint8_t*  memory;

void *runCPU(void *p);
void printM6502(M6502 *R);
void load_rom(char* filename, uint16_t address, memory_6502_t* memory);
void memory_dump(memory_6502_t *);

//sbc state
#define SBC_RUNNING     0x1

//TODO mutexes
uint8_t sbc_status;


int main() {

memory=(uint8_t*)calloc(MEM_SIZE,1);
//Initialize ram, fill with RTS so errand code will
// jump to handler
memory_sram.memory=memory;
memset(memory_sram.memory,RTS,(SRAM_SIZE));
memory_sram.size=SRAM_SIZE;
memory_sram.start=SRAM_START;
memory_sram.desc="32K-main-ram";

//Initialize rom
memory_rom.memory=memory;
memory_rom.size=ROM_SIZE;
memory_rom.start=ROM_START;
memory_rom.desc="8K-main-rom";

//Initialize VIAs
via0_ram.memory=memory;
via0_ram.size=0,DEVICE_6522_NUM_REG;
via0_ram.start=VIA_0_START;
via0_ram.desc="VIA0";
device_6522_init((uint8_t*)(via0_ram.memory + via0_ram.start));

//Load rom
load_rom("sbc.rom", 0xe000, &memory_rom);

//Set machine to running
sbc_status=SBC_RUNNING;

pthread_t cpu_thread;
memset((void*)cpu_thread,0,sizeof(pthread_t));

int cpu_handle;
cpu_handle = pthread_create(&cpu_thread,NULL,runCPU,NULL);


pthread_join(cpu_thread,NULL);

printf("Started CPU\n");



} 

void *runCPU(void *p)
{

    M6502 *R = malloc(sizeof(M6502));
    printM6502(R);
    Reset6502(R);
    R->Trace=1; //          Turn trace on/off
    R->Trap=1; //          Turn trace on/off
    printM6502(R);

Run6502(R);

    printM6502(R);
    free (R);

return 0;
}

void printM6502(M6502 *R)
{
    printf("A=%.2x P=%.2x X=%.2x Y=%.2x S=%.2x  PC=%.2x%.2x\n",R->A, R->P, R->X, R->Y, R->S, R->PC.B.h, R->PC.B.l);

}

//Core CPU functions

void Wr6502(register word Addr,register byte Value)
{   
    if (Addr >= memory_sram.start && Addr < memory_sram.start + memory_sram.size ) { 
        *(uint8_t *)(memory_sram.memory + Addr) = Value;
        //printf("write ram %04x:%02x\n",Addr,*(uint8_t *)(memory_sram.memory + Addr));
    }

    if (Addr >= via0_ram.start && Addr < via0_ram.start + via0_ram.size ) { 
        *(uint8_t *)(via0_ram.memory + Addr) = Value;
        //printf("write via0_ram %04x:%02x\n",Addr,*(uint8_t *)(via0_ram.memory + Addr));
    }

    if (Addr >= memory_rom.start && Addr < memory_rom.start + memory_rom.size ) { 
        printf("write rom !! %04x:%02x error\n",Addr,*(uint8_t *)(memory_rom.memory + Addr));
    }
}

byte Rd6502(register word Addr)
{
    if (Addr >= memory_sram.start && Addr < memory_sram.start + memory_sram.size ) { 
        //printf("read ram %04x:%02x\n",Addr,*(uint8_t *)(memory_sram.memory + Addr));
        return *(uint8_t *)(memory_sram.memory + Addr);
    }

    if (Addr >=  via0_ram.start && Addr <  via0_ram.start +  via0_ram.size ) { 
        //printf("read  via0_ram %04x:%02x\n",Addr,*(uint8_t *)( via0_ram.memory + Addr));
        return *(uint8_t *)( via0_ram.memory + Addr);
    }

    if (Addr >= memory_rom.start && Addr < memory_rom.start + memory_rom.size ) { 
        //printf("read rom %04x:%02x\n",Addr,*(uint8_t *)(memory_rom.memory + Addr));
        return *(uint8_t *)(memory_rom.memory + Addr);
    }
}

/* Unimpelemented. Used for operations that don't need to read memory
 * Future speed enhancement
 *
 *      byte Op6502(register word Addr);
 */

byte Loop6502(register M6502 *R)
{
    //Input/Interrupts 

    if (! (sbc_status & SBC_RUNNING) ) return (INT_QUIT);
    return (INT_NONE);
}

byte Patch6502(register byte Op,register M6502 *R)
{
    printf ("unknown opcode 0x%x\n", Op);
    //assert(0);
    return 1;
}

//Utilities

void load_rom(char* filename, uint16_t address, memory_6502_t* memory) {

FILE* f;
struct stat stat_s;
off_t rom_size;

    //open rom
    f = fopen(filename,"r");

    if (f == NULL) {
        printf("Couldn't open rom %s\n",filename);
        exit(-1);
    }

    stat(filename,&stat_s);
    rom_size=stat_s.st_size;
    printf("rom %s has %lx bytes, load",filename,rom_size);

    printf(" into location 0x%04x - 0x%04lx\n",address,rom_size + address -1); 

    if ( rom_size  > memory->size ) {
        printf("error: ram %s has %x bytes total.\n",memory->desc,memory->size);
        fclose(f);
        exit(-1);
    }

    if (rom_size -1 + address >  memory->start + memory->size -1) {
        printf("Cannot load at 0x%04x, ram range is from 0x%04x - 0x%04x\n",
                address, memory->start, memory->start + memory->size -1);
        printf("Trying to load  0x%04x - 0x%04lx\n",
                address, address + rom_size -1);
        exit(-1);
    }


    fread((void*)((uint8_t*)(memory->memory + address)),rom_size,1,f);
    printf("%p %p\n",(void*)((uint8_t*)(memory->memory)), (void*)((uint8_t*)(memory->memory + address)));
    printf("loaded into %s\n",memory->desc);

    memory_dump(memory);

    fclose(f);
}

void memory_dump(memory_6502_t *memory) {

    printf("0x%04x [%s] size 0x%x\n",memory->start,memory->desc,memory->size);
    for (uint32_t i=memory->start;i<memory->start + memory->size;i++) {
        if (i % 32 == 0) { printf("\n"); }
        printf("%02x ",memory->memory[i]);
    }
    printf("\n");
}

