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
*//*! \file netif_init.c
** <L5_PRIVATE L5_SOURCE>
** \author  stg
**  \brief  Common functionality used by TCP & UDP
**   \date  2004/06/09
**    \cop  (c) Level 5 Networks Limited.
** </L5_PRIVATE>
*//*
\**************************************************************************/
  
/*! \cidoxg_lib_transport_unix */

#include <ci/internal/ip.h>
#include <ci/internal/transport_config_opt.h>
#include <ci/internal/transport_common.h>

/* This breaks the "common" code separation by including stuff from
 * unix directly with no windows equivalent implemented */
#include <onload/ul/stackname.h>

#include <onload/ul/tcp_helper.h>
#include <onload/ul.h>

#define LPF "citp_netif_"
#define LPFIN "-> " LPF
#define LPFOUT "<- " LPF


#define VERB(x)

/*
 * Support for n netif per lib. 
 * 
 * There is one pool of netifs - "general" (each call to socket()
 * results in an allocation from this pool)
 *
 * Netifs from the pool are constructed on demand.  A netif from the
 * general pool is allocated each time citp_netif_alloc_and_init() is
 * called.  Currently there's 1 netif in this pool.  The general pool
 * is the one that's checked for logging etc.
 */

/* ***************************
 * Global vars
 */



/* ***************************
 * Local vars
 */

/* All known netifs  */
static ci_dllist citp_active_netifs;

/* set after first call to citp_netif_startup */
static int citp_netifs_inited=0;

/* The netif destructor mode of operation */
static unsigned citp_netif_dtor_mode = CITP_NETIF_DTOR_ONLY_SHARED;


/* ***************************
 *  Local Functions
 */

ci_inline void __citp_add_netif( ci_netif* ni )
{
  /* Requires that the FD table write lock has been taken */

  ci_assert( ni );
  CI_MAGIC_CHECK(ni, NETIF_MAGIC);
  ci_assert( citp_netifs_inited );
  ci_assert( CITP_ISLOCKED(&citp_ul_lock) );

  /* Add to the list of active netifs */
  ci_dllist_push(&citp_active_netifs, &ni->link);
}

ci_inline void __citp_remove_netif(ci_netif* ni)
{
  /* Requires that the FD table write lock has been taken */

  ci_assert( ni );
  CI_MAGIC_CHECK(ni, NETIF_MAGIC);
  ci_assert( citp_netifs_inited );
  ci_assert( CITP_ISLOCKED(&citp_ul_lock) );
  ci_assert( ci_dllist_not_empty(&citp_active_netifs) );

  /* Remove from the list of active netifs */
  ci_dllist_remove(&ni->link);
}


static int __citp_netif_alloc(ef_driver_handle* fd, const char *name, 
                              ci_netif** out_ni)
{
  int rc;
  ci_netif* ni;
  int realloc = 0;

  ci_assert( CITP_ISLOCKED(&citp_ul_lock) );

  /* Allocate netif from the heap */
  ni = CI_ALLOC_OBJ(ci_netif);
  if( !ni ) {
    Log_E(ci_log("%s: OS failure (memory low!)", __FUNCTION__ ));
    rc = -ENOMEM;
    goto fail1;
  }

  rc = ef_onload_driver_open(fd, 1);
  if( rc < 0 ) {
    Log_E(ci_log("%s: failed to open driver (%d)", __FUNCTION__, -rc));
    goto fail2;
  }

  while( 1 ) {
    if( name[0] != '\0' ) {
      rc = ci_netif_restore_name(ni, name);
      if( rc == 0 ) {
        ef_onload_driver_close(*fd);
        break;
      }
      else if( rc == -EACCES)
        goto fail3;
    }

    rc = ci_netif_ctor(ni, *fd, name, 0);
    if( rc == 0 ) {
      break;
    }
    else if( rc != -EEXIST ) {
      Log_E(ci_log("%s: failed to construct netif (%d)", __FUNCTION__, -rc));
      goto fail3;
    }
    /* Stack with given name exists -- try again to restore. */
  }

  __citp_add_netif(ni);

  /* Call the platform specifc netif ctor hook */
  citp_netif_ctor_hook(ni, realloc);

  *out_ni = ni;
  *fd = ci_netif_get_driver_handle(ni);	/* UNIX may change FD in
					   citp_netif_ctor_hook() */
  return 0;

 fail3:
  ef_onload_driver_close(*fd);
 fail2:
  CI_FREE_OBJ(ni);
 fail1:
  errno = -rc;

  return rc;
}

