/* 
 * Script for GNU linker.
 * Describes layout of sections, location of stack.
 *
 * In this case vectors are at location 0 (reset @ 0x08)
 *
 * +------------+  0x00400000
 * data         |
 * end
 * |(heap)      |
 * .            .
 * .            .
 * |(heap limit)|
 *
 * |- - - - - - |
 * stack bottom    64k
 * +------------+  
 *
 * +------------+  0x0000000
 * |vectors     |
 * |            |
 * |------------+
 * |text        |  0x0000300
 * |data        |
 * |            |  512k
 * +------------+
 */

/* memory:
 *  D-QMEM                     64kB (start at 00400000)
 *  IAHB-MEM EBI Flash         96kB (start at 00000000, mapped to 00000000)
 *  CACHES 2way x8K/way        16kB
 */

/* Split memory into area for vectors and ram */
MEMORY
	{
            boot_section    (rx)  :    ORIGIN = 0x00000000,    LENGTH = 8k
            flash           (rx)  :    ORIGIN = 0x00005A80,    LENGTH = 4M
            ram             (rw!x):    ORIGIN = 0x00400000,    LENGTH = 56k
            sbc_mem         (rw!x):    ORIGIN = 0x0040E000,    LENGTH = 18k
            mem_pool        (rw!x):    ORIGIN = 0x00412800,    LENGTH = 38k
            shmem           (rw!x):    ORIGIN = 0x00E00000,    LENGTH = 64k    
	}

/* start execution at default reset vector location */
ENTRY(_reset_vector);
_reset_vector = 0x00000008;

SECTIONS
{
/* vectors go to vectors region */
	.boot_vectors : 
        {
		*(.boot_vectors)
		. = ALIGN (8);
		*(boot_code boot_code.*) 
		. = ALIGN (8);           
	} > boot_section
	
	.vectors : 
	{ 
		*(.vectors)
		. = ALIGN (8);
	} > flash
	

/* instructions go to the text region*/

	. = ALIGN(0x8);
	.dram_code :
	{
		*(dram_code dram_code.*)
		. = ALIGN(0x8); 
	} > flash
/* code, instructions    i=i+1; */
	.flash_code : 
	{
		. = ALIGN (4);
		*(ram_code ram_code.*)
		. = ALIGN (4);
		/* *(mp3_code mp3_code.*) */
		/* . = ALIGN (4); */
		*(ram_test_code ram_test_code.*)
		. = ALIGN (4);
		*(flash_code flash_code.* )
        . = ALIGN(0x8);
	} > flash

/* code, instructions    i=i+1; */
	.text : 
	{
		*(.text .text.* ) 
	} > flash

/* read only data    const int rom_data[3]={1,2,3}; */
	.rodata ALIGN(8) : 
	{
		*(.rodata)
		. = ALIGN (4);
		*(.rodata.*)
	} > flash					/* in flash */
	
/*.ceva_code_data 0xa0000 :*/
	/*
	.ceva_code_data ALIGN(8) :
	{
		*(ceva_code.flash)
		*(ceva_code.tcm)
		*(ceva_data)
		
	} > flash
	*/

	. = ORIGIN(ram);

/* globals  int ram_data[3]={4,5,6}; */
	.data ALIGN(8) : AT( ALIGN( LOADADDR(.rodata)+SIZEOF(.rodata), 4 ) )		/* VMA in RAM, but keep LMA in flash */
	{
		*(.data .data.*)
		/* *(.rodata) */
		/* *(.rodata.*) */
	} >ram
	
	/* Loader will copy data from _flash_begin to _ram_begin..ram_end */
	_data_flash_begin = LOADADDR(.data);
	_data_ram_begin = ADDR(.data);
	_data_ram_end = .;

/* small read-write data section */
	.sdata ALIGN(8) : AT( ALIGN( LOADADDR(.data)+SIZEOF(.data), 4 ) )		/* VMA in RAM, but keep LMA in flash */
	{ 
		*(.sdata) 
	} >ram

	/* Loader will copy sdata from _flash_begin to ram_begin..ram_end */
	_sdata_flash_begin = LOADADDR(.sdata);
	_sdata_ram_begin = ADDR(.sdata);
	_sdata_ram_end = .;


/* frame unside informatino for exception handling */
	/DISCARD/ :
	{
		*(.eh_frame)
	}


/* global pointer in middle of data, so as much as possible is accessible */
	. = ALIGN(4);  __gp = . + 0x1000;


/* uninitialized data section - global   int i; */
	.bss ALIGN(8):
	{
		_bss_start = .;
		. = ALIGN (4);
		*(.bss .bss.*)
		. = ALIGN (4);
		_bss_end = .;
	} > ram						/* in RAM */

	. = ALIGN (8);
	_empty_ram = .;

/* small uninitialized data section */
	.sbss ALIGN(8): 
	{
		_sbss_start = .;
		. = ALIGN (4);
		*(.scommon)
		. = ALIGN (4);
		*(.sbss)
        . = ALIGN (4);
		_sbss_end = .;
	} > ram

/* This symbol defines end of code/data sections. Heap starts here. */
	PROVIDE(end    	  = .);
	
/* Define symbol for heap limit. Heap spans from end to here. When this is exhausted,
   heap allocations will fail. If you don't use heap (malloc etc.) this is irrelevant.
   This also defines the size of stack that will not be overwritten by dynamic
   allocations. If you need more/less stack, modify below definition.
   Default memory area reserved for stack is 8k here.
 */
	
/* _stack symbol defines initial stack bottom addres. Stack grows to lower addresses.
   Typically you set this to be top of your RAM. Note: code never checks, if stack
   grows into heap area!
*/
	PROVIDE(_heap_limit = 4M + 56k - 8k);
	PROVIDE(_stack      = 4M + 56k);
	
	PROVIDE(_sbcmem_begin      = 4M + 56k);
	PROVIDE(_sbcmem_end      = 4M + 56k + 18k - 4);
	PROVIDE(_mempool_begin      = 4M + 56k + 18k);
	PROVIDE(_mempool_end      = 4M + 56k + 18k + 38k);
	

	PROVIDE(_shdata_ram_begin = 0x00E00000);
	PROVIDE(_shdata_ram_OPUS = 0x00E00000 + 4k);
	PROVIDE(_shdata_ram_end = 0x00E00000 + 64k);
	
}

