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


#include <ci/internal/ip.h>

static int syncookie_mss[8] =
{
  64, /* minimal mss */
  512, /* CSLIP */
  536, /* default */
  1024,
  1220, /* ipv6-related */
  1440,
  1460,
  2048 /* our maximum; replaced by ni->state->max_mss */
};



/* Siphash-2-4 implementation */

struct siphash {
  ci_uint64 v0, v1, v2, v3;

  unsigned char buf[8];
  unsigned char* p;
  ci_uint64 c;
};

#define SIP_ROTL(x, b) (ci_uint64)(((x) << (b)) | ( (x) >> (64 - (b))))

static void
sip_round(struct siphash* h, int rounds)
{
  int i;

  for( i = 0; i < rounds; i++ ) {
    h->v0 += h->v1;
    h->v1 = SIP_ROTL(h->v1, 13);
    h->v1 ^= h->v0;
    h->v0 = SIP_ROTL(h->v0, 32);

    h->v2 += h->v3;
    h->v3 = SIP_ROTL(h->v3, 16);
    h->v3 ^= h->v2;

    h->v0 += h->v3;
    h->v3 = SIP_ROTL(h->v3, 21);
    h->v3 ^= h->v0;

    h->v2 += h->v1;
    h->v1 = SIP_ROTL(h->v1, 17);
    h->v1 ^= h->v2;
    h->v2 = SIP_ROTL(h->v2, 32);
  }
}

static ci_uint64
sip_hash(ci_uint64* key, const void* data, int len)
{
  struct siphash h;
  const unsigned char *p = data, *pe = p + len;
  ci_uint64 m;
  char left;
  ci_uint64 b;

  /* init */
  memset(&h, 0, sizeof(h));
  h.v0 = 0x736f6d6570736575ULL ^ key[0];
  h.v1 = 0x646f72616e646f6dULL ^ key[1];
  h.v2 = 0x6c7967656e657261ULL ^ key[0];
  h.v3 = 0x7465646279746573ULL ^ key[1];
  h.p = h.buf;

  /* update */
  do {
    while( p < pe && h.p - h.buf < sizeof(h.buf) )
      *h.p++ = *p++;

    if( h.p - h.buf < sizeof(h.buf) )
      break;

    m = ((ci_uint64)h.buf[0] << 0) | ((ci_uint64)h.buf[1] << 8) |
        ((ci_uint64)h.buf[2] << 16) | ((ci_uint64)h.buf[3] << 24) |
        ((ci_uint64)h.buf[2] << 32) | ((ci_uint64)h.buf[3] << 40) |
        ((ci_uint64)h.buf[2] << 48) | ((ci_uint64)h.buf[3] << 56);
    h.v3 ^= m;
    sip_round(&h, 2);
    h.v0 ^= m;

    h.p = h.buf;
    h.c += 8;
  } while( pe - p > 0 );

  /* finish */
  left = h.p - h.buf;
  b = (h.c + left) << 56;

  switch (left) {
    case 7: b |= (uint64_t)h.buf[6] << 48;
    case 6: b |= (uint64_t)h.buf[5] << 40;
    case 5: b |= (uint64_t)h.buf[4] << 32;
    case 4: b |= (uint64_t)h.buf[3] << 24;
    case 3: b |= (uint64_t)h.buf[2] << 16;
    case 2: b |= (uint64_t)h.buf[1] << 8;
    case 1: b |= (uint64_t)h.buf[0] << 0;
    case 0: break;
  }

  h.v3 ^= b;
  sip_round(&h, 2);
  h.v0 ^= b;
  h.v2 ^= 0xff;
  sip_round(&h, 4);

  return h.v0 ^ h.v1 ^ h.v2  ^ h.v3;
}

static ci_uint32
ci_tcp_syncookie_hash(ci_netif* netif, ci_tcp_socket_listen* tls,
                      ci_tcp_state_synrecv* tsr, int t, int m)
{
  ci_uint8 hash_data[13];

  hash_data[0] = tcp_lport_be16(tls) & 0xff;
  hash_data[1] = tcp_lport_be16(tls) >> 8;
  hash_data[2] = tsr->r_port & 0xff;
  hash_data[3] = tsr->r_port >> 8;
  hash_data[4] = tsr->l_addr & 0xff;
  hash_data[5] = (tsr->l_addr & 0xff00) >> 8;
  hash_data[6] = (tsr->l_addr & 0xff0000) >> 16;
  hash_data[7] = (tsr->l_addr & 0xff000000) >> 24;
  hash_data[8] = tsr->r_addr & 0xff;
  hash_data[9] = (tsr->r_addr & 0xff00) >> 8;
  hash_data[10] = (tsr->r_addr & 0xff0000) >> 16;
  hash_data[11] = (tsr->r_addr & 0xff000000) >> 24;
  hash_data[12] = t << 3 | m;

  ci_assert_equal(sizeof(netif->state->syncookie_salt),
                  2 * sizeof(ci_uint64));
  return (ci_uint32)sip_hash((void *)netif->state->syncookie_salt,
                             hash_data, sizeof(hash_data));
}

