
vpath %.h ./M6502
vpath %.c ./M6502
vpath %.h ../../ecen5013/src/driver
vpath %.c ../../ecen5013/src/driver

SOURCES :=  main.c E6522.c M6502.c Debug.c
OBJS := $(SOURCES:%.c=%.o)

CC := /usr/bin/gcc

CFLAGS := -g -std=c99

INCLUDES += -I./M6502 -I../../ecen5013/src/driver
LIBS := -lpthread

#defining DEBUG=1 turns compiles in M6502 debugger
ifeq ($(DEBUG), 1)
    CFLAGS += -DDEBUG
endif

# little endian machine
CFLAGS += -DLSB_FIRST

sbc_emu: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)
	cp ~/ecen5013/sbc/out.bin sbc.rom

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -c -o $@

clean:
	rm -rf *.o sbc_emu

getrom:
	cp ~/ecen5013/sbc/out.bin sbc.rom
