/*
                   .--\/--.
D13   --    R/W -> |01  40| -- +5V     -- Vin
D15   -- CPU D0 <> |02  39| -> ALE     -- N/C
 D2   -- CPU D1 <> |03  38| <> PPU AD0 -- N/C
 D4   -- CPU D2 <> |04  37| <> PPU AD1 -- N/C
RX2(D16)-CPU D3 <> |05  36| <> PPU AD2 -- N/C
TX2(D17)-CPU D4 <> |06  35| <> PPU AD3 -- N/C
 D5   -- CPU D5 <> |07  34| <> PPU AD4 -- N/C
D18   -- CPU D6 <> |08  33| <> PPU AD5 -- N/C
D19   -- CPU D7 <> |09  32| <> PPU AD6 -- N/C
D21   --     A2 -> |10  31| <> PPU AD7 -- N/C
D22   --     A1 -> |11  30| -> PPU A8  -- N/C
D23   --     A0 -> |12  29| -> PPU A9  -- N/C
D12   --    /CS -> |13  28| -> PPU A10 -- N/C
N/C   --   EXT0 <> |14  27| -> PPU A11 -- N/C
N/C   --   EXT1 <> |15  26| -> PPU A12 -- N/C
N/C   --   EXT2 <> |16  25| -> PPU A13 -- N/C
N/C   --   EXT3 <> |17  24| -> /RD     -- N/C
D27   --    CLK -> |18  23| -> /WR     -- N/C
D32   --   /INT <+ |19  22| <- /RST    -- D14
GND   --    GND -- |20  21| -> VOUT    -- D25
                   '------'

*/

#ifndef __PINS_H__
#define __PINS_H__

#define PIN_RW 13
#define PIN_D0 15
#define PIN_D1 2
#define PIN_D2 4
#define PIN_D3 16
#define PIN_D4 17
#define PIN_D5 5
#define PIN_D6 18
#define PIN_D7 19

#define PIN_A2 21
#define PIN_A1 22
#define PIN_A0 23
#define PIN_CS 12

#define PIN_CLK 27
#define PIN_INT 32
#define PIN_RST 14
#define PIN_VOUT 25

// this has nothing to do here, but it is a single line to share with assembly
#define COMMAND_BUFFER_SIZE 8192


#endif

