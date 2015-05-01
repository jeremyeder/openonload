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
** \author  ok_sasha
**  \brief  eplock API exported for its user
**     $Id$
**   \date  2007/08
**    \cop  (c) Solaraflare Communications
** </L5_PRIVATE>
*//*
\**************************************************************************/

/*! \cidoxg_include_onload  */

/* This file is a part of ci/internal/ip.h or, more specially, 
 * of ci/internal/ip_shared_ops.h.
 * ***** Do not include it directly! Include ip.h instead! *****
 */

#ifndef __ONLOAD_EPLOCK_H__
#define __ONLOAD_EPLOCK_H__


#define OO_MUST_CHECK_RET  __attribute__((warn_unused_result))
#ifdef __KERNEL__
# define OO_MUST_CHECK_RET_IN_KERNEL  OO_MUST_CHECK_RET
#else
# define OO_MUST_CHECK_RET_IN_KERNEL
#endif


/* Internal!  Do not call. */
extern int __ef_eplock_lock_slow(ci_netif *) CI_HF;


#if defined(CI_HAVE_COMPARE_AND_SWAP)

  /*! Attempt to lock an eplock.  Returns true on success. */
ci_inline int ef_eplock_trylock(ci_eplock_t* l) {
  unsigned v = l->lock;
  return (v & CI_EPLOCK_UNLOCKED) &&
    ci_cas32_succeed(&l->lock,
		    v, (v &~ CI_EPLOCK_UNLOCKED) | CI_EPLOCK_LOCKED);
}

  /* Always returns 0 (success) at userland.  Returns -EINTR if interrupted
   * when invoked in kernel.  So return value *must* be checked when
   * invoked in kernel, else risk of proceeding without the lock held.
   */
ci_inline int ef_eplock_lock(ci_netif *ni) OO_MUST_CHECK_RET_IN_KERNEL;
ci_inline int ef_eplock_lock(ci_netif *ni) {
  int rc = 0;
  if( ci_cas32_fail(&ni->state->lock.lock,
                    CI_EPLOCK_UNLOCKED, CI_EPLOCK_LOCKED) )
    rc = __ef_eplock_lock_slow(ni);
#ifdef __KERNEL__
  return rc;
#else
  /* Ensure the compiler knows we're returning zero, so it can optimise out
   * any code conditional on the return value.
   */
  (void) rc;
  return 0;
#endif
}

  /*! Only call this if you hold the lock.  [flag] must have exactly one
  ** bit set.
  */
ci_inline void ef_eplock_holder_set_flag(ci_eplock_t* l, int flag) {
  unsigned v;
  ci_assert((flag & 0xf0000000) == 0u);
  do {
    v = l->lock;
    ci_assert(v & CI_EPLOCK_LOCKED);
    if( v & flag )  break;
  } while( ci_cas32_fail(&l->lock, v, v | flag) );
}

  /*! Only call this if you hold the lock. */
ci_inline void ef_eplock_holder_set_flags(ci_eplock_t* l, unsigned flags) {
  unsigned v;
  ci_assert((flags & 0xf0000000) == 0u);
  do {
    v = l->lock;
    ci_assert(v & CI_EPLOCK_LOCKED);
    if( (v & flags) == flags )
      break;
  } while( ci_cas32_fail(&l->lock, v, v | flags) );
}

  /*! Clear the specified lock flags. */
ci_inline void ef_eplock_clear_flags(ci_eplock_t* l, int flags) {
  unsigned v;
  ci_assert((flags & 0xf0000000) == 0u);
  do {
    v = l->lock;
  } while( ci_cas32_fail(&l->lock, v, v &~ flags) );
}

  /*! Used to alert the lock holder.  Caller would normally not hold the
  ** lock.  Returns 1 on success, or 0 if the lock is unlocked.  [flag]
  ** must have exactly one bit set.
  */
ci_inline int ef_eplock_set_flag_if_locked(ci_eplock_t* l, unsigned flag) {
  unsigned v;
  ci_assert((flag & 0xf0000000) == 0u);
  do {
    v = l->lock;
    if( v & CI_EPLOCK_UNLOCKED )  return 0;
    if( v & flag )  break;
  } while( ci_cas32_fail(&l->lock, v, v | flag) );
  return 1;
}

  /*! Used to alert the lock holder.  Caller would normally not hold the
  ** lock.  Returns 1 on success, or 0 if the lock is unlocked.  [flag]
  ** may have one or more bits set.
  */
