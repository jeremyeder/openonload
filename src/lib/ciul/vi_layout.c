/*
** Copyright 2005-2015  Solarflare Communications Inc.
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

#include "ef_vi_internal.h"


#define FRAME_DESCRIPTION			\
  "Ethernet frame"

#define TICKS_DESCRIPTION			\
  "Hardware timestamp (minor ticks)"


static const ef_vi_layout_entry layout_no_prefix = {
  .evle_type = EF_VI_LAYOUT_FRAME,
  .evle_offset = 0,
  .evle_description = FRAME_DESCRIPTION,
};


static int
falcon_query_layout(ef_vi* vi, const ef_vi_layout_entry**const ef_vi_layout_out,
                    int* len_out)
{
  static const ef_vi_layout_entry layout_prefix = {
    .evle_type = EF_VI_LAYOUT_FRAME,
    .evle_offset = FS_BZ_RX_PREFIX_SIZE,
    .evle_description = FRAME_DESCRIPTION,
  };

  if( vi->rx_prefix_len )
    *ef_vi_layout_out = &layout_prefix;
  else
    *ef_vi_layout_out = &layout_no_prefix;
  *len_out = 1;
  return 0;
}


static int
ef10_query_layout(ef_vi* vi, const ef_vi_layout_entry**const ef_vi_layout_out,
                  int* len_out)
{
  static const ef_vi_layout_entry layout_prefix[] = {
    {
      .evle_type = EF_VI_LAYOUT_FRAME,
      .evle_offset = ES_DZ_RX_PREFIX_SIZE,
      .evle_description = FRAME_DESCRIPTION,
    },
    {
      .evle_type = EF_VI_LAYOUT_MINOR_TICKS,
      .evle_offset = ES_DZ_RX_PREFIX_TSTAMP_OFST,
      .evle_description = TICKS_DESCRIPTION,
    },
  };

  if( vi->rx_prefix_len ) {
    *ef_vi_layout_out = layout_prefix;
    if( (vi->vi_flags & EF_VI_RX_TIMESTAMPS) != 0 )
      *len_out = 2;
    else
      *len_out = 1;
  }
  else {
    *ef_vi_layout_out = &layout_no_prefix;
    *len_out = 1;
  }
  return 0;
}


int ef_vi_receive_query_layout(ef_vi* vi,
                               const ef_vi_layout_entry**const ef_vi_layout_out,
                               int* len_out)
{
  switch( vi->nic_type.arch ) {
  case EF_VI_ARCH_FALCON:
    return falcon_query_layout(vi, ef_vi_layout_out, len_out);
  case EF_VI_ARCH_EF10:
    return ef10_query_layout(vi, ef_vi_layout_out, len_out);
  default:
    EF_VI_BUG_ON(1);
    return -EINVAL;
  }
}
