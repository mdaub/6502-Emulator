# 6502-Emulator
An emulation of the MOS 6502 CPU

It is not cycle accurate, and does not currently impliment any real timing.
The only IO device currently attached to the bus is the terminal, which allows string
printing to the screen. It also allows the CPU to request the emulation to terminate.

## Building
For now there is only a makefile for GNU/Linux. It is configure to use GCC as the compiler. The only external build requirements is the C standard library.

There are only two build targets, release and debug. To build them, run 
```sh
$ make
```
for release, and
```sh
$ make debug
```
for the debug build.

## Disclaimer

I have so far only tested a small hello world program, and the vast majority of instructions are untested at this point. If you chose to use my code, do so at your own risk!