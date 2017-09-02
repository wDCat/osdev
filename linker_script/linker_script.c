#ifndef __keep_symbol__
OUTPUT_FORMAT("binary")
#endif
ENTRY(start)
phys = 0x00100000;
SECTIONS
{
.boot 0x00100000 : {
*(.mboot)
. = ALIGN(4096);
}
.

text : {
    code = .;
    *(.text)
    . = ALIGN(4096);
}

.

data : {
    *(.data)
    *(.rodata)
    . = ALIGN(4096);
}

.

bss : {
    bss = .;
    *(.bss)
    . = ALIGN(4096);
}

initrd =
.;

end =
.;

}