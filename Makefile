include build_all.makefile
LD_FLAGS=  -n -m elf_i386 -A elf32-i386 -nostdlib
LOOP_DEVICE_ID=5
pre:
	sudo umount /home/dcat/osdev/bgrub/ || echo ""
	sudo losetup /dev/loop$(LOOP_DEVICE_ID) ../bgrub.img
	sudo kpartx -a /dev/loop$(LOOP_DEVICE_ID)
mount_disk:
	sudo losetup /dev/loop3 ../disk.img || echo ""
	sudo kpartx -a /dev/loop3 || echo ""
	sudo mount /dev/mapper/loop3p1 /home/dcat/osdev/disk/ || echo ""
umount_disk:
	sudo umount /dev/mapper/loop3p1 || echo ""
	sudo kpartx -d /dev/loop3 || echo ""
	sudo losetup -d /dev/loop3 || echo ""
fsck_disk:
	sudo umount /dev/mapper/loop3p1 || echo ""
	sudo fsck -f /dev/mapper/loop3p1
update:
	#update file list
	cd build && kotlinc src/build.kt -include-runtime -d w2_build.jar
	cd build &&java -jar w2_build.jar
all:
	#build all
	make build_all
	#build linker script
	gcc -E -P linker_script/linker_script.c -D__keep_symbol__ -o kernelsym.ld.o
	gcc -E -P linker_script/linker_script.c -o kernel.ld.o
	#link them or zelda them
	ld $(LD_FLAGS) -T kernelsym.ld.o -o kernel.sym.bin OUTPUT/*.o
	ld $(LD_FLAGS) -T kernel.ld.o -o kernel.bin OUTPUT/*.o
	#Export symbol table
	nm -v kernel.sym.bin > kernel.sym
	#Copy files and remount
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
	qemu-system-i386 -serial stdio -hda ../bgrub.img -hdb ../disk.img -m 16 -k en-us -sdl  -s -d guest_errors,pcall,cpu_reset -no-reboot
run:
	qemu-system-i386 -hda ../bgrub.img -fdb ../disk.img -m 16 -k en-us -sdl  -s
clean:
	rm -rf *.o kernel.bin
	rm -rf OUTPUT