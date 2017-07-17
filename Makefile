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
	gcc $(C_FLAGS) -o contious_heap.c.o contious_heap.c
	gcc $(C_FLAGS) -o ide.c.o ide.c
	gcc $(C_FLAGS) -o catmfs.c.o catmfs.c
	gcc $(C_FLAGS) -o proc.c.o proc.c
	ld $(LD_FLAGS) -o kernel.bin start.o *.asm.o *.c.o
	sudo rm -f /home/dcat/osdev/bgrub/kernel.bin
	sudo cp kernel.bin /home/dcat/osdev/bgrub/
	sudo cp ../initrd /home/dcat/osdev/bgrub/
	sudo umount /dev/mapper/loop0p1 || echo ""
	sudo mount /dev/mapper/loop0p1 /home/dcat/osdev/bgrub/
	#sudo umount /dev/loop0
	#sudo mount /dev/loop0 /home/dcat/osdev/bgrub/
	rm -rf *.o # kernel.bin
bochs:
	bochs -q -f ../bochsrc.txt
debug:
	bochs -q -f ../bochsrc_debug.txt
qemu:
	qemu-system-i386 -hda ../bgrub.img -hdb ../initrd -m 16 -k en-us -sdl  -s -d guest_errors,cpu_reset,pcall,cpu_reset -no-reboot
run:
	qemu-system-i386 -hda ../bgrub.img -fdb ../initrd -m 16 -k en-us -sdl  -s
clean:
	rm -rf *.o kernel.bin