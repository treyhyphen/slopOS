/* slopOS Linker Script */

ENTRY(_start)

SECTIONS
{
    /* Begin putting sections at 1 MiB, a conventional place for kernels to be loaded */
    . = 1M;

    /* First put the multiboot header, as it is required to be near the beginning */
    .boot :
    {
        *(.multiboot)
    }

    /* Followed by the text (code) section */
    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.text)
    }

    /* Read-only data */
    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    /* Read-write data (initialized) */
    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    /* Read-write data (uninitialized) and stack */
    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }
}
