#pragma once

#if defined(__x86_64__)
    #include <isa/x86_64/gdt.h>
    #include <isa/x86_64/idt.h>
    #include <isa/x86_64/msr.h>
    #include <isa/x86_64/io.h>
    #include <isa/x86_64/paging.h>
    #include <isa/x86_64/x86_64.h>
#endif
