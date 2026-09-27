/* Linker script for uCore on SW64 */

OUTPUT_FORMAT("elf64-sw_64")
OUTPUT_ARCH(sw_64:sw6b)
ENTRY(kern_entry)

BASE_ADDRESS = 0xffffffff80910000;

SECTIONS
{
    . = BASE_ADDRESS;

    .text : {
        *(.text.kern_entry)
        *(.text .stub .text.* .gnu.linkonce.t.*)
    }

    PROVIDE(etext = .);

    . = ALIGN(0x2000);

    .rodata : {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
    }

    . = ALIGN(0x2000);

    .data : {
        *(.data)
        *(.data.*)
    }

    .got : {
        *(.got)
    }

    .sdata : {
        *(.sdata)
        *(.sdata.*)
    }

    PROVIDE(edata = .);

    . = ALIGN(0x2000);

    .bss : {
        *(.bss)
        *(.bss.*)
        *(.sbss*)
    }

    PROVIDE(end = .);

    /DISCARD/ : {
        *(.eh_frame .note.GNU-stack)
        *(.comment)
        *(.mdebug)
    }
}
