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
** <L5_PRIVATE L5_HEADER>
** \author  adp
**  \brief  Simple memory access checker.
**   \date  2004/07/21
**    \cop  (c) Level 5 Networks Limited.
** </L5_PRIVATE>
*//*
\**************************************************************************/

/*! \cidoxg_include_ci_tools */
#ifndef __CI_TOOLS_MEMCHK_H__
#define __CI_TOOLS_MEMCHK_H__

  
/********
 * Memroy check interface
 */

/* MASK VALUES; */
#define CI_EXT_NONE     0x0
#define CI_EXT_READ     0x1
#define CI_EXT_WRITE    0x2

#define CI_EXT_RDWR     0x3 /* EXT_READ | EXT_WRITE */


/*
 * Register a piece of memory with the checker
 *
 * p -> the start address of the memory
 * len -> the length in bytes of the memory
 * mask -> whether we can read or write from/to the memory
 *
 * Non-zero if no problems occurred (can generally ignore the
 * return value).
 */
int ci_memregister(void* p, long len, long mask);

/*
 * Unregister a piece of memory from the checker
 *
 * p -> the start address of the memory
 * len -> the length of the buffer (as passed before)
 *
 * Non-zero if no problems occurred (can generally ignore the
 * return value).
 */
int ci_unregister(void* p, long mask);

/*
 * Checks that we are okay to read a range of memory
 * 
 * p -> the start address of the memory
 * len -> the length of the buffer
 *
 * Non-zero if we are okay to read the memory, 0 otherwise.
 */
int ci_readcheck(void *p, long len);

/* 
 * Checks that we are okay to write to a range of memroy
 * This function DOES NOT check for writes across two extents
 *
 * p -> the start address of the memory
 * len -> the length of the buffer
 *
 * None-zero if we are okay to write the memory, 0 otherwise.
 */
int ci_writecheck(void *p, long len);


#endif  /* __CI_TOOLS_MEMCHK_H__ */
/*! \cidoxg_end */
