src := src
cfiles := $(wildcard src/*.c)
ofiles := $(cfiles:.c=.o)
headers := $(wildcard src/*.h)
executable := daubmos

cc := gcc
cflags := -c 
ldflags :=

all: $(executable)

debug: cflags += -DDEBUG -g
debug: $(executable)

$(executable): $(ofiles)
	$(cc) $(ldflags) -o $@ $^

%.o: %.c $(headers)
	$(cc) -o $@ $< $(cflags)

clean:
	rm -f emulator $(ofiles) *.o
