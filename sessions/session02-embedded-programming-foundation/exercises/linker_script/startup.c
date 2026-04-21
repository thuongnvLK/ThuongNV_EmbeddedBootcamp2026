#include <stdint.h>

#define MAGIC 0xDEADBEEF



//*****************************************************************************
//
// The entry point for the application.
//
//*****************************************************************************
extern int main(void);


void Reset_Handler(void);

extern uint32_t _estack;

extern uint32_t _sidata;
extern uint32_t _sdata; 
extern uint32_t _edata;

extern uint32_t _sbss;
extern uint32_t _ebss;

extern uint32_t _snon_clear_ram;
extern uint32_t _enon_clear_ram;

//*****************************************************************************
//
// The vector table.  Note that the proper constructs must be placed on this to
// ensure that it ends up at physical address 0x0000.0000.
//
//*****************************************************************************
__attribute__ ((section(".isr_vector")))
void (*const vector_table[])(void) = {
    (void (*)(void)) &_estack,
    Reset_Handler
};


__attribute__ ((section(".non_clear_ram")))
volatile uint32_t reset_count;

__attribute__ ((section(".non_clear_ram")))
volatile uint32_t reset_flag;

#define MAX_RESET 1000000

void Reset_Handler(void)
{
    uint32_t *src, *dst;

    // copy .data
    src = &_sidata;
    dst = &_sdata;
    while (dst < &_edata)
        *dst++ = *src++;

    // clear .bss
    dst = &_sbss;
    while (dst < &_ebss)
        *dst++ = 0;

    if (reset_flag != MAGIC)
    {
        reset_flag = MAGIC;
        reset_count = 0;
    }
    else
    {
        if (reset_count >= MAX_RESET)
            reset_count = 0;
    }

    reset_count++;

    main();

    while (1);
}