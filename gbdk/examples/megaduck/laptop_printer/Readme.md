## GBDK example for the Mega Duck Laptop
How to interface with the printer made for the Mega Duck Laptop
models ("Super QuiQue" and "Super Junior Computer").

- Initializing the external controller connected over the serial link port
- Checking whether the printer is connected and which type
- Printing the screen to the printer
- Printing blank rows to the printer (to scroll the paper)

## Printer Types
There are two types of Mega Duck printer, single pass and double pass.

- The single pass monochrome model is the standard Mega Duck JC-510 thermal printer which connects to the DB-26 "Printer" port.
- The double pass model 3-color model (identified in the System ROM disassembly) is most likely the JC-502 "Drucker Interface" which connects to DB-15 "External Connector" port and allows interfacing with standard (1990's) parallel printers.

Printing example support for the double pass model is untested
on hardware at present and so not included in the `main` branch.
For it's example code see the `save_2_pass_printer` branch.

## Code structure
There are two separate files for the Mega Duck printer in this project.
- `megaduck_printer.c` contains the core interface code for querying the printer, sending tile data to it and transforming (rotate and flip) tiles to the format expected by the print er.
- `megaduck_printscreen.c` contains example code for sending a "print screen" of the console screen to the printer.
