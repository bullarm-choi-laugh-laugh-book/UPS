#define INTTAB                  0x80026000
#define RESET                   0x80000020
  
/* trap tab is on the reset address of BTV (cached area) */
#define IFXTRAPTAB              (INTTAB + 0x1800)  /* this is the default address (only cached area) of BTV after reset */

#if defined(__PROC_TC23X__)
#define __REDEFINE_ON_CHIP_ITEMS
#define __BMHD0_CONFIG __BMHD_GENERATE
#define __BMHD0_BMI __BMHD_BMI_HWCFG_ABM
#include "tc23x.lsl"
processor mpe
{
    derivative = my_tc23x;
}
derivative my_tc23x extends tc23x
{
    memory dspr0 (tag="on-chip")
    {
        mau = 8;
        type = ram;
        size = 184k;
        map (dest=bus:tc0:fpi_bus, dest_offset=0xD0000000, size=184k, priority=1);
        map (dest=bus:sri, dest_offset=0x70000000, size=184k);
    }
    memory pspr0 (tag="on-chip")
    {
        mau = 8;
        type = ram;
        size = 8k;
        map (dest=bus:tc0:fpi_bus, dest_offset=0xC0000000, size=8k);
        map (dest=bus:sri, dest_offset=0x70100000, size=8k);
    }
    memory pflash_training_data (tag="on-chip")
    {
        mau = 8;
        type = rom;
        size = 160k-8k;
        priority = 8;
        map cached(dest=bus:sri, dest_offset=0x80000000, size=160k-8k);
        map not_cached(dest=bus:sri, dest_offset=0xA0000000, size=160k-8k, reserved);
    }
    memory pflash_training_inttab (tag="on-chip")
    {
        mau = 8;
        type = rom;
        size = 8k;
        priority = 8;
        map cached(dest=bus:sri, dest_offset=0x80026000, size=8k);
        map not_cached(dest=bus:sri, dest_offset=0xA0026000, size=8k, reserved);
    }
    memory dflash0 (tag="on-chip")
    {
        mau = 8;
        type = reserved nvram;
        size = 128k+16k;
        map (dest=bus:sri, src_offset=0, dest_offset=0xAF000000, size=128k);
        map (dest=bus:sri, src_offset=128k, dest_offset=0xAF100000, size=16k);
    }
}
#else
#include "tc23x.lsl"
#endif

// -----------------------------------------------------------------------------
// Interrupt & trap Vector table Section
// -----------------------------------------------------------------------------
section_layout :vtc:linear
{
    "_lc_u_int_tab" = (INTTAB);
    "_lc_u_int_tab_tc0" = (INTTAB);

    // interrupt vector table
    group int_tab (ordered, contiguous, align = 4,run_addr=mem:mpe:pflash_training_inttab)
    {
      select ".text.inttab*";
    }
	
    "_lc_u_trap_tab" = (IFXTRAPTAB);
    "_lc_u_trap_tab_tc0" = (IFXTRAPTAB);
    
    // trapvector table for CPU0
    group (ordered, contiguous, align = 32, run_addr=(IFXTRAPTAB)+0x0000)
    {
        select ".text.CPU0_TRAP_HANDLER_CODE_ROM";
    }
}

// -----------------------------------------------------------------------------
// ETC Section
// -----------------------------------------------------------------------------
section_layout :tc0:linear
{
    group ( run_addr = 0x70023C00, ordered ) stack "ustack_tc0" (size = 40k,tag="user");
}
section_layout :tc0:linear
{
    group ( run_addr = 0x7002DC00, ordered ) stack "istack_tc0" (size = 1k,tag="user");
}

