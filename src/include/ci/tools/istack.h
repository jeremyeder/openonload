/*
** Copyright 2005-2015  Solarflare Communications Inc.
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
**  \brief  Trivial pointer-free stack.
**   \date  2003/06/30
**    \cop  (c) Level 5 Networks Limited.
** </L5_PRIVATE>
*//*
\**************************************************************************/

/*! \cidoxg_include_ci_tools */

#ifndef __CI_TOOLS_ISTACK_H__
#define __CI_TOOLS_ISTACK_H__

/*
** [s] is a pointer to a data structure that should have at least the
** following members (or similar):
**
**   struct int_istack {
**     int      istack_size;
**     int      istack_ptr;
**     elem_t   istack_base[n];   (or an elem_t*)
**   };
*/

#define ci_istack_init(s, capacity)		\
  do{ (s)->istack_ptr = 0;			\
      (s)->istack_size = (capacity);		\
  }while(0)

#define ci_istack_valid(s)  ((s) && (s)->istack_base             &&	\
			     (s)->istack_size >= 0               &&	\
			     (s)->istack_ptr >= 0                &&	\
			     (s)->istack_ptr <= (s)->istack_size   )

/* ?? These should be purged. */
#define ci_istack_empty(s)     ((s)->istack_ptr == 0)
#define ci_istack_full(s)      ((s)->istack_ptr == (s)->istack_size)

#define ci_istack_is_empty(s)  ((s)->istack_ptr == 0)
#define ci_istack_not_empty(s) ((s)->istack_ptr)
#define ci_istack_is_full(s)   ((s)->istack_ptr == (s)->istack_size)
#define ci_istack_not_full(s)  ((s)->istack_ptr != (s)->istack_size)
#define ci_istack_num(s)       ((s)->istack_ptr)
#define ci_istack_space(s)     ((s)->istack_size - (s)->istack_ptr)
#define ci_istack_capacity(s)  ((s)->istack_size)

#define ci_istack_push(s, v)   ((s)->istack_base[(s)->istack_ptr++] = (v))
#define ci_istack_pop(s)       ((s)->istack_base[--(s)->istack_ptr])

#define ci_istack_peek(s)      ((s)->istack_base[(s)->istack_ptr - 1])
#define ci_istack_peek_i(s,i)  ((s)->istack_base[(s)->istack_ptr - 1 - (i)])
#define ci_istack_poke(s)      ((s)->istack_base[(s)->istack_ptr])


#endif  /* __CI_TOOLS_ISTACK_H__ */

/*! \cidoxg_end */
