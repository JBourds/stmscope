# STMScope

Author: Jordan Bourdeau

For fun project learning STM32 development, ARM Cortex-M processors, and playing
around with some new peripherals like LCD displays. Also I don't have an
oscilloscope and wanted one :)

Developed using platformio for a build system in neovim, with occasional dilly
dallies into the STM32 CubeIDE for its codegen capabilities.

# Uploading Code

`pio run -t upload`

# Serial Monitor

`pio device monitor`

# Debugging with GDB

`pio debug --interface=gdb -- -x .pioinit`

or, if keystrokes are at a premium,

`piodebuggdb -x .pioinit`
