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
** \author  kjm
**  \brief  onload_set_stackname() extension.
**   \date  2010/12/11
**    \cop  (c) Solarflare Communications Inc.
** </L5_PRIVATE>
*//*
\**************************************************************************/

#define _GNU_SOURCE /* for dlsym(), RTLD_NEXT, etc */

#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <dlfcn.h>

#include "internal.h"
#include <onload/extensions.h>
#include <onload/ul/stackname.h>
#include <ci/internal/tls.h>

#if CI_CFG_USERSPACE_PIPE
#include "ul_pipe.h"
#endif


static struct oo_stackname_state oo_stackname_global;


static int oo_stackname_get_suffix(enum onload_stackname_scope context,
                                       char *suffix)
{
  switch( context ) {
  case ONLOAD_SCOPE_NOCHANGE:
  case ONLOAD_SCOPE_GLOBAL:
    suffix[0] = '\0';
    break;
  case ONLOAD_SCOPE_THREAD:
    snprintf(suffix, CI_CFG_STACK_NAME_LEN, "-t%d", (int)syscall(SYS_gettid));
    break;
  case ONLOAD_SCOPE_PROCESS:
    snprintf(suffix, CI_CFG_STACK_NAME_LEN, "-p%d", getpid());
    break;
  case ONLOAD_SCOPE_USER:
    snprintf(suffix, CI_CFG_STACK_NAME_LEN, "-u%d", getuid());
    break;
  default:
    return -1;
  }
  return 0;
}


static void
oo_stackname_rescope_local(struct oo_stackname_state *state)
{
  char suffix[CI_CFG_STACK_NAME_LEN];
  int rc;

  if( oo_stackname_global.stackname[0] == 1 ) {
    /* Don't accelerate, so don't try and append suffix */
    state->scoped_stackname[0] = 1;
  }
  else {
    rc = oo_stackname_get_suffix(state->context, suffix);
    ci_assert(rc == 0);

    strncpy(state->scoped_stackname, 
            oo_stackname_global.stackname,
            CI_CFG_STACK_NAME_LEN);
    /* This should be guaranteed by onload_set_stackname() */
    ci_assert(strlen(state->scoped_stackname) + strlen(suffix) <
              CI_CFG_STACK_NAME_LEN);
    strncat(state->scoped_stackname, suffix, 
            CI_CFG_STACK_NAME_LEN - strlen(state->scoped_stackname) - 1);
  }
  state->sequence = oo_stackname_global.sequence;
}


static void 
oo_stackname_rescope_global(struct oo_stackname_state *state)
{
  if( oo_stackname_global.stackname[0] == 1 ) {
    oo_stackname_global.scoped_stackname[0] = 1; 
  }
  else if( state->context != ONLOAD_SCOPE_THREAD ) {
    /* If not-per-thread scope is requested, we can store it in the
     * global state
     */
    strncpy(oo_stackname_global.scoped_stackname, 
            state->scoped_stackname, CI_CFG_STACK_NAME_LEN);
  }
  else {
    /* We'll update the thread's scoped_stackname next time it's
     * needed, leave global one blank as it should not be used 
     */
    oo_stackname_global.scoped_stackname[0] = '\0';
  }

  ++oo_stackname_global.sequence;
}


static void 
oo_stackname_sync_global(struct oo_stackname_state *state)
{
  oo_stackname_global.who = state->who;
  oo_stackname_global.context = state->context;

  if( state->stackname[0] == 1 ) {
    /* magic value for "don't accelerate */
    oo_stackname_global.stackname[0] = 1;
  } 
  else {
    strncpy(oo_stackname_global.stackname, state->stackname, 
            CI_CFG_STACK_NAME_LEN);
  }
  
  oo_stackname_rescope_global(state);
}


int onload_set_stackname(enum onload_stackname_who who,
                         enum onload_stackname_scope context, 
                         const char* stackname)
{
  char suffix[CI_CFG_STACK_NAME_LEN];
  struct oo_stackname_state *state = 
    oo_stackname_thread_get();

  if( who != ONLOAD_THIS_THREAD && who != ONLOAD_ALL_THREADS ) {
    errno = EINVAL;
    return -1;
  }

  if( who == ONLOAD_THIS_THREAD && context == ONLOAD_SCOPE_NOCHANGE ) {
    errno = EINVAL;
    return -1;
  }

  if( stackname != ONLOAD_DONT_ACCELERATE ) {
    if( stackname[0] == 1 ) {
      errno = EINVAL;
      return -1;
    }
    
    /* Make sure we have room for the suffix */
    if( strlen(stackname) > CI_CFG_STACK_NAME_LEN - 8 ) {
      errno = ENAMETOOLONG;
      return -1;
    }
  }

  if( oo_stackname_get_suffix(context, suffix) ) {
    errno = EINVAL;
    return -1;
  }

  CITP_LOCK(&citp_ul_lock);

  state->who = who;
  state->context = context;

  if( context != ONLOAD_SCOPE_NOCHANGE ) {
    if( stackname == ONLOAD_DONT_ACCELERATE )
      state->stackname[0] = state->scoped_stackname[0] = 1; /* magic number */
    else {
      strncpy(state->stackname, stackname, CI_CFG_STACK_NAME_LEN);
      strncpy(state->scoped_stackname, stackname, CI_CFG_STACK_NAME_LEN);
      strncat(state->scoped_stackname, suffix, 
              CI_CFG_STACK_NAME_LEN - strlen(state->scoped_stackname) - 1);
    }

    if( who == ONLOAD_ALL_THREADS )
      oo_stackname_sync_global(state);
  }

  CITP_UNLOCK(&citp_ul_lock);
  return 0;
}


