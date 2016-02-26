SECTIONS
{
    .rel.rodata.suite_registry : {
        /* Fix GDB — it thinks the file is invalid
           without these 16 bytes. */
        BYTE(1); BYTE(1); BYTE(1); BYTE(1);
        BYTE(1); BYTE(1); BYTE(1); BYTE(1);
        BYTE(1); BYTE(1); BYTE(1); BYTE(1);
        BYTE(1); BYTE(1); BYTE(1); BYTE(1);
        PROVIDE(suite_registry_begin = .);
        *(.suite_registry)
        PROVIDE(suite_registry_end = .);
    }
}
INSERT BEFORE .text;
