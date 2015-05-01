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
** \author  kjm
**  \brief  onload_set_stackname() extension.
**   \date  2010/12/11
**    \cop  (c) Solarflare Communications Inc.
** </L5_PRIVATE>
*//*
\**************************************************************************/

#include "internal.h"


int onload_stack_opt_get_int(const char* opt_env, int64_t* opt_val)
{
  struct oo_per_thread* pt;
  ci_netif_config_opts* opts;
  
  pt = oo_per_thread_get();
  opts = pt->thread_local_netif_opts;

  if( opts == NULL) {
    opts = &ci_cfg_opts.netif_opts;
  }

  #undef CI_CFG_OPT
  #define CI_CFG_OPT(env, name, p3, p4, p5, p6, p7, p8, p9, p10)    \
    {                                                               \
      if( ! strcmp(env, opt_env) ) {                                \
        *opt_val = opts->name;                                      \
        return 0;                                                   \
      }                                                             \
    }

  #include <ci/internal/opts_netif_def.h>
  LOG_E(ci_log("%s: Requested option %s not found", __FUNCTION__, opt_env));
  return -EINVAL;
}


/* This API provides per thread ability to modify ci_netif_config_opts
 * for future stacks.  If not present, this makes a thread local copy
 * of the default ci_netif_config_opts and updates it as requested.
 * Any future stacks will use the thread local copy of config_opts and
 * if absent, use the default copy.  */
int onload_stack_opt_set_int(const char* opt_env, int64_t opt_val)
{
  struct oo_per_thread* pt;
  ci_netif_config_opts* default_opts;

  pt = oo_per_thread_get();

  if( pt->thread_local_netif_opts == NULL ) {
    pt->thread_local_netif_opts = malloc(sizeof(*pt->thread_local_netif_opts));
    if( ! pt->thread_local_netif_opts)
      return -ENOMEM;
    default_opts = &ci_cfg_opts.netif_opts;
    memcpy(pt->thread_local_netif_opts, default_opts, sizeof(*default_opts));
  }

  #define ci_uint32_fmt   "%u"
  #define ci_uint16_fmt   "%u"
  #define ci_int32_fmt    "%d"
  #define ci_int16_fmt    "%d"
  #define ci_iptime_t_fmt "%u"

  #define _CI_CFG_BITVAL   _optbits
  #define _CI_CFG_BITVAL1  1
  #define _CI_CFG_BITVAL2  2
  #define _CI_CFG_BITVAL3  3
  #define _CI_CFG_BITVAL4  4
  #define _CI_CFG_BITVAL8  8
  #define _CI_CFG_BITVAL16 16
  #define _CI_CFG_BITVALA8 _CI_CFG_BITVAL

  #undef CI_CFG_OPTFILE_VERSION
  #undef CI_CFG_OPT
  #undef CI_CFG_OPTGROUP
  #undef MIN
  #undef MAX
  #undef SMIN
  #undef SMAX

  #define CI_CFG_OPTFILE_VERSION(version)
  #define CI_CFG_OPTGROUP(group, category, expertise)
  #define MIN 0
  #define MAX (((1ull<<(_bitwidth-1))<<1) - 1ull)
  #define SMAX (MAX >> 1)
  #define SMIN (-SMAX-1)
  
  #define CI_CFG_OPT(env, name, type, doc, bits, group, default, min, max, presentation) \
    {                                                                   \
      type _max;                                                        \
      type _min;                                                        \
      int _optbits = sizeof(type) * 8;                                  \
      int _bitwidth = _CI_CFG_BITVAL##bits;                             \
      (void)_bitwidth;                                                  \
      (void)_optbits;                                                   \
      _max = (type)(max);                                               \
      _min = (type)(min);                                               \
      if( ! strcmp(env, opt_env) ) {                                    \
        if( opt_val < _min || opt_val > _max ) {                        \
          LOG_E(ci_log("%s: %"PRId64" outside of range ("type##_fmt":"  \
                       type##_fmt") for %s",                            \
                       __FUNCTION__, opt_val, _min, _max, opt_env));    \
          return -EINVAL;                                               \
        }                                                               \
        pt->thread_local_netif_opts->name = opt_val;                    \
        return 0;                                                       \
      }                                                                 \
    }

  #include <ci/internal/opts_netif_def.h>
  LOG_E(ci_log("%s: Requested option %s not found", __FUNCTION__, opt_env));
  return -EINVAL;
}


/* Simply delete the thread local copy of config_opts to revert to
 * using the netif's config_opts. */
int onload_stack_opt_reset(void)
{
  struct oo_per_thread* pt;

  pt = oo_per_thread_get();
  if( pt->thread_local_netif_opts != NULL ) {
    free(pt->thread_local_netif_opts);
    pt->thread_local_netif_opts = NULL;
  }
  return 0;
}
