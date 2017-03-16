C_FLAGS= -fno-stack-protector -m32 -std=c99 -Wall -O -fstrength-reduce -fomit-frame-pointer -finline-functions -nostdinc -fno-builtin -I./include -c
LD_FLAGS=  -n -m elf_i386 -A elf32-i386 -nostdlib -T linker.ld
cfiles :=$(wildcard *.c)
all:
	rm -rf *.o
	nasm -felf -f aout -o start.o asm/start.asm
	for name in `ls *.c`; \
	do \
	gcc $(C_FLAGS) -o $$name.o $$name;\
	done
	ld $(LD_FLAGS) -o kernel.bin start.o *.c.o
	rm -f /home/dcat/osdev/bgrub/kernel.bin
	cp kernel.bin /home/dcat/osdev/bgrub/
	sudo umount /dev/mapper/loop0p1
	sudo mount /dev/mapper/loop0p1 /home/dcat/osdev/bgrub/
	rm -rf *.o # kernel.bin
	qemu-system-i386 -hda ../bgrub.img -m 16 -k en-us -sdl  -s -d guest_errors,cpu_reset,pcall
run:
	qemu-system-i386 -hda ../bgrub.img -m 512 -k en-us -sdl  -s
clean:
	rm -rf *.o kernel.bin
old_build:
	#gcc $(C_FLAGS) -o pp.o *.c
	#gcc $(C_FLAGS) -o main.o main.c
	#gcc $(C_FLAGS) -o screen.o screen.c
	#gcc $(C_FLAGS) -o str.o str.c
	#gcc $(C_FLAGS) -o gdt.o gdt.c
	#gcc $(C_FLAGS) -o idt.o idt.c
	#gcc $(C_FLAGS) -o isrs.o isrs.c
	#gcc $(C_FLAGS) -o irqs.o irqs.c
	#gcc $(C_FLAGS) -o timer.o timer.c
	#gcc $(C_FLAGS) -o keyboard.o keyboard.c
	#gcc $(C_FLAGS) -o page.o page.c
	#ld $(LD_FLAGS) -o kernel.bin start.o main.o screen.o str.o gdt.o idt.o isrs.o irqs.o timer.o keyboard.o page.o
	
debug:
	qemu-system-i386 -hda ../bgrub.img -m 8 -k en-us -sdl -s -S