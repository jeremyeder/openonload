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

/****************************************************************************
 * Driver for Solarflare network controllers -
 *          resource management for Xen backend, OpenOnload, etc
 *           (including support for SFE4001 10GBT NIC)
 *
 * This file provides EtherFabric NIC - EFXXXX (aka Falcon) event
 * definitions.
 *
 * Copyright 2005-2007: Solarflare Communications Inc,
 *                      9501 Jeronimo Road, Suite 250,
 *                      Irvine, CA 92618, USA
 *
 * Developed and maintained by Solarflare Communications:
 *                      <linux-xen-drivers@solarflare.com>
 *                      <onload-dev@solarflare.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation, incorporated herein by reference.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 ****************************************************************************
 */

/*************---- Events Format C Header ----*************/
/*************---- Event entry ----*************/
  #define EV_CODE_LBN 60
  #define EV_CODE_WIDTH 4
      #define RX_IP_EV_DECODE 0
      #define TX_IP_EV_DECODE 2
      #define DRIVER_EV_DECODE 5
      #define GLOBAL_EV_DECODE 6
      #define DRV_GEN_EV_DECODE 7
  #define EV_DATA_LBN 0
  #define EV_DATA_WIDTH 60
/******---- Receive IP events for both Kernel & User event queues ----******/
  #define RX_EV_PKT_OK_LBN 56
  #define RX_EV_PKT_OK_WIDTH 1
  #define RX_EV_BUF_OWNER_ID_ERR_LBN 54
  #define RX_EV_BUF_OWNER_ID_ERR_WIDTH 1
  #define RX_EV_IP_FRAG_ERR_LBN 53
  #define RX_EV_IP_FRAG_ERR_WIDTH 1
  #define RX_EV_IP_HDR_CHKSUM_ERR_LBN 52
  #define RX_EV_IP_HDR_CHKSUM_ERR_WIDTH 1
  #define RX_EV_TCP_UDP_CHKSUM_ERR_LBN 51
  #define RX_EV_TCP_UDP_CHKSUM_ERR_WIDTH 1
  #define RX_EV_ETH_CRC_ERR_LBN 50
  #define RX_EV_ETH_CRC_ERR_WIDTH 1
  #define RX_EV_FRM_TRUNC_LBN 49
  #define RX_EV_FRM_TRUNC_WIDTH 1
  #define RX_EV_DRIB_NIB_LBN 48
  #define RX_EV_DRIB_NIB_WIDTH 1
  #define RX_EV_TOBE_DISC_LBN 47
  #define RX_EV_TOBE_DISC_WIDTH 1
  #define RX_EV_PKT_TYPE_LBN 44
  #define RX_EV_PKT_TYPE_WIDTH 3
      #define RX_EV_PKT_TYPE_ETH_DECODE 0
      #define RX_EV_PKT_TYPE_LLC_DECODE 1
      #define RX_EV_PKT_TYPE_JUMBO_DECODE 2
      #define RX_EV_PKT_TYPE_VLAN_DECODE 3
      #define RX_EV_PKT_TYPE_VLAN_LLC_DECODE 4
      #define RX_EV_PKT_TYPE_VLAN_JUMBO_DECODE 5
  #define RX_EV_HDR_TYPE_LBN 42
  #define RX_EV_HDR_TYPE_WIDTH 2
      #define RX_EV_HDR_TYPE_TCP_IPV4_DECODE 0
      #define RX_EV_HDR_TYPE_UDP_IPV4_DECODE 1
      #define RX_EV_HDR_TYPE_OTHER_IP_DECODE 2
      #define RX_EV_HDR_TYPE_NON_IP_DECODE 3
  #define RX_EV_DESC_Q_EMPTY_LBN 41
  #define RX_EV_DESC_Q_EMPTY_WIDTH 1
  #define RX_EV_MCAST_HASH_MATCH_LBN 40
  #define RX_EV_MCAST_HASH_MATCH_WIDTH 1
  #define RX_EV_MCAST_PKT_LBN 39
  #define RX_EV_MCAST_PKT_WIDTH 1
  #define RX_EV_Q_LABEL_LBN 32
  #define RX_EV_Q_LABEL_WIDTH 5
  #define RX_JUMBO_CONT_LBN 31
  #define RX_JUMBO_CONT_WIDTH 1
  #define RX_SOP_LBN 15
  #define RX_SOP_WIDTH 1
  #define RX_PORT_LBN 30
  #define RX_PORT_WIDTH 1
  #define RX_EV_BYTE_CNT_LBN 16
  #define RX_EV_BYTE_CNT_WIDTH 14
  #define RX_iSCSI_PKT_OK_LBN 14
  #define RX_iSCSI_PKT_OK_WIDTH 1
  #define RX_ISCSI_DDIG_ERR_LBN 13
  #define RX_ISCSI_DDIG_ERR_WIDTH 1
  #define RX_ISCSI_HDIG_ERR_LBN 12
  #define RX_ISCSI_HDIG_ERR_WIDTH 1
  #define RX_EV_DESC_PTR_LBN 0
  #define RX_EV_DESC_PTR_WIDTH 12
