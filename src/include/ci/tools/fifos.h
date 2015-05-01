/*
** Copyright 2005-2013  Solarflare Communications Inc.
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
** \author  djr
**  \brief  Fifos for integers and pointers.
**   \date  
**    \cop  (c) Level 5 Networks Limited.
** </L5_PRIVATE>
*//*
\**************************************************************************/

/*! \cidoxg_include_ci_tools */

#ifndef __CI_TOOLS_FIFOS_H__
#define __CI_TOOLS_FIFOS_H__

/* Make this header self-sufficient */
#include <ci/compat.h>
#include <ci/tools/log.h>
#include <ci/tools/debug.h>
#include <ci/tools/sysdep.h>

#include <ci/tools/config.h>
#include <ci/tools/utils.h>
#include <ci/tools/spinlock.h>
#include <ci/tools/fifo.h>


/*! Comment? */
typedef struct {
  int*      fifo;
  unsigned  fifo_size;
  unsigned  fifo_rd_i;
  unsigned  fifo_wr_i;
} ci_int_fifo;

typedef ci_int_fifo ci_int_fifo_t;

CI_FIFO_DEFINE_GET(ci_int_fifo_get, ci_int_fifo, int)


/*! Comment? */
typedef struct {
  int*      fifo;
  unsigned  fifo_mask;
  unsigned  fifo_rd_i;
  unsigned  fifo_wr_i;
} ci_int_fifo2;

typedef ci_int_fifo2 ci_int_fifo2_t;

CI_FIFO2_DEFINE_GET(ci_int_fifo2_get, ci_int_fifo2, int)

/**********************************************************************/

/*! Comment? */
typedef struct {
  void**    fifo;
  unsigned  fifo_size;
  unsigned  fifo_rd_i;
  unsigned  fifo_wr_i;
} ci_ptr_fifo;

typedef ci_ptr_fifo ci_ptr_fifo_t;

CI_FIFO_DEFINE_GET(ci_ptr_fifo_get, ci_ptr_fifo, void*)


/*! Comment? */
typedef struct {
  void**    fifo;
  unsigned  fifo_mask;
  unsigned  fifo_rd_i;
  unsigned  fifo_wr_i;
} ci_ptr_fifo2;

typedef ci_ptr_fifo2 ci_ptr_fifo2_t;

CI_FIFO2_DEFINE_GET(ci_ptr_fifo2_get, ci_ptr_fifo2, void*)

/**********************************************************************/

/*! Comment? */
typedef struct {
  char*		fifo;
  unsigned	fifo_size;
  unsigned	fifo_rd_i;
  unsigned	fifo_wr_i;
} ci_byte_fifo;

typedef ci_byte_fifo ci_byte_fifo_t;

CI_FIFO_DEFINE_GET(ci_byte_fifo_get, ci_byte_fifo, char)


/*! Comment? */
typedef struct {
  char*		fifo;
  unsigned	fifo_mask;
  unsigned	fifo_rd_i;
  unsigned	fifo_wr_i;
} ci_byte_fifo2;

typedef ci_byte_fifo2 ci_byte_fifo2_t;

CI_FIFO2_DEFINE_GET(ci_byte_fifo2_get, ci_byte_fifo2, char)


#endif  /* __CI_TOOLS_FIFOS_H__ */

/*! \cidoxg_end */
