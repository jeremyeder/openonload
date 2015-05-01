/*
** Copyright 2005-2014  Solarflare Communications Inc.
**                      7505 Irvine Center Drive, Irvine, CA 92618, USA
** Copyright 2002-2005  Level 5 Networks Inc.
**
** This program is free software; you can redistribute it and/or modify it
** under the terms of version 2 of the GNU General Public License as
** published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

/**************************************************************************\
*//*! \file
** <L5_PRIVATE L5_HEADER >
** \author  
**  \brief  
**   \date  
**    \cop  (c) Level 5 Networks Limited.
** </L5_PRIVATE>
*//*
\**************************************************************************/

/*! \cidoxg_include_ci_tools_platform  */

#ifndef __CI_TOOLS_PLATFORM_UL_COMMON_H__
#define __CI_TOOLS_PLATFORM_UL_COMMON_H__


/**********************************************************************
 * Memory allocation
 */
#define ci_alloc                           malloc
#define ci_atomic_alloc                    malloc
#define ci_calloc                          calloc
#define ci_realloc(ptr, oldsize, newsize)  realloc((ptr), (newsize))

#define ci_vmalloc                         malloc
#define ci_vmalloc_fn                      malloc     
#define ci_vfree                           free

#define ci_sprintf                         sprintf
#define ci_snprintf                        snprintf
#define ci_vsprintf                        vsprintf
#define ci_vsnprintf                       vsnprintf
#define ci_sscanf                          sscanf

/* ensure ci_alloc_fn and ci_free have the correct definitions so we can safely 
** pass around as function pointers. Without this, Windows user-level driver
** gets clashes of calling convention 
*/
ci_inline void *
ci_alloc_fn (size_t size)
{
  return malloc(size);
}

ci_inline void 
ci_free(void * p)
{
  free(p);
}


/*--------------------------------------------------------------------
 *
 * in_interrupt and in_atomic macros .. well portable code shouldn't
 * be using it, and definitely not for anything other than picking
 * which memory alloc routine to use
 *
 *--------------------------------------------------------------------*/

#define ci_in_interrupt() 0
#define ci_in_atomic() 0
#define ci_in_irq() 0


#endif  /* __CI_TOOLS_PLATFORM_UL_COMMON_H__ */
/*! \cidoxg_end */
