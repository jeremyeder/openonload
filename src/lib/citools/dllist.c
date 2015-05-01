/*
** Copyright 2005-2012  Solarflare Communications Inc.
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
 ** <L5_PRIVATE L5_SOURCE>
 ** \author  
 **  \brief  
 **   \date  
 **    \cop  (c) Level 5 Networks Limited.
 ** </L5_PRIVATE>
 *//*
 \**************************************************************************/
 
 /*! \cidoxg_lib_citools */
 
#include "citools_internal.h"


#if CI_INCLUDE_ASSERT_VALID
void ci_dllist_assert_valid(ci_dllist* list)
{
  ci_dllink* l;

  ci_assert(list);
  ci_assert(list->l.next);
  ci_assert(list->l.prev);

  for( l = list->l.next; l != &list->l; l = l->next ) {
    ci_assert(l);
    ci_assert(l->prev);
  }
}
#endif

/*! \cidoxg_end */
