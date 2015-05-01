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


#ifndef __CI_EFHW_HARDWARE_UL_H__
#define __CI_EFHW_HARDWARE_UL_H__

#include <ci/compat.h>
#include <ci/tools/byteorder.h>
#include <ci/tools/sysdep.h>

ci_inline void
__raw_writeb(ci_uint8 data, volatile char *addr)
{
	*(volatile ci_uint8*) addr = data;
}
ci_inline void
writeb(ci_uint8 data, volatile char *addr)
{
	return __raw_writeb(data, addr);
}


ci_inline ci_uint8 __raw_readb(volatile char *addr)
{
	return *(volatile ci_uint8*) addr;
}
ci_inline ci_uint8 readb(volatile char *addr)
{
	return __raw_readb(addr);
}


ci_inline void
__raw_writew(ci_uint16 data, volatile char *addr)
{
	*(volatile ci_uint16*) addr = data;
}
ci_inline void
writew(ci_uint16 data, volatile char *addr)
{
	return __raw_writew(CI_BSWAP_LE16(data), (addr));
}


ci_inline ci_uint16 __raw_readw(volatile char *addr)
{
	return *(volatile ci_uint16*) addr;
}
ci_inline ci_uint16 readw(volatile char *addr)
{
	ci_uint16 x = __raw_readw(addr);
	return CI_BSWAP_LE16(x);
}


ci_inline void
__raw_writel(ci_uint32 data, volatile char *addr)
{
	*(volatile ci_uint32*) addr = data;
}
ci_inline void
writel(ci_uint32 data, volatile char *addr)
{
	__raw_writel(CI_BSWAP_LE32(data), (addr));
}


ci_inline ci_uint32 __raw_readl(volatile char *addr)
{
	return *(volatile ci_uint32*) addr;
}
ci_inline ci_uint32 readl(volatile char *addr)
{
	ci_uint32 x = __raw_readl(addr);
	return CI_BSWAP_LE32(x);
}


ci_inline void
__raw_writeq(ci_uint64 data, volatile char *addr)
{
	*(volatile ci_uint64*) addr = data;
}
ci_inline void
writeq(ci_uint64 data, volatile char *addr)
{
	__raw_writeq(CI_BSWAP_LE64(data), (addr));
}


ci_inline ci_uint64 __raw_readq(volatile char *addr)
{
	return *(volatile ci_uint64*) addr;
}
ci_inline ci_uint64 readq(volatile char *addr)
{
	ci_uint64 x = __raw_readq(addr);
	return CI_BSWAP_LE64(x);
}


#endif /* __CI_EFHW_HARDWARE_UL_H__ */
