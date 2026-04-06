#include <stdio.h>
#include <stdint.h>

extern volatile uint32_t reset_count;
extern volatile uint32_t reset_flag;

int main(void)
{
    volatile uint32_t cnt = reset_count;

    while (1)
    {
    }
}