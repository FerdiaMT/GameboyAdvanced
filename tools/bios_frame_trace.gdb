# Capture the BIOS OBJ animation state once per completed visible frame.
# Usage: gdb -q -batch -x tools/bios_frame_trace.gdb ./GBA
set pagination off
set debuginfod enabled off
set logging file /tmp/gba-bios-frames.log
set logging enabled on
set $frame = 0
set $line0DisplayControl = 0

break PPU::renderScanline if line == 0
commands
  silent
  set $line0DisplayControl = this->displayControl
  continue
end

break PPU::renderScanline if line == 159
commands
  silent
  set $frame = $frame + 1
  if $frame >= 70 && $frame <= 150
    printf "F=%03d DC=%04x->%04x O0=%04x/%04x/%04x O5=%04x/%04x/%04x O6=%04x/%04x/%04x M1=%d,%d,%d,%d M6=%d,%d,%d,%d P1=%04x\n", $frame, $line0DisplayControl, this->displayControl, *((unsigned short *)&this->bus.oam[0]), *((unsigned short *)&this->bus.oam[2]), *((unsigned short *)&this->bus.oam[4]), *((unsigned short *)&this->bus.oam[40]), *((unsigned short *)&this->bus.oam[42]), *((unsigned short *)&this->bus.oam[44]), *((unsigned short *)&this->bus.oam[48]), *((unsigned short *)&this->bus.oam[50]), *((unsigned short *)&this->bus.oam[52]), *((short *)&this->bus.oam[38]), *((short *)&this->bus.oam[46]), *((short *)&this->bus.oam[54]), *((short *)&this->bus.oam[62]), *((short *)&this->bus.oam[198]), *((short *)&this->bus.oam[206]), *((short *)&this->bus.oam[214]), *((short *)&this->bus.oam[222]), *((unsigned short *)&this->bus.palette[514])
  end
  continue
end

run run bin/sma.gba --bios bin/gba_bios.bin --steps 10000000
