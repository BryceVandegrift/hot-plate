# Building Instructions

## Prerequisites

### Tools and Equipment

- Soldering iron with a fine tip. Temperature controlled soldering station is
  recommended.
- Small side cutters for cutting components' leads.
- Desk lamp and or magnifying glass
- An ISCP programmer for programming the microcontroller
- Solder suitable for soldering electronics.
- Solder wick for removing excess of solder.
- 90-99% Isopropyl Alcohol for removing the excess of flux after soldering.
- Lint free wipes, used toothbrush, or cotton swabs for cleaning the PCB
  before and after soldering.
- A multimeter can be beneficial for troubleshooting.

### Parts

#### Getting Parts

A list of parts can be found in the [bill of materials](README.md#bill-of-materials)
of the README.md file. You can find the provided parts from Digikey
(recommended), Mouser, or from any other electronics parts suppliers.

You can order PCBs using any PCB manufacturing service. I recommend using JLCPCB
since they will manufacture the PCBs for cheap. Just give them the
[gerber files](gerber) and make sure that you get a **Lead free** PCB (to be
safe).

#### Optional Parts

The ISCP header (J2) on the control board is optional if you get your ATTINY84A
pre-programmed or use another method to program it besides ISCP.

#### 3D Printed Parts

This design also uses 3D printed legs to hold everything up. If you don't have a
3D printer then you can use a 3D printing service like JLC3DP. You can use
almost any material for the legs, but I would recommend just normal PLA plastic.
You can find the STL file for the legs [here]().
Also don't worry, the legs will **not** melt or majorly deform despite the heat.

#### Firmware

In order to flash the firmware to the ATTINY84A you can either use a pre-built
HEX file [here]() or build the firmware from scratch using the instructions
below.

## Building

### Build Steps

1. Inspect the PCB for obvious defects, such as deep scratches or short-circuits
   between traces.
2. Clean the PCB with an alcohol wipe.
3. Solder the components, going from lower profile components to higher profile
   ones.
4. Carefully inspect all the solder joints. Re-solder if needed. Optionally use
   a multimeter to check the board for short-circuits.
5. Clean the board using cotton swaps, wipes, and the toothbrush soaked in alcohol.
6. Insert the ATTINY84A into the socket and flash it using an ISCP programmer.
7. Attach the 3D printed feet to the boards and connect the boards together.
8. Connect the power supply and if everything works you should see text on the
   screen.

### Building the Firmware

In order to build firmware for the hot plate you'll need to make sure you have
the AVR-GCC toolchain. This is easy to install for Linux using your package
manager and MacOS using homebrew. Windows users can use
[WinAVR](https://winavr.sourceforge.net/) to get the AVR toolchain on thier
system. Besides GCC-AVR, you need Make as well.

In order to build, go to the `src` directory and run:

``` sh
make
```

This will build the firmware and create a `main.hex` file which is what you need
to flash.