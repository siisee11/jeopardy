//-------------------------------------------------------------------
//	pkmap.c
//
//	This module shows values for some kernel memory-management
//	constants and variables, displays some address ranges, and
//	tests use of the 'kmap_atomic()' function (which creates a 
//	temporary mapping for a physical page in high memory).
//
//	NOTE: Written and tested using kernel version 2.4.20-smp.
//
//	programmer: ALLAN CRUSE
//	written on: 09 MAR 2003
//-------------------------------------------------------------------

#define	__KERNEL__
#define	  MODULE  

#include <linux/module.h>	// for init_module() 
#include <linux/proc_fs.h>	// for create_proc_read_entry() 
#include <linux/utsname.h>	// for system_utsname
#include <linux/highmem.h>	// for PKMAP_BASE

#define	SUCCESS	0

static char modname[] = "pkmap";


static int my_proc_read( char *buf, char **start, off_t off,
					int count, int *eof, void *data )
{
	unsigned long	struct_page_size = sizeof( struct page );
	unsigned long	mem_map_size = num_physpages * struct_page_size;
	unsigned long	mem_map_start, mem_map_end, pkmap_end;
	void		*vaddr;
	int		i, j, len;

	len = 0;
	len += sprintf( buf+len, "\nhost: \'%s\'\n", system_utsname.nodename );
	len += sprintf( buf+len, "\nPKMAP_BASE=%08X  ", PKMAP_BASE );
	len += sprintf( buf+len, "\nLAST_PKMAP=%08X  ", LAST_PKMAP );
	len += sprintf( buf+len, "\n" );

	mem_map_start = (unsigned long)mem_map;
	mem_map_end = mem_map_start + mem_map_size;
	len += sprintf( buf+len, "\nmem_map: %08X-", mem_map_start );
	len += sprintf( buf+len, "%08X  ", mem_map_end );
	len += sprintf( buf+len, "num_physpages=%08X \n", num_physpages );
	len += sprintf( buf+len, "\nhighmem_start_page=%08X \n", 
						highmem_start_page );

	if ( mem_map_end == (unsigned long)highmem_start_page )
		len += sprintf( buf+len, "No high memory pages exist\n" );
	else	{
		j = 0;
		for (i = 0; i < LAST_PKMAP; i++)
			{
			struct page	*page = highmem_start_page+i;	
			if ( !(page->virtual) ) continue; else ++j;
			if ( ( j % 8 ) == 0 ) len += sprintf( buf+len, "\n" );
			len += sprintf( buf+len, "%08X ", page->virtual );	
			}
		len += sprintf( buf+len, "\n" );
		}
	
	len += sprintf( buf+len, "\n     high_memory=%08X \n", high_memory );
	len += sprintf( buf+len, "\n  VMALLOC_OFFSET=%08X \n", VMALLOC_OFFSET );

	len += sprintf( buf+len, "\n   VMALLOC_START=%08X ", VMALLOC_START );
	len += sprintf( buf+len, "\n     VMALLOC_END=%08X \n", VMALLOC_END );
	
	pkmap_end = PKMAP_BASE + LAST_PKMAP * PAGE_SIZE;
	len += sprintf( buf+len, "\n      PKMAP_BASE=%08X ",    PKMAP_BASE );
	len += sprintf( buf+len, "\n       pkmap_end=%08X \n",  pkmap_end  );

	len += sprintf( buf+len, "\n   FIXADDR_START=%08X ", FIXADDR_START );
	len += sprintf( buf+len, "\n     FIXADDR_TOP=%08X \n", FIXADDR_TOP );
	
	len += sprintf( buf+len, "\n  KM_TYPE_NR=%d  ", KM_TYPE_NR );
	len += sprintf( buf+len, "  NR_CPUS=%d  ", NR_CPUS );
	len += sprintf( buf+len, "\n  FIX_KMAP_BEGIN=%08X ", FIX_KMAP_BEGIN );
	len += sprintf( buf+len, "\n    FIX_KMAP_END=%08X \n", FIX_KMAP_END );

	// try setting up some 'temporary' kernel mappings
	len += sprintf( buf+len, "\ncpu=%d  vaddr: ", current->processor );
	for (i = 0; i < KM_TYPE_NR; i++)
		{
		vaddr = kmap_atomic( &mem_map[ num_physpages-1-i ], i );
		len += sprintf( buf+len, "%08X ", vaddr );
		}
	len += sprintf( buf+len, "\n\n" );

	*eof = 1;
	return	len;
}


void cleanup_module( void )
{
	remove_proc_entry( modname, NULL );
	printk( "<1>Removing \'%s\' module\n", modname );
}


int init_module( void )
{
	printk( "<1>\nInstalling \'%s\' module\n", modname );
	create_proc_read_entry( modname, 0, NULL, my_proc_read, NULL );
	return	SUCCESS;
}

MODULE_LICENSE("GPL"); 
