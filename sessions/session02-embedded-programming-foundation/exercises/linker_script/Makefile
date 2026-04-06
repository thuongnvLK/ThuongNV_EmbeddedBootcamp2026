CC=arm-none-eabi-gcc

CFLAGS=-mcpu=cortex-m4 -mthumb -O0 -g -ffreestanding

all:
	$(CC) $(CFLAGS) startup.c main.c -T linker.ld -nostdlib -o main.elf

clean:
	rm -f *.elf