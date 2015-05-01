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
** <L5_PRIVATE L5_SOURCE>
** \author  djr
**  \brief  Kernel thread abstraction.
**   \date  2004/12/09
**    \cop  (c) Level 5 Networks Limited.
** </L5_PRIVATE>
*//*
\**************************************************************************/

/*! \cidoxg_lib_citools */
#include "citools_internal.h"

#ifndef __KERNEL__
# error "__KERNEL__ is not defined."
#endif



#include <linux/kthread.h>

static int linux_kernel_thread_wrapper(void* arg)
{
  ci_kernel_thread_t* kt = (ci_kernel_thread_t*) arg;

  kt->fn(kt->arg);

  complete_and_exit(&kt->exit_event, 0);
  return 0;
}


int cithread_create(cithread_t* tid, void* (*fn)(void*), void* arg,
		    const char* name)
{
  ci_kernel_thread_t* kt;

  ci_assert(tid);

  *tid = kt = (ci_kernel_thread_t*) ci_alloc(sizeof(ci_kernel_thread_t));
  if( kt == 0 )  return -ENOMEM;

  kt->name = name;
  kt->fn = fn;
  kt->arg = arg;
  kt->thrd_id = NULL;
  init_completion(&kt->exit_event);

  kt->thrd_id = kthread_create(linux_kernel_thread_wrapper, kt, kt->name);
  wake_up_process(kt->thrd_id);
  if( kt->thrd_id == NULL ) {
    CI_DEBUG(ci_log("%s: kthread_create %s failed", __FUNCTION__,
		    kt->name ? kt->name : ""));
    complete(&kt->exit_event);
  }

  return 0;
}

int cithread_detach(cithread_t kt)
{
  /* TODO */
  ci_assert(0);
  return -EOPNOTSUPP;
}


int cithread_join(cithread_t kt)
{
  wait_for_completion(&kt->exit_event);
  ci_free(kt);
  return 0;
}



/*! \cidoxg_end */
