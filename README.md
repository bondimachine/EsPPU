# EsPPU 
ESP32 based NES PPU (2C0x) replacement 


# Pinout

ESP32 Dev Kit V1 to 2C02 (assuming 3.3v levels)


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
D33   --    /CS -> |13  28| -> PPU A10 -- N/C
N/C   --   EXT0 <> |14  27| -> PPU A11 -- N/C
N/C   --   EXT1 <> |15  26| -> PPU A12 -- N/C
N/C   --   EXT2 <> |16  25| -> PPU A13 -- N/C
N/C   --   EXT3 <> |17  24| -> /RD     -- N/C
D35   --    CLK -> |18  23| -> /WR     -- N/C
D32   --   /INT <+ |19  22| <- /RST    -- D34
GND   --    GND -- |20  21| -> VOUT    -- D25
                   '------'
