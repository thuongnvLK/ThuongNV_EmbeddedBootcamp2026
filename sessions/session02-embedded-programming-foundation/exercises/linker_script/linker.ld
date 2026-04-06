ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 256K
  RAM   (rwx): ORIGIN = 0x20000000, LENGTH = 31K
  RAM_NC (rwx): ORIGIN = 0x20007C00, LENGTH = 1K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
  .text :
  {
    *(.isr_vector)
    *(.text*)
    *(.rodata*)
  } > FLASH

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > RAM AT > FLASH

  _sidata = LOADADDR(.data);

  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } > RAM
  
  .non_clear_ram (NOLOAD):
  {
    /*. = ORIGIN(RAM) + 0x00002400; */
    . = ALIGN(4);
    _snon_clear_ram = .;
    KEEP(*(.non_clear_ram))
    KEEP(*(.non_clear_ram*))
    . = ALIGN(4);
    _enon_clear_ram = .;
  } > RAM_NC
  
}