/* End of siphash implementation */

 

static ci_int16 ci_tcp_syncookie_get_t(ci_netif* netif)
{
  /* tick2sec >> 3 */
  return (ci_ip_time_now(netif) >>
          (IPTIMER_STATE(netif)->ci_ip_time_frc2us + 23
           - IPTIMER_STATE(netif)->ci_ip_time_frc2tick)
          ) & 0x1f;
}

void
ci_tcp_syncookie_syn(ci_netif* netif, ci_tcp_socket_listen* tls,
                     ci_tcp_state_synrecv* tsr)
{
  int t, m;

  t = ci_tcp_syncookie_get_t(netif);

  if( tsr->tcpopts.smss >= netif->state->max_mss )
    m = 7;
  else {
    for( m = 6; m > 0; m-- )
      if( tsr->tcpopts.smss > syncookie_mss[m] )
        break;
  }

  /* Calculate sequence number */
  tsr->snd_isn = (t << 3) | m |
      (ci_tcp_syncookie_hash(netif, tls, tsr, t, m) << 8);

  /* disable all TCP options or put the info into timestamp */
  if( tsr->tcpopts.flags & CI_TCPT_FLAG_TSO ) {
    tsr->tcpopts.flags |= CI_TCPT_FLAG_SYNCOOKIE;
    tsr->timest &= ~0x1ff;
    if( tsr->tcpopts.flags & CI_TCPT_FLAG_WSCL ) {
      tsr->timest |= tsr->rcv_wscl;
      tsr->timest |= tsr->tcpopts.wscl_shft << 4;
    }
    if( tsr->tcpopts.flags & CI_TCPT_FLAG_SACK )
      tsr->timest |= 1 << 8;
    tsr->tcpopts.flags &= ~CI_TCPT_FLAG_ECN;

    if( tsr->timest > ci_tcp_time_now(netif) ) {
      tsr->timest -= 1 << 9;
    }
  }
  else
    tsr->tcpopts.flags = CI_TCPT_FLAG_SYNCOOKIE;

  CITP_STATS_TCP_LISTEN(++tls->stats.n_syncookie_syn);
}

void
ci_tcp_syncookie_ack(ci_netif* netif, ci_tcp_socket_listen* tls,
                     ciip_tcp_rx_pkt* rxp,
                     ci_tcp_state_synrecv** tsr_p)
{
  int t, m, t_now;
  ci_tcp_state_synrecv* tsr;
  ci_uint32 isn = rxp->ack - 1;

  CITP_STATS_TCP_LISTEN(++tls->stats.n_syncookie_ack_recv);
  *tsr_p = NULL;

  t_now = ci_tcp_syncookie_get_t(netif);

  m = isn & 7;
  t = (isn >> 3) & 0x1f;

  if( t != t_now && t != ((t_now - 1) & 0x1f) ) {
    CITP_STATS_TCP_LISTEN(++tls->stats.n_syncookie_ack_ts_rej);
    return;
  }

  tsr = ci_alloc(sizeof(ci_tcp_state_synrecv));
  if( tsr == NULL )
    return;
  memset(tsr, 0, sizeof(ci_tcp_state_synrecv));
  tsr->tcpopts.flags = CI_TCPT_FLAG_SYNCOOKIE;

  tsr->r_port = rxp->tcp->tcp_source_be16;
  tsr->l_addr = oo_ip_hdr(rxp->pkt)->ip_daddr_be32;
  tsr->r_addr = oo_ip_hdr(rxp->pkt)->ip_saddr_be32;
  tsr->tcpopts.smss = syncookie_mss[m];
  tsr->snd_isn = isn;
  tsr->rcv_nxt = rxp->seq;

  if( (isn >> 8) !=
      (ci_tcp_syncookie_hash(netif, tls, tsr, t, m) & 0xffffff) ) {
    CITP_STATS_TCP_LISTEN(++tls->stats.n_syncookie_ack_hash_rej);
    ci_free(tsr);
    return;
  }

  tsr->local_peer = OO_SP_NULL;

  *tsr_p = tsr;

  if( rxp->flags & CI_TCPT_FLAG_TSO ) {
    if( rxp->timestamp_echo & 0xff ) {
      tsr->tcpopts.flags |= CI_TCPT_FLAG_WSCL;
      tsr->rcv_wscl = rxp->timestamp_echo & 0xf;
      tsr->tcpopts.wscl_shft = (rxp->timestamp_echo >> 4) & 0xf;
    }
    if( rxp->timestamp_echo & 0x100 )
      tsr->tcpopts.flags |= CI_TCPT_FLAG_SACK;
  }

  CITP_STATS_TCP_LISTEN(++tls->stats.n_syncookie_ack_answ);
}

