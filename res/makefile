hello_world_asm := hello_world.s
multiply_asm := mul.s
ASSEM := vasm6502_oldstyle
AFLAGS := -dotdir -Fbin


hello_world := hello_world.bin
multiply := multiply.bin

all: $(hello_world) $(multiply)

$(multiply): $(multiply_asm)
	$(ASSEM) $(AFLAGS) $< -o $@

$(hello_world): $(hello_world_asm)
	$(ASSEM) $(AFLAGS) $< -o $@