int citp_netif_by_id(ci_uint32 stack_id, ci_netif** out_ni)
{
  ci_netif *ni;
  int rc;


  ni = citp_find_ul_netif(stack_id, 0);
  if( ni != NULL ) {
    citp_netif_add_ref(ni);
    *out_ni = ni;
    return 0;
  }

  ni = CI_ALLOC_OBJ(ci_netif);
  if( ni == NULL ) {
    return -ENOMEM;
  }

  CITP_LOCK(&citp_ul_lock);
  rc = ci_netif_restore_id(ni, stack_id);
  if( rc < 0 ) {
    CITP_UNLOCK(&citp_ul_lock);
    CI_FREE_OBJ(ni);
    return rc;
  }
  __citp_add_netif(ni);
  ni->flags |= CI_NETIF_FLAGS_SHARED;
  citp_netif_add_ref(ni);
  citp_netif_ctor_hook(ni, 0);

  CITP_UNLOCK(&citp_ul_lock);

  *out_ni = ni;
  return 0;
}

/* ***************************
 * Interface
 */




/* Check the active netifs to look for one with
 * a matching ID
 * \param id          ID to look for (as returned by NI_ID())
 * \param fdt_locked  0 if the fd table lock is NOT held
 *                    or != 0 if the fd table lock IS held.
 * \return     ptr to UL netif or NULL if not found
 */
ci_netif* citp_find_ul_netif( int id, int locked )
{
  ci_netif* ni;

  ci_assert( citp_netifs_inited );

  if( !locked )
    CITP_LOCK_RD(&citp_ul_lock);
    
  CI_DLLIST_FOR_EACH2( ci_netif, ni, link, &citp_active_netifs )
    if( NI_ID(ni) == id )
      goto exit_find;

  ni = NULL;

 exit_find:
  if( !locked )
    CITP_UNLOCK(&citp_ul_lock);
  return ni;
}


void citp_cmn_netif_init_ctor(unsigned netif_dtor_mode)
{
  Log_S(ci_log("%s()", __FUNCTION__));

  if( citp_netifs_inited ) {
    Log_U(ci_log("%s: citp_netifs_inited = %d", 
		 __FUNCTION__, citp_netifs_inited));
  }

  citp_netifs_inited=1;
  
  /* Remember configuration parameters */
  citp_netif_dtor_mode = netif_dtor_mode;

  CITP_LOCK(&citp_ul_lock);

  /* Initialise the active netif list */
  ci_dllist_init(&citp_active_netifs);

  CITP_UNLOCK(&citp_ul_lock);
}


/* Common netif initialiser.  
 * \param IN fd file descriptor
 * \param OUT netif constructed
 * \return 0 - ok else -1 & errno set
 */
