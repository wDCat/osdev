SRC_ROOT=.
include build_kernel.makefile
include build_lib.makefile
LD_FLAGS=-n -m elf_i386 -A elf32-i386 -nostdlib
LOOP_DEVICE_ID=4
.PHONY:pre
pre:
	sudo umount $(SRC_ROOT)/../bgrub/ || echo ""
	sudo losetup /dev/loop$(LOOP_DEVICE_ID) ../bgrub.img
	sudo kpartx -a /dev/loop$(LOOP_DEVICE_ID)
.PHONY:mount_disk
mount_disk:
	sudo losetup /dev/loop3 $(SRC_ROOT)/../disk.img || echo ""
	sudo kpartx -a /dev/loop3 || echo ""
	sudo mount /dev/mapper/loop3p1 $(SRC_ROOT)/../disk/ || echo ""
.PHONY:umount_disk
umount_disk:
	sudo umount /dev/mapper/loop3p1 || echo ""
	sudo kpartx -d /dev/loop3 || echo ""
	sudo losetup -d /dev/loop3 || echo ""
.PHONY:fsck_disk
fsck_disk:
	sudo umount /dev/mapper/loop3p1 || echo ""
	sudo fsck -f /dev/mapper/loop3p1
.PHONY:update
update:
	#update file list
	make clean
	cd build && make w2_build.jar
	cd build && java -jar w2_build.jar
kernel.ld:
	@ #build linker script
	cp -f linker_script.pld OUTPUT/linker_script.c
	gcc -E -P OUTPUT/linker_script.c -o OUTPUT/kernel.ld
kernel.bin:$(koutfiles) kernel.ld
	@ #link them or zelda them
	ld $(LD_FLAGS) -T OUTPUT/kernel.ld -o kernel.bin OUTPUT/*.o
kernel.sym:
	nm -v kernel.bin > kernel.sym
install_libdcat:libdcat.so
	cp -f libdcat.so $(SRC_ROOT)/../a_out_test/libdcat.so
	#cd $(SRC_ROOT)/../a_out_test && make install
.PHONY:install
install:
	#Copy files and remount
	sudo mount /dev/mapper/loop$(LOOP_DEVICE_ID)p1 /home/dcat/osdev/bgrub/ || echo ""
	sudo rm -f $(SRC_ROOT)/../bgrub/kernel.bin
	sudo cp kernel.bin $(SRC_ROOT)/../bgrub/
	sudo cp ../initrd $(SRC_ROOT)/../bgrub/
	sudo umount /dev/mapper/loop$(LOOP_DEVICE_ID)p1 || echo ""
	sudo mount /dev/mapper/loop$(LOOP_DEVICE_ID)p1 $(SRC_ROOT)/../bgrub/
.PHONY:all
all:kernel.bin kernel.sym
.PHONY:remount
remount:
	sudo umount /dev/mapper/loop$(LOOP_DEVICE_ID)p1 || echo ""
	sudo mount /dev/mapper/loop$(LOOP_DEVICE_ID)p1 $(SRC_ROOT)/../bgrub/
.PHONY:bochs
bochs:
	bochs -q -f ../bochsrc.txt
.PHONY:debug
debug:
	bochs -q -f ../bochsrc_debug.txt
.PHONY:qemu
qemu:
	qemu-system-i386 -serial stdio -hda ../bgrub.img -hdb ../disk.img -hdc ../swap.img -m 16 -k en-us -sdl  -s -d guest_errors,pcall,cpu_reset -no-reboot -vga std
.PHONY:run
run:
	qemu-system-i386 -hda ../bgrub.img -fdb ../disk.img -m 16 -k en-us -sdl  -s
.PHONY:clean
clean:
	rm -rf *.o kernel.bin
	rm -rf OUTPUT
.PHONY:onestep
onestep:all install qemu