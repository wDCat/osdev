C_FLAGS= -fno-stack-protector -m32 -std=c99 -Wall -O -fstrength-reduce -fomit-frame-pointer -finline-functions -nostdinc -fno-builtin -I./include -c
LD_FLAGS=  -n -m elf_i386 -A elf32-i386 -nostdlib -T linker.ld
cfiles :=$(wildcard *.c)
pre:
	sudo losetup /dev/loop0 ../bgrub.img
	sudo kpartx -a /dev/loop0
all:
	rm -rf *.o
	nasm -felf -f aout -o start.o asm/start.asm
	nasm -felf -f aout -o syscall.asm.o asm/syscall.asm
	for name in `ls *.c`; \
	do \
	gcc $(C_FLAGS) -o $$name.o $$name;\
	done
	gcc $(C_FLAGS) -o heap_array_list.c.o heap_array_list.c
	gcc $(C_FLAGS) -o heap.c.o heap.c
	gcc $(C_FLAGS) -o ide.c.o ide.c
	gcc $(C_FLAGS) -o catmfs.c.o catmfs.c
	ld $(LD_FLAGS) -o kernel.bin start.o *.asm.o *.c.o
	sudo rm -f /home/dcat/osdev/bgrub/kernel.bin
	sudo cp kernel.bin /home/dcat/osdev/bgrub/
	sudo cp ../initrd /home/dcat/osdev/bgrub/
	sudo umount /dev/mapper/loop0p1 || echo ""
	sudo mount /dev/mapper/loop0p1 /home/dcat/osdev/bgrub/
	#sudo umount /dev/loop1
	#sudo mount /dev/loop1 /home/dcat/osdev/bgrub/
	rm -rf *.o # kernel.bin
	qemu-system-i386 -hda ../bgrub.img -hdb ../initrd -m 16 -k en-us -sdl  -s -d guest_errors,cpu_reset,pcall,cpu_reset  -no-reboot
run:
	qemu-system-i386 -hda ../bgrub.img -fdb ../initrd -m 512 -k en-us -sdl  -s
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