int citp_netif_alloc_and_init(ef_driver_handle* fd, ci_netif** out_ni)
{
  ci_netif* ni = NULL;
  int rc;
  char* stackname;

  ci_assert( citp_netifs_inited );
  ci_assert( fd );
  ci_assert( out_ni );

  CITP_LOCK(&citp_ul_lock);

  oo_stackname_get(&stackname);
  if( stackname == NULL ) {
    CITP_UNLOCK(&citp_ul_lock);
    return CI_SOCKET_HANDOVER;
  }

  /* Look through the active netifs for a stack with a name that
   * matches.  If it has no name, ignore it if DONT_USE_ANON netif
   * flag is set
   */
  if( ci_dllist_not_empty(&citp_active_netifs) )
    CI_DLLIST_FOR_EACH2(ci_netif, ni, link, &citp_active_netifs)
      if( strncmp(ni->state->name, stackname, CI_CFG_STACK_NAME_LEN) == 0 )
        if( strlen(ni->state->name) != 0 || 
            (ni->flags & CI_NETIF_FLAGS_DONT_USE_ANON) == 0 )
          break;

  if( ni == NULL ) {
    /* Allocate a new netif */
    rc = __citp_netif_alloc(fd, stackname, &ni);
    if( rc < 0 ) {
      Log_E(ci_log("%s: failed to create netif (%d)", __FUNCTION__, -rc));
      goto fail;
    } 

    /* If we shouldn't destruct private netifs at user-level add an extra
    ** 'destruct protect' reference to prevent it happening.
    */
    if( citp_netif_dtor_mode == CITP_NETIF_DTOR_ONLY_SHARED ) {
      citp_netif_add_ref(ni);
      ni->flags |= CI_NETIF_FLAGS_DTOR_PROTECTED;
    }
    else if( citp_netif_dtor_mode == CITP_NETIF_DTOR_NONE )
      citp_netif_add_ref(ni);

    VERB(ci_log("%s: constructed NI %d", __FUNCTION__, NI_ID(ni)));
  }

  /* We wouldn't be recreating this unless we had an endpoint to attach.
  ** We add the reference for the endpoint here to prevent a race
  ** condition with short-lived endpoints.
  */
  citp_netif_add_ref(ni);
  CITP_UNLOCK(&citp_ul_lock);
  CI_MAGIC_CHECK(ni, NETIF_MAGIC);
  *out_ni = ni;
  return 0;
  
 fail:
  CITP_UNLOCK(&citp_ul_lock);
  errno = -rc;

  return rc;
}


/* Recreate a netif for a 'probed' user-level endpoint, must already hold
** the writer lock to the FD table. caller_fd is the fd of the ep that is
** associated with the netif to be recreated.
*/
int citp_netif_recreate_probed(ci_fd_t ul_sock_fd, 
                               ef_driver_handle* ni_fd_out,
                               ci_netif** ni_out)
{
  int rc;
  ci_netif* ni;
  ci_uint32 map_size;

  ci_assert( citp_netifs_inited );
  ci_assert( ni_fd_out );
  ci_assert( ni_out );
  ci_assert( CITP_ISLOCKED(&citp_ul_lock) );

  /* Allocate netif from the heap */
  ni = CI_ALLOC_OBJ(ci_netif);
  if( !ni ) {		
    Log_E(ci_log("%s: OS failure (memory low!)", __FUNCTION__ ));
    rc = -ENOMEM;
    goto fail1;
  }

  CI_ZERO(ni);  /* bc: need to zero-out the UL netif */
  
  /* Create a new file descriptor that maps to the netif. */
  rc = ci_tcp_helper_stack_attach(ul_sock_fd, &ni->nic_set, &map_size);
  if( rc < 0 ) {
    Log_E(ci_log("%s: FAILED: ci_tcp_helper_stack_attach %d", __FUNCTION__, rc));
    goto fail2;
  }

  /* Restore the netif mmaps and user-level state */
  ci_netif_restore(ni, (ci_fd_t)rc, map_size);

  CI_MAGIC_CHECK(ni, NETIF_MAGIC);

  /* Remember the netif */
  __citp_add_netif( ni );

  /* Setup flags, restored netifs are definitely shared with another
  ** process.  If they weren't shared they wouldn't exist to be restored.
  */
  ni->flags |= CI_NETIF_FLAGS_SHARED;
  
  /* If we shouldn't destruct netifs at user-level add an extra 'destruct
  ** protect' reference to prevent it ever happening.
  **
  ** Since we're never deleting any netifs in this case we avoid setting
  ** the CI_NETIF_FLAGS_DTOR_PROTECTED flag.
  */
  if( citp_netif_dtor_mode == CITP_NETIF_DTOR_NONE )
    citp_netif_add_ref(ni);

  /* We wouldn't be recreating this unless we had an endpoint to attach.
  ** We add the reference for the endpoint here to prevent a race
  ** condition.
  */
  citp_netif_add_ref(ni);

  /* Call the platform specifc netif ctor hook */
  citp_netif_ctor_hook(ni, 0);

  *ni_out = ni;
  *ni_fd_out = ci_netif_get_driver_handle(ni); /* UNIX may change FD in
						  citp_netif_ctor_hook() */
  return 0;

 fail2:
  CI_FREE_OBJ(ni);
 fail1:
  return rc;
}


