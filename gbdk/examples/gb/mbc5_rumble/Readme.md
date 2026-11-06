# GBDK example for using the rumble motor on the Game Boy MBC5 Cartridge
This example demonstrates using the rumble motor that can be present
on some MBC5 cartridges. For example the rumble motor was used in Pokemon Pinball.

# Makefile MBC
You need to use one of the following MBCs in the Makefile:
- `0x1C`: MBC5 + RUMBLE
- `0x1D`: MBC5 + RUMBLE + SRAM
- `0x1E`: MBC5 + RUMBLE + SRAM + BATTERY

In this example the last one is used. You can change it in the `Makefile`

# Emulators
Not every emulator supports Rumble, and most of the ones that do only work
when you plug in a controller. The intensity of rumble using emulators may
differ from what is experienced on an actual rumble cart in a Game Boy.

# SRAM Banks
If rumble is used in a program which does cart SRAM bank switching then the
SRAM bank should be kept track of in a variable. The `MBC5_RUMBLE_ON` and
`MBC5_RUMBLE_OFF` macros should be changed to include that SRAM bank variable
(replacing `MBC_RAM_BANK_0`) so that rumble updates do not unexpectedly alter
the cart SRAM bank.

Likewise when switching cart SRAM banks to access data, the rumble state
should be included when calling `SWITCH_RAM()` or `SWITCH_RAM_MBC5()` so
that rumble status is not interrupted when active.