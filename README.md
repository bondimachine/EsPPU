# EsPPU 
ESP32 based NES PPU (2C0x) replacement 


# Pinout

ESP32 Dev Kit V1 to 2C02 (don't worry, ESP32 supports 5v levels in GPIO)

```
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
D14   --    /CS -> |13  28| -> PPU A10 -- N/C
N/C   --   EXT0 <> |14  27| -> PPU A11 -- N/C
N/C   --   EXT1 <> |15  26| -> PPU A12 -- N/C
N/C   --   EXT2 <> |16  25| -> PPU A13 -- N/C
N/C   --   EXT3 <> |17  24| -> /RD     -- N/C
D27   --    CLK -> |18  23| -> /WR     -- N/C
D32   --   /INT <+ |19  22| <- /RST    -- D12
GND   --    GND -- |20  21| -> VOUT    -- D25
                   '------'
```

TODO: CHR-ROM access (latch wih 373/374 or 573/574) 

## APU 

The origial goal of this project was to be able to run NES games in using a "modern" 65C02.

As such, we also added support for emulating the APU. 

You'll need to build an external chip select signal /AS for the APU when address is between 4000-4017; excluding 4016. 4017 actually needs to combine the signals from the joystick support circuits. 

```
D39(VN) - A3
D34     - A4 
D35     - A5
D33     - Audio out
D26     - /AS
```

Audio output should pass thru a simple rc filter

```
33 ----/\/\/\/----|------- audio out
         1k       |
                 ---
                 --- 10nf
                  |
                  v gnd
```