void oo_stackname_get(char **stackname)
{
  struct oo_stackname_state *state = 
    oo_stackname_thread_get();

  ci_assert(state != NULL);
  ci_assert(CITP_ISLOCKED(&citp_ul_lock));

  /* First look at this thread's configuration to decide where to
   * search for the stack */
  if( state->who == ONLOAD_THIS_THREAD ) {
    *stackname = state->scoped_stackname;
  }
  else {
    /* Try looking at the global configuration instead */
    if( oo_stackname_global.context != ONLOAD_SCOPE_THREAD )
      *stackname = oo_stackname_global.scoped_stackname;
    else {
      /* See if we've already cached this thread's scoped stackname in
       * the thread local storage.  If so, use that, if not generate
       * the cached copy
       */
      if( state->sequence != oo_stackname_global.sequence )
        oo_stackname_rescope_local(state);

      *stackname = state->scoped_stackname;
    }
  }

  if( (*stackname)[0] == 1 ) {
    /* don't accelerate */
    *stackname = ONLOAD_DONT_ACCELERATE;
    return;
  }

  return;
}


static void
oo_stackname_update_local_suffix(struct oo_stackname_state *state,
                                     enum onload_stackname_scope context)
{
  char suffix[CI_CFG_STACK_NAME_LEN];
  int rc;

  /* First obtain the appropriate base stackname */
  if( state->who == ONLOAD_THIS_THREAD ) {
    strncpy(state->scoped_stackname, state->stackname, 
            CI_CFG_STACK_NAME_LEN);
  }
  else {
    strncpy(state->scoped_stackname, oo_stackname_global.stackname,
            CI_CFG_STACK_NAME_LEN);
    state->sequence = oo_stackname_global.sequence;
  }
    
  /* Only append suffix if it's not the special "don't accelerate" name */
  if( state->scoped_stackname[0] != 1 ) {
    rc = oo_stackname_get_suffix(context, suffix);
    ci_assert(rc == 0);

    strncat(state->scoped_stackname, suffix, 
            CI_CFG_STACK_NAME_LEN - strlen(state->scoped_stackname) - 1);
  }
}


static void 
oo_stackname_update_local_state(struct oo_stackname_state *state,
                                    struct oo_stackname_state *cache)
{
  enum onload_stackname_scope context;

  ci_assert(state != NULL);
  ci_assert(cache != NULL);
  ci_assert(CITP_ISLOCKED(&citp_ul_lock));

  state->context = cache->context;
  state->who = cache->who;
  strncpy(state->stackname, cache->stackname, CI_CFG_STACK_NAME_LEN);
  state->sequence = cache->sequence;

  if( state->who == ONLOAD_THIS_THREAD )
    context = state->context;
  else 
    context = oo_stackname_global.context;

  ci_assert(context != ONLOAD_SCOPE_NOCHANGE);

  if( context == ONLOAD_SCOPE_GLOBAL ) {
    strncpy(state->scoped_stackname, cache->scoped_stackname, 
            CI_CFG_STACK_NAME_LEN);
  }
  else {
    /* Remaining scopes have a suffix that might need to change. */
    oo_stackname_update_local_suffix(state, context);
  }
}


static void 
oo_stackname_update_global_state(struct oo_stackname_state *state)
{
  /* Stackname shouldn't have changed in this circumstance (currently
   * only called post fork) but the suffix may, so just rescope for now
   */
  if( state->who == ONLOAD_ALL_THREADS )
    oo_stackname_rescope_global(state);
}


/* This will, if the supplied cache of the state from before the
 * change is not-NULL, sort out the stackname state when
 * thread/process has changed (e.g. across fork) and there may be no
 * local state existing.
 * 
 * If the supplied state is NULL then it is assumed that the local
 * state already exists, and it will sort out any suffix changes
 * (e.g. across setuid())
 */
void oo_stackname_update(struct oo_stackname_state *cache)
{
  struct oo_stackname_state *state = 
    oo_stackname_thread_get();

  ci_assert(CITP_ISLOCKED(&citp_ul_lock));

  if( cache )
    oo_stackname_update_local_state(state, cache);
  else {
    if( state->who == ONLOAD_THIS_THREAD )
      oo_stackname_update_local_suffix(state, state->context);
    else 
      oo_stackname_update_local_suffix
        (state, oo_stackname_global.context);
  }

  oo_stackname_update_global_state(state);
}


void oo_stackname_state_init(struct oo_stackname_state *spt)
{
  char *s;

  spt->context = ONLOAD_SCOPE_GLOBAL;
  spt->who = ONLOAD_ALL_THREADS;
  memset(spt->stackname, 0, sizeof(spt->stackname));
  memset(spt->scoped_stackname, 0, sizeof(spt->scoped_stackname));
  spt->sequence = 0;

  if( (s = getenv("EF_NAME")) != NULL ) {
    spt->context = ONLOAD_SCOPE_GLOBAL;
    strncpy(spt->scoped_stackname, s, CI_CFG_STACK_NAME_LEN);
  }

  if( CITP_OPTS.stack_per_thread ) {
    spt->context = ONLOAD_SCOPE_THREAD;
    snprintf(spt->scoped_stackname, CI_CFG_STACK_NAME_LEN, 
             "-t%d", (int)syscall(SYS_gettid));
  }

  if( CITP_OPTS.dont_accelerate )
    spt->stackname[0] = spt->scoped_stackname[0] = 1;
}


void oo_stackname_thread_init(struct oo_stackname_state* snpts)
{
  oo_stackname_state_init(snpts);
}


void oo_stackname_init(void)
{
  oo_stackname_state_init(&oo_stackname_global);
}
