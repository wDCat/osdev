C_FLAGS= -fno-stack-protector -m32 -std=c99 -Wall -O0 -O -fstrength-reduce -fomit-frame-pointer -D _BUILD_TIME=`date +%s` -finline-functions -nostdinc -fno-builtin -I./include -c
LD_FLAGS=  -n -m elf_i386 -A elf32-i386 -nostdlib -T linker.ld
LOOP_DEVICE_ID=5
cfiles :=$(wildcard *.c)
pre:
	sudo umount /home/dcat/osdev/bgrub/ || echo ""
	sudo losetup /dev/loop$(LOOP_DEVICE_ID) ../bgrub.img
	sudo kpartx -a /dev/loop$(LOOP_DEVICE_ID)
	sudo losetup /dev/loop1 ../disk.img
	sudo kpartx -a /dev/loop1
	sudo mount /dev/mapper/loop1p1 /home/dcat/osdev/disk/
all:
	rm -rf *.o
	nasm -f elf -o start.o asm/start.asm
	nasm -f elf -o syscall.asm.o asm/syscall.asm
	for name in `ls *.c`; \
	do \
	gcc $(C_FLAGS) -o $$name.o $$name;\
	done
	gcc $(C_FLAGS) -o catmfs.c.o fs/catmfs/catmfs.c
	gcc $(C_FLAGS) -o ext2.c.o fs/ext2/ext2.c
	#Copy files and remount
	ld $(LD_FLAGS) -o kernel.bin start.o *.asm.o *.c.o
	sudo mount /dev/mapper/loop$(LOOP_DEVICE_ID)p1 /home/dcat/osdev/bgrub/ || echo ""
	sudo rm -f /home/dcat/osdev/bgrub/kernel.bin
	sudo cp kernel.bin /home/dcat/osdev/bgrub/
	sudo cp ../initrd /home/dcat/osdev/bgrub/
	sudo umount /dev/mapper/loop$(LOOP_DEVICE_ID)p1 || echo ""
	sudo mount /dev/mapper/loop$(LOOP_DEVICE_ID)p1 /home/dcat/osdev/bgrub/
	#sudo losetup -d /dev/loop1 || echo ""
	#sudo losetup /dev/loop1 ../bgrub.img || echo ""
	#sudo kpartx -a /dev/loop1
	#sudo umount /dev/loop1
	#sudo mount /dev/loop1 /home/dcat/osdev/bgrub/
	rm -rf *.o # kernel.bin
remount:
	sudo umount /dev/mapper/loop$(LOOP_DEVICE_ID)p1 || echo ""
	sudo mount /dev/mapper/loop$(LOOP_DEVICE_ID)p1 /home/dcat/osdev/bgrub/
bochs:
	bochs -q -f ../bochsrc.txt
debug:
	bochs -q -f ../bochsrc_debug.txt
qemu:
	qemu-system-i386 -hda ../bgrub.img -hdb ../disk.img -m 16 -k en-us -sdl  -s -d guest_errors,cpu_reset,pcall,cpu_reset -no-reboot
run:
	qemu-system-i386 -hda ../bgrub.img -fdb ../disk.img -m 16 -k en-us -sdl  -s
clean:
	rm -rf *.o kernel.bin