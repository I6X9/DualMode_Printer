/******************************************************************
 *                                                                *
 *        Copyright Mentor Graphics Corporation 2004              *
 *                                                                *
 *                All Rights Reserved.                            *
 *                                                                *
 *    THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION *
 *  WHICH IS THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS   *
 *  LICENSORS AND IS SUBJECT TO LICENSE TERMS.                    *
 *                                                                *
 ******************************************************************/

#ifndef __MUSB_NONE_AFS_MEMORY_H__
#define __MUSB_NONE_AFS_MEMORY_H__

/*
 * AFS-specific memory abstraction
 * $Revision: 1.4 $
 */

//#include "uhal.h"

//#include "os_port.h"

#include "os_common.h"

#define j_memset                os_memset
#define j_memcpy                os_memcpy

extern void* MGC_AfsMemRealloc(void* pBuffer, uint32_t iSize);
extern void *print_jmalloc(uint32_t isize);
extern void print_jfree(void *addr);
extern void *os_memcpy(void *out, const void *in, size_t n);
extern void *os_memset(void *b, int c, size_t len);

#define MUSB_MemAlloc(a) print_jmalloc(a)//jmalloc(a,0)
#define MUSB_MemRealloc MGC_AfsMemRealloc
#define MUSB_MemFree print_jfree//jfree
#define MUSB_MemCopy(_pDest, _pSrc, _iSize) \
    j_memcpy((void*)_pDest, (void*)_pSrc, _iSize)
#define MUSB_MemSet(_pDest, _iData, _iSize) \
    j_memset((void*)_pDest, _iData, _iSize)

#endif	/* multiple inclusion protection */
