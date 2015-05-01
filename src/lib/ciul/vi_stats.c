/*
** Copyright 2005-2014  Solarflare Communications Inc.
**                      7505 Irvine Center Drive, Irvine, CA 92618, USA
** Copyright 2002-2005  Level 5 Networks Inc.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of version 2.1 of the GNU Lesser General Public
** License as published by the Free Software Foundation.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
*/

/**************************************************************************\
*//*! \file
** <L5_PRIVATE L5_SOURCE>
** \author  Akhi Singhania <asinghania@solarflare.com>
**  \brief  Layout of RX data.
**   \date  2013/11
**    \cop  (c) Solarflare Communications, Inc.
** </L5_PRIVATE>
*//*
\**************************************************************************/

#include <etherfabric/vi.h>
#include "ef_vi_internal.h"
#include "driver_access.h"
#include "logging.h"

int
falcon_query_layout(ef_vi* vi,
                    const ef_vi_stats_layout**const layout_out)
{
  return -EINVAL;
}

int
ef10_query_layout(ef_vi* vi, const ef_vi_stats_layout**const layout_out)
{
  static const ef_vi_stats_layout layout = {
#define EF10_RX_STATS_SIZE 16
    .evsl_data_size = EF10_RX_STATS_SIZE,
    .evsl_fields_num = 4,
    .evsl_fields = {
      {
        .evsfl_name = "RX CRC errors",
        .evsfl_offset = 0,
        .evsfl_size = 4,
      },
      {
        .evsfl_name = "RX trunk errors",
        .evsfl_offset = 4,
        .evsfl_size = 4,
      },
      {
        .evsfl_name = "RX no descriptor errors",
        .evsfl_offset = 8,
        .evsfl_size = 4,
      },
      {
        .evsfl_name = "RX abort errors",
        .evsfl_offset = 12,
        .evsfl_size = 4,
      },
    }
  };
  *layout_out = &layout;
  return 0;
}

int
ef_vi_stats_query_layout(ef_vi* vi,
                         const ef_vi_stats_layout**const layout_out)
{
  switch( vi->nic_type.arch ) {
  case EF_VI_ARCH_FALCON:
    return falcon_query_layout(vi, layout_out);
  case EF_VI_ARCH_EF10:
    return ef10_query_layout(vi, layout_out);
  default:
	  EF_VI_BUG_ON(1);
    return -EINVAL;
  }
}

int
falcon_query(ef_vi* vi, ef_driver_handle dhL, void* data, int do_reset)
{
  return -EINVAL;
}

int
ef10_query(ef_vi* vi, ef_driver_handle dh, void* data, int do_reset)
{
  ci_resource_op_t  op;

  op.op = CI_RSOP_VI_GET_RX_ERROR_STATS;
  op.id = efch_make_resource_id(vi->vi_resource_id);
  op.u.vi_stats.data_ptr = (uintptr_t)data;
  op.u.vi_stats.data_len = EF10_RX_STATS_SIZE;
  op.u.vi_stats.do_reset = do_reset;
  return ci_resource_op(dh, &op);
}

int
ef_vi_stats_query(ef_vi* vi, ef_driver_handle dh, void* data, int do_reset)
{
  switch( vi->nic_type.arch ) {
  case EF_VI_ARCH_FALCON:
    return falcon_query(vi, dh, data, do_reset);
  case EF_VI_ARCH_EF10:
    return ef10_query(vi, dh, data, do_reset);
  default:
	  EF_VI_BUG_ON(1);
    return -EINVAL;
  }
}