/******---- Transmit IP events for both Kernel & User event queues ----******/
  #define TX_EV_PKT_ERR_LBN 38
  #define TX_EV_PKT_ERR_WIDTH 1
  #define TX_EV_PKT_TOO_BIG_LBN 37
  #define TX_EV_PKT_TOO_BIG_WIDTH 1
  #define TX_EV_Q_LABEL_LBN 32
  #define TX_EV_Q_LABEL_WIDTH 5
  #define TX_EV_PORT_LBN 16
  #define TX_EV_PORT_WIDTH 1
  #define TX_EV_WQ_FF_FULL_LBN 15
  #define TX_EV_WQ_FF_FULL_WIDTH 1
  #define TX_EV_BUF_OWNER_ID_ERR_LBN 14
  #define TX_EV_BUF_OWNER_ID_ERR_WIDTH 1
  #define TX_EV_COMP_LBN 12
  #define TX_EV_COMP_WIDTH 1
  #define TX_EV_DESC_PTR_LBN 0
  #define TX_EV_DESC_PTR_WIDTH 12
/*************---- Char or Kernel driver events ----*************/
  #define DRIVER_EV_SUB_CODE_LBN 56
  #define DRIVER_EV_SUB_CODE_WIDTH 4
      #define TX_DESCQ_FLS_DONE_EV_DECODE 0x0
      #define RX_DESCQ_FLS_DONE_EV_DECODE 0x1
      #define EVQ_INIT_DONE_EV_DECODE 0x2
      #define EVQ_NOT_EN_EV_DECODE 0x3
      #define RX_DESCQ_FLSFF_OVFL_EV_DECODE 0x4
      #define SRM_UPD_DONE_EV_DECODE 0x5
      #define WAKE_UP_EV_DECODE 0x6
      #define TX_PKT_NON_TCP_UDP_DECODE 0x9
      #define TIMER_EV_DECODE 0xA
      #define RX_DSC_ERROR_EV_DECODE 0xE
  #define DRIVER_EV_RX_FLUSH_FAIL_LBN 12
  #define DRIVER_EV_RX_FLUSH_FAIL_WIDTH 1
  #define DRIVER_EV_TX_DESCQ_ID_LBN 0
  #define DRIVER_EV_TX_DESCQ_ID_WIDTH 12
  #define DRIVER_EV_RX_DESCQ_ID_LBN 0
  #define DRIVER_EV_RX_DESCQ_ID_WIDTH 12
  #define DRIVER_EV_EVQ_ID_LBN 0
  #define DRIVER_EV_EVQ_ID_WIDTH 12
  #define DRIVER_TMR_ID_LBN 0
  #define DRIVER_TMR_ID_WIDTH 12
  #define DRIVER_EV_SRM_UPD_LBN 0
  #define DRIVER_EV_SRM_UPD_WIDTH 2
      #define SRM_CLR_EV_DECODE 0
      #define SRM_UPD_EV_DECODE 1
      #define SRM_ILLCLR_EV_DECODE 2
/********---- Global events. Sent to both event queue 0 and 4. ----********/
  #define XFP_PHY_INTR_LBN 10
  #define XFP_PHY_INTR_WIDTH 1
  #define XG_PHY_INTR_LBN 9
  #define XG_PHY_INTR_WIDTH 1
  #define G_PHY1_INTR_LBN 8
  #define G_PHY1_INTR_WIDTH 1
  #define G_PHY0_INTR_LBN 7
  #define G_PHY0_INTR_WIDTH 1
/*************---- Driver generated events ----*************/
  #define DRV_GEN_EV_CODE_LBN 60
  #define DRV_GEN_EV_CODE_WIDTH 4
  #define DRV_GEN_EV_DATA_LBN 0
  #define DRV_GEN_EV_DATA_WIDTH 60

/* RX packet prefix */
#define FS_BZ_RX_PREFIX_HASH_OFST 12
#define FS_BZ_RX_PREFIX_SIZE 16