ci_inline int ef_eplock_set_flags_if_locked(ci_eplock_t* l, unsigned flags) {
  unsigned v;
  ci_assert((flags & 0xf0000000) == 0u);
  do {
    v = l->lock;
    if( v & CI_EPLOCK_UNLOCKED )  return 0;
    if( (v & flags) == flags )  break;
  } while( ci_cas32_fail(&l->lock, v, v | flags) );
  return 1;
}

  /*! Attempt to grab the lock -- return non-zero on success.  Set [flags]
  ** whether or not the lock is obtained.
  */
ci_inline int ef_eplock_trylock_and_set_flags(ci_eplock_t* l, unsigned flags) {
  unsigned v, new_v;
  do {
    v = l->lock;
    new_v = (v &~ CI_EPLOCK_UNLOCKED) | CI_EPLOCK_LOCKED | flags;
  } while( ci_cas32_fail(&l->lock, v, new_v) );
  return v & CI_EPLOCK_UNLOCKED;
}

  /*! Either obtains the lock (returning 1) or sets the flag (returning 0).
  ** [flag] must have exactly one bit set.
  */
ci_inline int ef_eplock_lock_or_set_flag(ci_eplock_t* l, unsigned flag) {
  unsigned v, new_v;
  int rc;
  ci_assert((flag  & 0xf0000000) == 0u);
  do {
    if( (v = l->lock) & CI_EPLOCK_UNLOCKED ) {
      rc = 1;
      new_v = (v &~ CI_EPLOCK_UNLOCKED) | CI_EPLOCK_LOCKED;
    }
    else if( v & flag )
      return 0;
    else {
      rc = 0;
      new_v = v | flag;
    }
  } while( ci_cas32_fail(&l->lock, v, new_v) );
  return rc;
}

  /*! Either obtains the lock (returning 1) or sets the flags (returning
   * 0).
   */
ci_inline int ef_eplock_lock_or_set_flags(ci_eplock_t* l, unsigned flags) {
  unsigned v, new_v;
  int rc;
  ci_assert((flags  & 0xf0000000) == 0u);
  do {
    if( (v = l->lock) & CI_EPLOCK_UNLOCKED ) {
      rc = 1;
      new_v = (v &~ CI_EPLOCK_UNLOCKED) | CI_EPLOCK_LOCKED;
    }
    else if( (v & flags) == flags )
      return 0;
    else {
      rc = 0;
      new_v = v | flags;
    }
  } while( ci_cas32_fail(&l->lock, v, new_v) );
  return rc;
}

  /*! Attempt to unlock the lock.  This will fail if any flags are set.
  ** This should only be needed inside an unlock callback.
  */
ci_inline int ef_eplock_try_unlock(ci_eplock_t* l, 
				   unsigned* lock_val_out,
				   unsigned  flag_mask) {
  unsigned lv = *lock_val_out = l->lock;
  unsigned unlock = (lv &~ (CI_EPLOCK_LOCKED | CI_EPLOCK_FL_NEED_WAKE)) |
                    CI_EPLOCK_UNLOCKED;
  return (lv & flag_mask) ? 0 :
    ci_cas32_succeed(&l->lock, lv, unlock);
}

  /*! Return the flags which are currently set (including need-wakeup).
  ** NB. This is not atomic.
  */
ci_inline unsigned ef_eplock_flags(ci_eplock_t* l)
{ return l->lock & (CI_EPLOCK_FL_NEED_WAKE | 0x0fffffff); }

#endif

  /*! Return true if the lock is locked.  NB. This does not guarantee that
  ** the current thread is the holder!  So this is only useful for debug
  ** checks.
  */
ci_inline int ef_eplock_is_locked(ci_eplock_t* l)
{ return (l->lock & CI_EPLOCK_LOCKED) != 0; }


#endif /* __ONLOAD_EPLOCK_H__ */
