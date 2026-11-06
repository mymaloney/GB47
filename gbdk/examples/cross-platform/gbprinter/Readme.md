
GBPrinter
=========

Prints the image onto the game boy printer. To connect the game boy printer to the 
Mega Duck or the Sega Game Gear you need to solder the specific link cable adapter.

### Sega Game Gear cable pinout:

```
DPC0 <--> SOUT
DPC1 <--> SIN
DPC6 <--> CLK
GND  <--> GND
```

### Mega Duck cable pinout:

The link port on the Mega Duck has the same pin order and signals as the Game Boy,
but a different connector style (bare header).

Diagram showing the link port <ins>receptacle</ins>*1 pinout when looking at the side of the console:

```
      Front of console
-----------------------------------
                                   |
--------      -----------------    |
 volume |    | CLK  SIN  VCC*2 |   |
 dial   |    | GND   -   SOUT  |   |
--------      -----------------    |
                                   |
-----------------------------------                                   
       Back of console

```
- 1: View and pinout is of the connector receptacle, *not* of the cable
- 2: For most uses the VCC pin should not be connected