/* Returns any active netif (used when all you need is a netif to
** do a resource operation or similar)
*/
ci_netif* __citp_get_any_netif(void)
{
  ci_netif* ni = 0;
  
  ci_assert( CITP_ISLOCKED(&citp_ul_lock) );
  
  if( ci_dllist_not_empty(&citp_active_netifs) )
    ni = CI_CONTAINER(ci_netif, link, ci_dllist_start(&citp_active_netifs));

  return ni;
}

int citp_get_active_netifs(ci_netif **array, int array_size)
{
  ci_netif* ni = 0;
  int n = 0;
  ci_assert( CITP_ISLOCKED_RD(&citp_ul_lock) );
  
  if( ci_dllist_not_empty(&citp_active_netifs) ) {
    CI_DLLIST_FOR_EACH2(ci_netif, ni, link, &citp_active_netifs) {
      if (n >= array_size) {
        ci_log("***%s: cannot return all active netifs! Array too small.",
               __FUNCTION__);
        break;
      }
      citp_netif_add_ref(ni);
      array[n] = ni;
      n++;
    }
  }

  return n;
}

int citp_netif_exists(void)
{
    ci_assert( citp_netifs_inited );

    return ci_dllist_not_empty(&citp_active_netifs);
}


/* Mark all active netifs as shared */
void __citp_netif_mark_all_shared(void)
{
  ci_netif* ni;
  ci_netif* next_ni;

  ci_assert( CITP_ISLOCKED(&citp_ul_lock) );

  /* Remove any 'destruct protect' references, if we are in
  ** CITP_NETIF_DTOR_NONE mode the CI_NETIF_FLAGS_DTOR_PROTECTED
  ** flag isn't set if the first place, so this will still work as
  ** intended.
  */
  CI_DLLIST_FOR_EACH3(ci_netif, ni, link, &citp_active_netifs, next_ni) {
    if( ni->flags & CI_NETIF_FLAGS_DTOR_PROTECTED ) {
      ni->flags &= ~CI_NETIF_FLAGS_DTOR_PROTECTED;
      citp_netif_release_ref(ni, 1);
    }
  }

  /* Set shared flag on any netifs that haven't destructed above */
  CI_DLLIST_FOR_EACH2(ci_netif, ni, link, &citp_active_netifs)
    ni->flags |= CI_NETIF_FLAGS_SHARED;
}


/* Mark all netifs as "don't use for new sockets unless compelled to
 * do so by the stack name configuration 
 */
void __citp_netif_mark_all_dont_use(void)
{
  ci_netif* ni;

  ci_assert( CITP_ISLOCKED(&citp_ul_lock) );

  CI_DLLIST_FOR_EACH2(ci_netif, ni, link, &citp_active_netifs)
    ni->flags |= CI_NETIF_FLAGS_DONT_USE_ANON;
}


/* Unprotect all netifs, allowing them to disappear when they have no sockets.
 */
void __citp_netif_unprotect_all(void)
{
  ci_netif* ni;
  ci_netif* next_ni;

  ci_assert( CITP_ISLOCKED(&citp_ul_lock) );

  CI_DLLIST_FOR_EACH3(ci_netif, ni, link, &citp_active_netifs, next_ni) {
    if( ni->flags & CI_NETIF_FLAGS_DTOR_PROTECTED ) {
      ni->flags &= ~CI_NETIF_FLAGS_DTOR_PROTECTED;
      citp_netif_release_ref(ni, 1);
    }
    if( citp_netif_dtor_mode == CITP_NETIF_DTOR_NONE )
      citp_netif_release_ref(ni, 1);
  }
}


void __citp_netif_ref_count_zero( ci_netif* ni, int locked )
{
  ci_assert( ni );
  CI_MAGIC_CHECK(ni, NETIF_MAGIC);
  ci_assert( oo_atomic_read(&ni->ref_count) == 0 );

  Log_V(ci_log("%s: Last ref removed from NI %d (fd:%d ni:%p driver)",
               __FUNCTION__, NI_ID(ni), ci_netif_get_driver_handle(ni), ni));

  if( !locked )
    CITP_LOCK(&citp_ul_lock);

  ci_assert( CITP_ISLOCKED(&citp_ul_lock) );

  /* Forget about the netif */
  __citp_remove_netif(ni);

  __citp_netif_free(ni);

  if( !locked )
    CITP_UNLOCK(&citp_ul_lock);
}


