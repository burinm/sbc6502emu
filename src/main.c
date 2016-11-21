#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// GUI component
#include <SDL2/SDL.h>
#include "console.h"

#include <signal.h>

#include "M6502.h"
#include "E6522.h"
#include "6522.h"

typedef struct {
                    uint8_t* memory;
                    uint16_t size;
                    uint16_t start;
                    char*   desc;
                } memory_6502_t;

// Green LEDs
uint8_t green_led_array_porta;
uint8_t green_led_array_portb;



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
pthread_mutex_t mem_lock=PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t mem_lock=PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#define MEM_LOCK    { pthread_mutex_lock(&mem_lock); }
#define MEM_UNLOCK  { pthread_mutex_unlock(&mem_lock); }


//TODO mutexes
uint8_t sbc_status;
pthread_mutex_t sbc_status_lock=PTHREAD_MUTEX_INITIALIZER;
#define SBC_STATUS_LOCK    { pthread_mutex_lock(&sbc_status_lock); }
#define SBC_STATUS_UNLOCK  { pthread_mutex_unlock(&sbc_status_lock); }

//Threads
void *runCPU(void *p);
void *runVIA0(void *p);

void printM6502(M6502 *R);
void load_rom(char* filename, uint16_t address, memory_6502_t* memory);
void memory_dump(memory_6502_t *);

//sbc state
#define SBC_RUNNING     0x1



//Escape!!
void ctrlC(int sig)
{
    signal(sig, SIG_IGN);
    sbc_status &= ~(SBC_RUNNING);

    printf ("interrupt!\n");
    signal(SIGINT, ctrlC);
}

int main() {
signal(SIGINT, ctrlC);

//pthread_mutex_init(&mem_lock,NULL);
//pthread_mutex_init(&sbc_status_lock,NULL);

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
via0_ram.size=DEVICE_6522_NUM_REG;
via0_ram.start=VIA_0_START;
via0_ram.desc="VIA0";
device_6522_init((uint8_t*)(via0_ram.memory + via0_ram.start));

//Load rom
load_rom("sbc.rom", 0xe000, &memory_rom);

//Set machine to running
sbc_status=SBC_RUNNING;

// x2 8 Green LEDs
green_led_array_portb=0; // all off
green_led_array_porta=0; // all off

//Start console
console_main();

// Maybee use SDL thread....?
//SDL_Thread *thread;
//    int thread_id=0;
//if ((thread=SDL_CreateThread(runCPU, "CPUThread", (void *)NULL)) == NULL) { printf("couldn't creat CPU thread\n"); goto error;}


#if 0
pthread_t via0_thread;
memset((void*)&via0_thread,0,sizeof(pthread_t));

int via0_handle;
via0_handle = pthread_create(&via0_thread,NULL,runVIA0,NULL);
printf("Started VIA0\n");
#endif

pthread_t cpu_thread;
memset((void*)&cpu_thread,0,sizeof(pthread_t));

int cpu_handle;
cpu_handle = pthread_create(&cpu_thread,NULL,runCPU,NULL);
printf("Started CPU\n");

while (sbc_status & SBC_RUNNING) {
    consoleRefresh();
}

pthread_join(cpu_thread,NULL);
//pthread_join(via0_thread,NULL);

//pthread_mutex_destroy(&mem_lock);

console_quit();



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

printf("runCPU exit\n");
return 0;
}

void *runVIA0(void *p) { 

while (sbc_status & SBC_RUNNING) {
    //MEM_LOCK
    // Set LEDs to match port B output
    //green_led_array_portb = *(uint8_t*)(via0_ram.memory + via0_ram.start + DEVICE_6522_REG_RB);
    //printf("b - %x ", *(uint8_t*)(via0_ram.memory + via0_ram.start + DEVICE_6522_REG_RB));

    // Set LEDs to match port A output
    //green_led_array_porta = *(uint8_t*)(via0_ram.memory + via0_ram.start + DEVICE_6522_REG_RA);
    //printf("a - %x ", *(uint8_t*)(via0_ram.memory + via0_ram.start + DEVICE_6522_REG_RA));
    //MEM_UNLOCK
}
printf("runVIA0 exit\n");

}

void printM6502(M6502 *R)
{
    printf("A=%.2x P=%.2x X=%.2x Y=%.2x S=%.2x  PC=%.2x%.2x\n",R->A, R->P, R->X, R->Y, R->S, R->PC.B.h, R->PC.B.l);

}

//Core CPU functions

void Wr6502(register word Addr,register byte Value)
{   
//MEM_LOCK
    if (Addr >= memory_sram.start && Addr < memory_sram.start + memory_sram.size ) { 
        *(uint8_t *)(memory_sram.memory + Addr) = Value;
        //printf("write ram %04x:%02x\n",Addr,*(uint8_t *)(memory_sram.memory + Addr));
    }

    if (Addr >= via0_ram.start && Addr < via0_ram.start + via0_ram.size ) { 
        *(uint8_t *)(via0_ram.memory + Addr) = Value;
        printf("write via0_ram %04x:%02x\n",Addr,*(uint8_t *)(via0_ram.memory + Addr));
        MEM_LOCK
        green_led_array_portb = *(uint8_t*)(via0_ram.memory + via0_ram.start + DEVICE_6522_REG_RB);
        green_led_array_porta = *(uint8_t*)(via0_ram.memory + via0_ram.start + DEVICE_6522_REG_RA);
printf("greena %x green b %x\n",green_led_array_porta, green_led_array_portb);
        MEM_UNLOCK
    }

    if (Addr >= memory_rom.start && Addr < memory_rom.start + memory_rom.size ) { 
        printf("write rom !! %04x:%02x error\n",Addr,*(uint8_t *)(memory_rom.memory + Addr));
    }
//MEM_UNLOCK
}

byte Rd6502(register word Addr)
{
//MEM_LOCK
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
//MEM_UNLOCK
}

/* Unimpelemented. Used for operations that don't need to read memory
 * Future speed enhancement
 *
 *      byte Op6502(register word Addr);
 */

byte Loop6502(register M6502 *R)
{
    //Input/Interrupts 

    usleep(5000);
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
    //printf("%p %p\n",(void*)((uint8_t*)(memory->memory)), (void*)((uint8_t*)(memory->memory + address)));
    printf("loaded into %s\n",memory->desc);

   // memory_dump(memory);

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

