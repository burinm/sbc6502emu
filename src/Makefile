
vpath %.h ./M6502
vpath %.c ./M6502

SOURCES :=  main.c M6502.c Debug.c
OBJS := $(SOURCES:%.c=%.o)

CC := /usr/bin/gcc

CFLAGS := -g -std=c99

INCLUDES += -I./M6502
LIBS := -lpthread

ifeq ($(DEBUG), 1)
    CFLAGS += -DDEBUG
endif

# little endian machine
CFLAGS += -DLSB_FIRST

sbc: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -c -o $@



clean:
	rm -rf *.o