/* Free and destruct a netif
**
** Requires that the FD table write lock has been taken
*/
void __citp_netif_free(ci_netif* ni)
{
  ef_driver_handle fd;
  
  ci_assert( ni );
  CI_MAGIC_CHECK(ni, NETIF_MAGIC);
  ci_assert( oo_atomic_read(&ni->ref_count) == 0 );
  ci_assert( CITP_ISLOCKED(&citp_ul_lock) );

  Log_V(ci_log("%s: Freeing NI %d (fd:%d ni:%p)", __FUNCTION__,
               NI_ID(ni), ci_netif_get_driver_handle(ni), ni));

  /* Call the platform specifc netif free hook */
  citp_netif_free_hook(ni);

  /* Destruct the netif...
  **
  ** This will zap ci_netif_get_driver_handle(ni) so we remember it
  ** first, it also needs ci_netif_get_driver_handle(ni) to be valid
  ** and still open so we can't close it first
  */
  fd = ci_netif_get_driver_handle(ni);
  ci_netif_dtor(ni);

  /* Close the netif's FD */
  ef_onload_driver_close(fd);

  /* ...and free it */
  CI_FREE_OBJ(ni);
}


/*! \todo Wouldn't export from the lib. when in a separate module
 *   so moved it here for now - from whence it export's nicely! 
 *   Yep, not nice, but a pragmatic answer to something that was 
 *   taking too long to resolve. 
 */

/* Rehome (by duplicating) the existing EP from [ep] to another netif.
 * The target netif is [new_ni]: a valid ptr to netif to which to rehome [ep].  
 * [ep->netif] must be locked on entry, and if valid, [new_ni] must be unlocked.
 * Returns 0 if failed to rehome (in which case [*ep] is unchanged)
 *         1 if rehomed (in which case [*ep] has been altered & [ep->netif] is
 *           locked)
 */
int citp_rehome_closed_endpoint(citp_socket* ep, ci_fd_t fd, ci_netif* new_ni )
{
  ef_driver_handle new_fd = (ef_driver_handle)(-1); /* for new netif */
  ci_netif* old_ni;
  ci_tcp_state *old_ts, *new_ts;

  ci_assert_equal(ep->s->b.state, CI_TCP_CLOSED);
  ci_assert( ci_netif_is_locked(ep->netif) );

  /* The sequence here is:
   * 1. get a new netif (if not provided)
   *  Failure: do nothing more, just use the existing netif
   *  success: the netif has just been constructed. It's not locked.
   * 2. Lock new netif, get a TCP state
   * 3. Duplicate fields of current (CLOSED) TCP state -> new state
   * 4. Substitute new netif, tcp-state into existing ep
   * 5. Free-up old TCP state
   * 6. Unlock old netif
   */
  old_ni = ep->netif;
  old_ts = SOCK_TO_TCP(ep->s);

  ci_assert(new_ni);

  if( old_ni == new_ni ) {
    ci_log("%s: old NI = new NI (%p)!", __FUNCTION__, old_ni);
    return 1;
  }
  /* got "new" netif, lock it */
  ci_netif_lock( new_ni );
  /* We have to take the EP ref here  */
  citp_netif_add_ref(new_ni);

  /* Get en EP from [new_ni] and copy the state over */
  if( CI_UNLIKELY( ci_tcp_move_state(ep->netif, old_ts, fd, new_ni, &new_ts))) {
    ci_netif_unlock( new_ni );
    if(new_fd != (ef_driver_handle)(-1))
      ef_onload_driver_close( new_fd );
    else
      citp_netif_release_ref(new_ni, 0/*fd/context table not locked*/);
    Log_U(ci_log("%s: Failed to move state to new EP", __FUNCTION__));
    return 0;
  }
  
  /* 4. Substitute in the new netif & EP */
  ep->netif = new_ni;
  ep->s = SP_TO_SOCK_CMN( new_ni, S_SP(new_ts) );

  /* 5. Free the old EP (it is CLOSED) */
  ci_tcp_state_free(old_ni, old_ts);
  /* 6. Unlock the old netif */
  ci_netif_unlock(old_ni);
  CHECK_TEP(ep);
  ci_assert( ci_netif_is_locked(ep->netif) );
  return 1;
}

/*! \cidoxg_end */
