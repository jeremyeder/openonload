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
** <L5_PRIVATE L5_SOURCE>
** \author  
**  \brief  
**   \date  
**    \cop  (c) Level 5 Networks Limited.
** </L5_PRIVATE>
*//*
\**************************************************************************/

/*! \cidoxg_include_ci_internal */

#ifndef __CI_INTERNAL_SYSCALL3_H__
#define __CI_INTERNAL_SYSCALL3_H__


/* came from src/lib/transport/unix/log_fn.c */

/*
** We can't just use the default log function, as it uses the standard I/O
** mechanisms, which we intercept, leading to recursive nastiness.
**
** Hence we jump straight into a syscall.
**
** An alternative would be to use ci_sys_writev() or something, but that
** wouldn't be available as early in the library initialisation.
*/


#include <sys/uio.h>
#include <linux/unistd.h>

#if defined(__x86_64__)
ci_inline int
my_do_syscall3(int num, long a1, long a2, long a3)
{
  int rc;
  /* ?? TODO do we need to worry about PIC stuff here */

  __asm__ __volatile__(

   "syscall"
   : "=a" (rc)
   : "0"((long)num), "D"(a1), "S"(a2), "d"(a3) 
   : "r11","rcx","memory"

  );

  return rc;
}

#elif defined(__i386__)

ci_inline int
my_do_syscall3(int num, long a1, long a2, long a3)
{
  int rc;
  /* need to protect ebx because it is the PIC (position independent
  ** code) GOT (global offset table) used as a shared library
  */
  __asm__ __volatile__("pushl %%ebx; movl %%edi,%%ebx; int $0x80; popl %%ebx"
		       : "=a" (rc)
		       : "0"(num), "D"(a1), "c"(a2), "d"(a3)
                       : "memory");
  return rc;
}

#elif defined(__PPC__)

ci_inline int
my_do_syscall3(int num, long a1, long a2, long a3)
{
   unsigned long __sc_ret;    /* for syscall exit return */
   unsigned long __sc_err;    /* from syscall error return */

   register unsigned long __sc_0  __asm__ ("r0");  // Holds syscall function code
   register unsigned long __sc_3  __asm__ ("r3");  // Holds arg1
   register unsigned long __sc_4  __asm__ ("r4");  // Holds arg2
   register unsigned long __sc_5  __asm__ ("r5");  // Holds arg3

   /*  Load up the parameters into register variables ... */
   __sc_0 = num;
   __sc_3 = (unsigned long) (a1);
   __sc_4 = (unsigned long) (a2);
   __sc_5 = (unsigned long) (a3);


   /* And now we'll simply invoke a 'syscall' event ... */
   __asm__ __volatile__ (

      "sc           \n"
      "mfcr %0      \n"
      
      : "=&r" (__sc_0),"=&r" (__sc_3),"=&r" (__sc_4),"=&r" (__sc_5)
      : "0" (__sc_0), "1" (__sc_3), "2" (__sc_4), "3" (__sc_5)
      : "cr0", "ctr", "memory"

   );

   /* Collect the returns from 'syscall' */
   __sc_ret = __sc_3;
   __sc_err = __sc_0;

   /* Test and see if we had success with this syscall or not */   
   if (__sc_err & 0x10000000)
   {
      __sc_err = __sc_ret;    // Here syscall has given us the global 'errno' actually
      __sc_ret = -1;          // Just indicate that we failed!!
   }
   
   return (int) __sc_ret;
}

#elif defined(__ia64__)

ci_inline int my_do_syscall3(int num, __attribute__((unused)) long a1, 
			     __attribute__((unused)) long a2, 
			     __attribute__((unused)) long a3)
{
/* SPH: This is wrong */
  return syscall(num, a1, a2, a3);
}

#else

# error my_do_syscall3() unimplemented

#endif

#define	my_syscall3(call, a1, a2, a3) \
        my_do_syscall3(__NR_##call, (a1), (a2), (a3))



#endif /* __CI_INTERNAL_SYSCALL3_H__ */
