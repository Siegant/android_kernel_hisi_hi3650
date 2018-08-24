/*
* NoC. (NoC Mntn Module.)
*
* Copyright (c) 2016 Huawei Technologies CO., Ltd.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

/*****************************************************************************
  1 头文件包含
 *****************************************************************************/
#include <linux/io.h>
#include <linux/string.h>

#include <hisi_noc_info.h>

#define ERR_CODE_NR  8
#define OPC_NR      10

#define CFG_INITFLOW_ARRAY_SIZE			32
#define CFG_TARGETFLOW_ARRAY_SIZE		32
#define VCODEC_INITFLOW_ARRAY_SIZE		4
#define VCODEC_TARGETFLOW_ARRAY_SIZE	8
#define VIVO_INITFLOW_ARRAY_SIZE		8
#define VIVO_TARGETFLOW_ARRAY_SIZE		8

static char *cfg_initflow_array[] = {
	"SD3(noc_mmc1bus_sd3)",
	"Sdio1(noc_mmc1bus_sdio1)",
	"A7(noc_a7tocfg)",
	"Audio(noc_asp_mst)",
	"ACPU(noc_cci2sysbus)",
	"DJTAG(noc_djtag_mst)",
	"DMA Controller(noc_dmac_mst)",
	"EMMC0(noc_emmc0)",
	"SensorHub(noc_iomcu_ahb_mst)",
	"SensorHub(noc_iomcu_axi_mst)",
	"DMA-SensorHub(noc_iomcu_dma_mst)",
	"(noc_ipf)",
	"LPM3(noc_lpmcu_mst)",
	"MODEM(noc_modem_mst)",
	"PERF_STAT_DEBUG(noc_perf_stat)",
	"SECURITY(noc_sec_p)",
	"SECURITY(noc_sec_s)",
	"SOCP_DEBUG(noc_socp)",
	"CORESIGHT(noc_top_cssys)",
	"USB2(noc_usb2)",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
};

static char *cfg_targetflow_array[] = {
	"Audio(aspbus_service_target)",
	"Service_target(cfgbus_service_target)",
	"Service_target(dbgbus_service_target)",
	"Service_target(dmabus_service_target)",
	"Service_target(mmc0bus_service_target)",
	"Service_target(mmc1bus_service_target)",
	"Service_target(modembus_service_target)",
	"AO-MMBUF(noc_aobus_mmbuf)",
	"Audio(noc_asp_cfg)",
	"(noc_cci_cfg)",
	"Vivo(noc_cfg2vivo)",
	"debug(noc_debug_ahb_cfg)",
	"debug(noc_debug_apb_cfg)",
	"DMA controller(noc_dmac_cfg)",
	"EMMC0(noc_emmc0_cfg)",
	"GIC(noc_gic)",
	"SSI(noc_hkadc_ssi)",
	"IOMCU(noc_iomcu_ahb_slv)",
	"IOMCU(noc_iomcu_apb_slv)",
	"LPM3(noc_lpmcu_slv)",
	"SDIO1/SD3(noc_mmc1bus_apb_cfg)",
	"MODEM(noc_modem_cfg)",
	"DDRC(noc_sys2ddrc)",
	"HKMEM(noc_sys2hkmem)",
	"CFG(noc_sys_peri0_cfg)",
	"CFG(noc_sys_peri1_cfg)",
	"DMA(noc_sys_peri2_cfg)",
	"DMA(noc_sys_peri3_cfg)",
	"CORESIGHT(noc_top_cssys_slv)",
	"Service_target(sysbus_service_target)",
	"RESERVED",
	"RESERVED",
};

static char *vcodec_initflow_array[] = {
	"VCODEC(noc_vcodec_cfg)",
	"VDEC(noc_vdec)",
	"VENC(noc_venc)",
	"RESERVED",
};

static char *vcodec_targetflow_array[] = {
	"DDRC(noc_vcodecbus_ddrc)",
	"VDEC(noc_vdec_cfg)",
	"VENC(noc_venc_cfg)",
	"service_target (vcodec_bus_service_target)",
	"service_target (vdec_service_target)",
	"venc_service_target",
	"RESERVED",
	"RESERVED",
};

static char *vivo_initflow_array[] = {
	"A7_RD(noc_a7tovivobus_rd)",
	"A7_WR (noc_a7tovivobus_wr)",
	"DSS0_RD(noc_dss0_rd)",
	"DSS1_RD (noc_dss1_rd)",
	"DSS1_WR(noc_dss1_wr)",
	"ISP_RD(noc_isp_rd)",
	"ISP_WR(noc_isp_wr)",
	"VIVO(noc_vivo_cfg)",
};

static char *vivo_targetflow_array[VIVO_TARGETFLOW_ARRAY_SIZE] = {
	"service_target (dss_service_target)",
	"service_target (isp_service_target)",
	"service_target (noc_dss_cfg)",
	"ISP(noc_isp_cfg)",
	"DDRC(noc_vivobus_ddrc0_rd)",
	"DDRC (noc_vivobus_ddrc0_wr)",
	"DDRC (noc_vivobus_ddrc1_rd)",
	"DDRC (noc_vivobus_ddrc1_wr)"
};

/*
 * unsigned int  opc      : 4;  bit[1-4]  : Transaction操作类型：
 * 0-->RD：INCR的读操作；
 * 1-->RDW：WRAP的读操作；
 * 2-->RDL：Exclusive 读操作；
 * 3-->RDX：Lock read；
 * 4-->WR：INCR写操作；
 * 5-->WRW：WRAP写操作；
 * 6-->WRC：Exclusive写操作；
 * 8-->PRE：FIXED BURST；
 * 9-->URG：urgency packet，Error Probe不会检测到此类packet
 * 其它-->Reserveed：非法操作
 */
static char *opc_array[] = {
	"RD:  INCR READ",
	"RDW: WRAP READ",
	"RDL: EXCLUSIVE READ",
	"RDX: LOCK READ",
	"WR:  INCR WRITE",
	"WRW: WRAP WRITE",
	"WRC：EXCLUSIVE WRITE",
	"Reversed",
	"PRE: FIXED BURST",
	"Abnormal"
};

/*
 * unsigned int  errcode  : 3;  bit[8-10] : 错误类型
 * 0-->被访问slave返回Error Response；
 * 1-->master访问了总线的Reserve地址空间；
 * 2-->master发送了slave不支持的Burst类型（当前总线不会出现这种场景）；
 * 3-->master访问了掉电区域；
 * 4-->访问权限报错；
 * 5-->master访问时，收到Firewall的Hide Error Response；
 * 6-->被访问slave TimeOut，并返回Error Response；
 * 7-->none
 */
static char *err_code_array[] = {
	"Slave:  returns Error response",
	"Master: access reserved memory space",
	"Master: send the illigel type burst to slave",
	"Master: access the powerdown area",
	"Master: Permission error",
	"Master: received Hide Error Response from Firewall",
	"Master: accessed slave timeout and returned Error reponse",
	"None"
};

static const ROUTE_ID_ADDR_STRU cfgsys_routeid_addr_tbl[] = {
	/*Init_flow_bit   Targ_flow_bit    Targ subrange  Init localAddress*/
	/*-----------------------------------------------------------------*/
	{0x02, 0x00, 0x0, 0xe9830000},
	{0x02, 0x01, 0x0, 0xe9800000},
	{0x02, 0x02, 0x0, 0xe9870000},
	{0x02, 0x03, 0x0, 0xe9860000},
	{0x02, 0x04, 0x0, 0xe9890000},
	{0x02, 0x05, 0x0, 0xe9880000},
	{0x02, 0x06, 0x0, 0xe9820000},
	{0x02, 0x07, 0x0, 0xea840000},
	{0x02, 0x07, 0x1, 0xea800000},
	{0x02, 0x08, 0x0, 0xe8000000},
	{0x02, 0x09, 0x0, 0xe8100000},
	{0x02, 0x0B, 0x0, 0xff200000},
	{0x02, 0x0B, 0x1, 0xff030000},
	{0x02, 0x0B, 0x2, 0xff040000},
	{0x02, 0x0B, 0x3, 0xff100000},
	{0x02, 0x0C, 0x0, 0xff010000},
	{0x02, 0x0D, 0x0, 0xfdf30000},
	{0x02, 0x0E, 0x0, 0xff390000},
	{0x02, 0x0F, 0x0, 0xe82b0000},
	{0x02, 0x10, 0x0, 0xe82b8000},
	{0x02, 0x11, 0x0, 0xf0800000},
	{0x02, 0x11, 0x1, 0xf0000000},
	{0x02, 0x12, 0x0, 0xffd00000},
	{0x02, 0x13, 0x0, 0xffe00000},
	{0x02, 0x13, 0x1, 0xed000000},
	{0x02, 0x13, 0x2, 0xec000000},
	{0x02, 0x14, 0x0, 0xff37c000},
	{0x02, 0x15, 0x0, 0xe0000000},
	{0x02, 0x17, 0x0, 0xea000000},
	{0x02, 0x17, 0x1, 0x0},
	{0x02, 0x18, 0x0, 0xe8a00000},
	{0x02, 0x19, 0x0, 0xe8800000},
	{0x02, 0x1A, 0x0, 0xfdf20000},
	{0x02, 0x1B, 0x0, 0xfdf00000},
	{0x02, 0x1C, 0x0, 0xfe000000},
	{0x02, 0x1D, 0x0, 0xe9840000},
	{0x03, 0x00, 0x0, 0xe9830000},
	{0x03, 0x01, 0x0, 0xe9800000},
	{0x03, 0x02, 0x0, 0xe9870000},
	{0x03, 0x03, 0x0, 0xe9860000},
	{0x03, 0x04, 0x0, 0xe9890000},
	{0x03, 0x05, 0x0, 0xe9880000},
	{0x03, 0x06, 0x0, 0xe9820000},
	{0x03, 0x07, 0x0, 0xea840000},
	{0x03, 0x07, 0x1, 0xea800000},
	{0x03, 0x09, 0x0, 0xe8100000},
	{0x03, 0x0A, 0x0, 0xe8400000},
	{0x03, 0x0B, 0x0, 0xff200000},
	{0x03, 0x0B, 0x1, 0xff030000},
	{0x03, 0x0B, 0x2, 0xff040000},
	{0x03, 0x0B, 0x3, 0xff100000},
	{0x03, 0x0C, 0x0, 0xff010000},
	{0x03, 0x0D, 0x0, 0xfdf30000},
	{0x03, 0x0E, 0x0, 0xff390000},
	{0x03, 0x0F, 0x0, 0xe82b0000},
	{0x03, 0x10, 0x0, 0xe82b8000},
	{0x03, 0x11, 0x0, 0xf0800000},
	{0x03, 0x11, 0x1, 0xf0000000},
	{0x03, 0x12, 0x0, 0xffd00000},
	{0x03, 0x13, 0x0, 0xffe00000},
	{0x03, 0x13, 0x1, 0xed000000},
	{0x03, 0x13, 0x2, 0xec000000},
	{0x03, 0x14, 0x0, 0xff37c000},
	{0x03, 0x15, 0x0, 0xe0000000},
	{0x03, 0x16, 0x0, 0xc0000000},
	{0x03, 0x16, 0x1, 0x80000000},
	{0x03, 0x16, 0x2, 0x0},
	{0x03, 0x17, 0x0, 0xea000000},
	{0x03, 0x17, 0x1, 0x0},
	{0x03, 0x18, 0x0, 0xe8a00000},
	{0x03, 0x19, 0x0, 0xe8800000},
	{0x03, 0x1A, 0x0, 0xfdf20000},
	{0x03, 0x1B, 0x0, 0xfdf00000},
	{0x03, 0x1C, 0x0, 0xfe000000},
	{0x03, 0x1D, 0x0, 0xe9840000},
	{0x04, 0x00, 0x0, 0xe9830000},
	{0x04, 0x01, 0x0, 0xe9800000},
	{0x04, 0x02, 0x0, 0xe9870000},
	{0x04, 0x03, 0x0, 0xe9860000},
	{0x04, 0x04, 0x0, 0xe9890000},
	{0x04, 0x05, 0x0, 0xe9880000},
	{0x04, 0x06, 0x0, 0xe9820000},
	{0x04, 0x07, 0x0, 0xea840000},
	{0x04, 0x07, 0x1, 0xea800000},
	{0x04, 0x08, 0x0, 0xe8000000},
	{0x04, 0x09, 0x0, 0xe8100000},
	{0x04, 0x0A, 0x0, 0xe8400000},
	{0x04, 0x0B, 0x0, 0xff200000},
	{0x04, 0x0B, 0x1, 0xff030000},
	{0x04, 0x0B, 0x2, 0xff040000},
	{0x04, 0x0B, 0x3, 0xff100000},
	{0x04, 0x0C, 0x0, 0xff010000},
	{0x04, 0x0D, 0x0, 0xfdf30000},
	{0x04, 0x0E, 0x0, 0xff390000},
	{0x04, 0x0F, 0x0, 0xe82b0000},
	{0x04, 0x10, 0x0, 0xe82b8000},
	{0x04, 0x11, 0x0, 0xf0800000},
	{0x04, 0x11, 0x1, 0xf0000000},
	{0x04, 0x12, 0x0, 0xffd00000},
	{0x04, 0x13, 0x0, 0xffe00000},
	{0x04, 0x13, 0x1, 0xed000000},
	{0x04, 0x13, 0x2, 0xec000000},
	{0x04, 0x14, 0x0, 0xff37c000},
	{0x04, 0x15, 0x0, 0xe0000000},
	{0x04, 0x17, 0x0, 0xea000000},
	{0x04, 0x17, 0x1, 0x0},
	{0x04, 0x18, 0x0, 0xe8a00000},
	{0x04, 0x19, 0x0, 0xe8800000},
	{0x04, 0x1A, 0x0, 0xfdf20000},
	{0x04, 0x1B, 0x0, 0xfdf00000},
	{0x04, 0x1C, 0x0, 0xfe000000},
	{0x04, 0x1D, 0x0, 0xe9840000},
	{0x05, 0x00, 0x0, 0xe9830000},
	{0x05, 0x01, 0x0, 0xe9800000},
	{0x05, 0x02, 0x0, 0xe9870000},
	{0x05, 0x03, 0x0, 0xe9860000},
	{0x05, 0x04, 0x0, 0xe9890000},
	{0x05, 0x05, 0x0, 0xe9880000},
	{0x05, 0x06, 0x0, 0xe9820000},
	{0x05, 0x07, 0x0, 0xea840000},
	{0x05, 0x07, 0x1, 0xea800000},
	{0x05, 0x08, 0x0, 0xe8000000},
	{0x05, 0x09, 0x0, 0xe8100000},
	{0x05, 0x0A, 0x0, 0xe8400000},
	{0x05, 0x0B, 0x0, 0xff200000},
	{0x05, 0x0B, 0x1, 0xff030000},
	{0x05, 0x0B, 0x2, 0xff040000},
	{0x05, 0x0B, 0x3, 0xff100000},
	{0x05, 0x0C, 0x0, 0xff010000},
	{0x05, 0x0D, 0x0, 0xfdf30000},
	{0x05, 0x0E, 0x0, 0xff390000},
	{0x05, 0x0F, 0x0, 0xe82b0000},
	{0x05, 0x10, 0x0, 0xe82b8000},
	{0x05, 0x11, 0x0, 0xf0800000},
	{0x05, 0x11, 0x1, 0xf0000000},
	{0x05, 0x12, 0x0, 0xffd00000},
	{0x05, 0x13, 0x0, 0xffe00000},
	{0x05, 0x13, 0x1, 0xed000000},
	{0x05, 0x13, 0x2, 0xec000000},
	{0x05, 0x14, 0x0, 0xff37c000},
	{0x05, 0x15, 0x0, 0xe0000000},
	{0x05, 0x16, 0x0, 0xc0000000},
	{0x05, 0x16, 0x1, 0x80000000},
	{0x05, 0x16, 0x2, 0x0},
	{0x05, 0x17, 0x0, 0xea000000},
	{0x05, 0x17, 0x1, 0x0},
	{0x05, 0x18, 0x0, 0xe8a00000},
	{0x05, 0x19, 0x0, 0xe8800000},
	{0x05, 0x1A, 0x0, 0xfdf20000},
	{0x05, 0x1B, 0x0, 0xfdf00000},
	{0x05, 0x1C, 0x0, 0xfe000000},
	{0x05, 0x1D, 0x0, 0xe9840000},
	{0x06, 0x00, 0x0, 0xe9830000},
	{0x06, 0x01, 0x0, 0xe9800000},
	{0x06, 0x02, 0x0, 0xe9870000},
	{0x06, 0x03, 0x0, 0xe9860000},
	{0x06, 0x04, 0x0, 0xe9890000},
	{0x06, 0x05, 0x0, 0xe9880000},
	{0x06, 0x06, 0x0, 0xe9820000},
	{0x06, 0x07, 0x0, 0xea840000},
	{0x06, 0x07, 0x1, 0xea800000},
	{0x06, 0x08, 0x0, 0xe8000000},
	{0x06, 0x09, 0x0, 0xe8100000},
	{0x06, 0x0A, 0x0, 0xe8400000},
	{0x06, 0x0B, 0x0, 0xff200000},
	{0x06, 0x0B, 0x1, 0xff030000},
	{0x06, 0x0B, 0x2, 0xff040000},
	{0x06, 0x0B, 0x3, 0xff100000},
	{0x06, 0x0C, 0x0, 0xff010000},
	{0x06, 0x0D, 0x0, 0xfdf30000},
	{0x06, 0x0E, 0x0, 0xff390000},
	{0x06, 0x0F, 0x0, 0xe82b0000},
	{0x06, 0x10, 0x0, 0xe82b8000},
	{0x06, 0x11, 0x0, 0xf0800000},
	{0x06, 0x11, 0x1, 0xf0000000},
	{0x06, 0x12, 0x0, 0xffd00000},
	{0x06, 0x13, 0x0, 0xffe00000},
	{0x06, 0x13, 0x1, 0xed000000},
	{0x06, 0x13, 0x2, 0xec000000},
	{0x06, 0x14, 0x0, 0xff37c000},
	{0x06, 0x15, 0x0, 0xe0000000},
	{0x06, 0x16, 0x0, 0xc0000000},
	{0x06, 0x16, 0x1, 0x80000000},
	{0x06, 0x16, 0x2, 0x0},
	{0x06, 0x17, 0x0, 0xea000000},
	{0x06, 0x17, 0x1, 0x0},
	{0x06, 0x18, 0x0, 0xe8a00000},
	{0x06, 0x19, 0x0, 0xe8800000},
	{0x06, 0x1A, 0x0, 0xfdf20000},
	{0x06, 0x1B, 0x0, 0xfdf00000},
	{0x06, 0x1C, 0x0, 0xfe000000},
	{0x06, 0x1D, 0x0, 0xe9840000},
	{0x07, 0x00, 0x0, 0xe9830000},
	{0x07, 0x01, 0x0, 0xe9800000},
	{0x07, 0x02, 0x0, 0xe9870000},
	{0x07, 0x03, 0x0, 0xe9860000},
	{0x07, 0x04, 0x0, 0xe9890000},
	{0x07, 0x05, 0x0, 0xe9880000},
	{0x07, 0x06, 0x0, 0xe9820000},
	{0x07, 0x07, 0x0, 0xea840000},
	{0x07, 0x07, 0x1, 0xea800000},
	{0x07, 0x08, 0x0, 0xe8000000},
	{0x07, 0x09, 0x0, 0xe8100000},
	{0x07, 0x0A, 0x0, 0xe8400000},
	{0x07, 0x0B, 0x0, 0xff200000},
	{0x07, 0x0B, 0x1, 0xff030000},
	{0x07, 0x0B, 0x2, 0xff040000},
	{0x07, 0x0B, 0x3, 0xff100000},
	{0x07, 0x0C, 0x0, 0xff010000},
	{0x07, 0x0D, 0x0, 0xfdf30000},
	{0x07, 0x0E, 0x0, 0xff390000},
	{0x07, 0x0F, 0x0, 0xe82b0000},
	{0x07, 0x10, 0x0, 0xe82b8000},
	{0x07, 0x11, 0x0, 0xf0800000},
	{0x07, 0x11, 0x1, 0xf0000000},
	{0x07, 0x12, 0x0, 0xffd00000},
	{0x07, 0x13, 0x0, 0xffe00000},
	{0x07, 0x13, 0x1, 0xed000000},
	{0x07, 0x13, 0x2, 0xec000000},
	{0x07, 0x14, 0x0, 0xff37c000},
	{0x07, 0x15, 0x0, 0xe0000000},
	{0x07, 0x16, 0x0, 0xc0000000},
	{0x07, 0x16, 0x1, 0x80000000},
	{0x07, 0x16, 0x2, 0x0},
	{0x07, 0x17, 0x0, 0xea000000},
	{0x07, 0x17, 0x1, 0x0},
	{0x07, 0x18, 0x0, 0xe8a00000},
	{0x07, 0x19, 0x0, 0xe8800000},
	{0x07, 0x1A, 0x0, 0xfdf20000},
	{0x07, 0x1B, 0x0, 0xfdf00000},
	{0x07, 0x1C, 0x0, 0xfe000000},
	{0x07, 0x1D, 0x0, 0xe9840000},
	{0x08, 0x00, 0x0, 0xe9830000},
	{0x08, 0x01, 0x0, 0xe9800000},
	{0x08, 0x02, 0x0, 0xe9870000},
	{0x08, 0x03, 0x0, 0xe9860000},
	{0x08, 0x04, 0x0, 0xe9890000},
	{0x08, 0x05, 0x0, 0xe9880000},
	{0x08, 0x06, 0x0, 0xe9820000},
	{0x08, 0x08, 0x0, 0xe8000000},
	{0x08, 0x09, 0x0, 0xe8100000},
	{0x08, 0x0A, 0x0, 0xe8400000},
	{0x08, 0x0B, 0x0, 0xff200000},
	{0x08, 0x0B, 0x1, 0xff030000},
	{0x08, 0x0B, 0x2, 0xff040000},
	{0x08, 0x0B, 0x3, 0xff100000},
	{0x08, 0x0C, 0x0, 0xff010000},
	{0x08, 0x0D, 0x0, 0xfdf30000},
	{0x08, 0x0E, 0x0, 0xff390000},
	{0x08, 0x0F, 0x0, 0xe82b0000},
	{0x08, 0x10, 0x0, 0xe82b8000},
	{0x08, 0x12, 0x0, 0xffd00000},
	{0x08, 0x13, 0x0, 0xffe00000},
	{0x08, 0x13, 0x1, 0xed000000},
	{0x08, 0x13, 0x2, 0xec000000},
	{0x08, 0x14, 0x0, 0xff37c000},
	{0x08, 0x15, 0x0, 0xe0000000},
	{0x08, 0x17, 0x0, 0xea000000},
	{0x08, 0x17, 0x1, 0x0},
	{0x08, 0x18, 0x0, 0xe8a00000},
	{0x08, 0x19, 0x0, 0xe8800000},
	{0x08, 0x1A, 0x0, 0xfdf20000},
	{0x08, 0x1B, 0x0, 0xfdf00000},
	{0x08, 0x1C, 0x0, 0xfe000000},
	{0x08, 0x1D, 0x0, 0xe9840000},
	{0x09, 0x07, 0x0, 0xea840000},
	{0x09, 0x07, 0x1, 0xea800000},
	{0x09, 0x08, 0x0, 0xe8000000},
	{0x09, 0x16, 0x0, 0xc0000000},
	{0x09, 0x16, 0x1, 0x80000000},
	{0x09, 0x16, 0x2, 0x0},
	{0x09, 0x17, 0x0, 0xea000000},
	{0x09, 0x17, 0x1, 0x0},
	{0x0A, 0x00, 0x0, 0xe9830000},
	{0x0A, 0x01, 0x0, 0xe9800000},
	{0x0A, 0x02, 0x0, 0xe9870000},
	{0x0A, 0x03, 0x0, 0xe9860000},
	{0x0A, 0x04, 0x0, 0xe9890000},
	{0x0A, 0x05, 0x0, 0xe9880000},
	{0x0A, 0x06, 0x0, 0xe9820000},
	{0x0A, 0x07, 0x0, 0xea840000},
	{0x0A, 0x07, 0x1, 0xea800000},
	{0x0A, 0x08, 0x0, 0xe8000000},
	{0x0A, 0x09, 0x0, 0xe8100000},
	{0x0A, 0x0A, 0x0, 0xe8400000},
	{0x0A, 0x0B, 0x0, 0xff200000},
	{0x0A, 0x0B, 0x1, 0xff030000},
	{0x0A, 0x0B, 0x2, 0xff040000},
	{0x0A, 0x0B, 0x3, 0xff100000},
	{0x0A, 0x0C, 0x0, 0xff010000},
	{0x0A, 0x0D, 0x0, 0xfdf30000},
	{0x0A, 0x0E, 0x0, 0xff390000},
	{0x0A, 0x0F, 0x0, 0xe82b0000},
	{0x0A, 0x10, 0x0, 0xe82b8000},
	{0x0A, 0x11, 0x0, 0xf0800000},
	{0x0A, 0x11, 0x1, 0xf0000000},
	{0x0A, 0x12, 0x0, 0xffd00000},
	{0x0A, 0x13, 0x0, 0xffe00000},
	{0x0A, 0x13, 0x1, 0xed000000},
	{0x0A, 0x13, 0x2, 0xec000000},
	{0x0A, 0x14, 0x0, 0xff37c000},
	{0x0A, 0x15, 0x0, 0xe0000000},
	{0x0A, 0x16, 0x0, 0xc0000000},
	{0x0A, 0x16, 0x1, 0x80000000},
	{0x0A, 0x16, 0x2, 0x0},
	{0x0A, 0x17, 0x0, 0xea000000},
	{0x0A, 0x17, 0x1, 0x0},
	{0x0A, 0x18, 0x0, 0xe8a00000},
	{0x0A, 0x19, 0x0, 0xe8800000},
	{0x0A, 0x1A, 0x0, 0xfdf20000},
	{0x0A, 0x1B, 0x0, 0xfdf00000},
	{0x0A, 0x1C, 0x0, 0xfe000000},
	{0x0A, 0x1D, 0x0, 0xe9840000},
	{0x0B, 0x00, 0x0, 0xe9830000},
	{0x0B, 0x01, 0x0, 0xe9800000},
	{0x0B, 0x02, 0x0, 0xe9870000},
	{0x0B, 0x03, 0x0, 0xe9860000},
	{0x0B, 0x04, 0x0, 0xe9890000},
	{0x0B, 0x05, 0x0, 0xe9880000},
	{0x0B, 0x06, 0x0, 0xe9820000},
	{0x0B, 0x07, 0x0, 0xea840000},
	{0x0B, 0x07, 0x1, 0xea800000},
	{0x0B, 0x08, 0x0, 0xe8000000},
	{0x0B, 0x09, 0x0, 0xe8100000},
	{0x0B, 0x0A, 0x0, 0xe8400000},
	{0x0B, 0x0B, 0x0, 0xff200000},
	{0x0B, 0x0B, 0x1, 0xff030000},
	{0x0B, 0x0B, 0x2, 0xff040000},
	{0x0B, 0x0B, 0x3, 0xff100000},
	{0x0B, 0x0C, 0x0, 0xff010000},
	{0x0B, 0x0D, 0x0, 0xfdf30000},
	{0x0B, 0x0E, 0x0, 0xff390000},
	{0x0B, 0x0F, 0x0, 0xe82b0000},
	{0x0B, 0x10, 0x0, 0xe82b8000},
	{0x0B, 0x11, 0x0, 0xf0800000},
	{0x0B, 0x11, 0x1, 0xf0000000},
	{0x0B, 0x12, 0x0, 0xffd00000},
	{0x0B, 0x13, 0x0, 0xffe00000},
	{0x0B, 0x13, 0x1, 0xed000000},
	{0x0B, 0x13, 0x2, 0xec000000},
	{0x0B, 0x14, 0x0, 0xff37c000},
	{0x0B, 0x15, 0x0, 0xe0000000},
	{0x0B, 0x16, 0x0, 0xc0000000},
	{0x0B, 0x16, 0x1, 0x80000000},
	{0x0B, 0x16, 0x2, 0x0},
	{0x0B, 0x17, 0x0, 0xea000000},
	{0x0B, 0x17, 0x1, 0x0},
	{0x0B, 0x18, 0x0, 0xe8a00000},
	{0x0B, 0x19, 0x0, 0xe8800000},
	{0x0B, 0x1A, 0x0, 0xfdf20000},
	{0x0B, 0x1B, 0x0, 0xfdf00000},
	{0x0B, 0x1C, 0x0, 0xfe000000},
	{0x0B, 0x1D, 0x0, 0xe9840000},
	{0x0c, 0x00, 0x0, 0xe9830000},
	{0x0c, 0x01, 0x0, 0xe9800000},
	{0x0c, 0x02, 0x0, 0xe9870000},
	{0x0c, 0x03, 0x0, 0xe9860000},
	{0x0c, 0x04, 0x0, 0xe9890000},
	{0x0c, 0x05, 0x0, 0xe9880000},
	{0x0c, 0x06, 0x0, 0xe9820000},
	{0x0c, 0x07, 0x0, 0xea840000},
	{0x0c, 0x07, 0x1, 0xea800000},
	{0x0c, 0x08, 0x0, 0xe8000000},
	{0x0c, 0x09, 0x0, 0xe8100000},
	{0x0c, 0x0A, 0x0, 0xe8400000},
	{0x0c, 0x0B, 0x0, 0xff200000},
	{0x0c, 0x0B, 0x1, 0xff030000},
	{0x0c, 0x0B, 0x2, 0xff040000},
	{0x0c, 0x0B, 0x3, 0xff100000},
	{0x0c, 0x0C, 0x0, 0xff010000},
	{0x0c, 0x0D, 0x0, 0xfdf30000},
	{0x0c, 0x0E, 0x0, 0xff390000},
	{0x0c, 0x0F, 0x0, 0xe82b0000},
	{0x0c, 0x10, 0x0, 0xe82b8000},
	{0x0c, 0x11, 0x0, 0xf0800000},
	{0x0c, 0x11, 0x1, 0xf0000000},
	{0x0c, 0x12, 0x0, 0xffd00000},
	{0x0c, 0x14, 0x0, 0xff37c000},
	{0x0c, 0x15, 0x0, 0xe0000000},
	{0x0c, 0x16, 0x0, 0xc0000000},
	{0x0c, 0x16, 0x1, 0x80000000},
	{0x0c, 0x16, 0x2, 0x0},
	{0x0c, 0x17, 0x0, 0xea000000},
	{0x0c, 0x17, 0x1, 0x0},
	{0x0c, 0x18, 0x0, 0xe8a00000},
	{0x0c, 0x19, 0x0, 0xe8800000},
	{0x0c, 0x1A, 0x0, 0xfdf20000},
	{0x0c, 0x1B, 0x0, 0xfdf00000},
	{0x0c, 0x1C, 0x0, 0xfe000000},
	{0x0c, 0x1D, 0x0, 0xe9840000},
	{0x00, 0x00, 0x0, 0xe9830000},
	{0x00, 0x01, 0x0, 0xe9800000},
	{0x00, 0x02, 0x0, 0xe9870000},
	{0x00, 0x03, 0x0, 0xe9860000},
	{0x00, 0x04, 0x0, 0xe9890000},
	{0x00, 0x05, 0x0, 0xe9880000},
	{0x00, 0x06, 0x0, 0xe9820000},
	{0x00, 0x07, 0x0, 0xea840000},
	{0x00, 0x07, 0x1, 0xea800000},
	{0x00, 0x08, 0x0, 0xe8000000},
	{0x00, 0x09, 0x0, 0xe8100000},
	{0x00, 0x0A, 0x0, 0xe8400000},
	{0x00, 0x0B, 0x0, 0xff200000},
	{0x00, 0x0B, 0x1, 0xff030000},
	{0x00, 0x0B, 0x2, 0xff040000},
	{0x00, 0x0B, 0x3, 0xff100000},
	{0x00, 0x0C, 0x0, 0xff010000},
	{0x00, 0x0D, 0x0, 0xfdf30000},
	{0x00, 0x0E, 0x0, 0xff390000},
	{0x00, 0x0F, 0x0, 0xe82b0000},
	{0x00, 0x10, 0x0, 0xe82b8000},
	{0x00, 0x11, 0x0, 0xf0800000},
	{0x00, 0x11, 0x1, 0xf0000000},
	{0x00, 0x12, 0x0, 0xffd00000},
	{0x00, 0x13, 0x0, 0xffe00000},
	{0x00, 0x13, 0x1, 0xed000000},
	{0x00, 0x13, 0x2, 0xec000000},
	{0x00, 0x14, 0x0, 0xff37c000},
	{0x00, 0x15, 0x0, 0xe0000000},
	{0x00, 0x16, 0x0, 0xc0000000},
	{0x00, 0x16, 0x1, 0x80000000},
	{0x00, 0x16, 0x2, 0x0},
	{0x00, 0x17, 0x0, 0xea000000},
	{0x00, 0x17, 0x1, 0x0},
	{0x00, 0x18, 0x0, 0xe8a00000},
	{0x00, 0x19, 0x0, 0xe8800000},
	{0x00, 0x1A, 0x0, 0xfdf20000},
	{0x00, 0x1B, 0x0, 0xfdf00000},
	{0x00, 0x1C, 0x0, 0xfe000000},
	{0x00, 0x1D, 0x0, 0xe9840000},
	{0x01, 0x00, 0x0, 0xe9830000},
	{0x01, 0x01, 0x0, 0xe9800000},
	{0x01, 0x02, 0x0, 0xe9870000},
	{0x01, 0x03, 0x0, 0xe9860000},
	{0x01, 0x04, 0x0, 0xe9890000},
	{0x01, 0x05, 0x0, 0xe9880000},
	{0x01, 0x06, 0x0, 0xe9820000},
	{0x01, 0x07, 0x0, 0xea840000},
	{0x01, 0x07, 0x1, 0xea800000},
	{0x01, 0x08, 0x0, 0xe8000000},
	{0x01, 0x09, 0x0, 0xe8100000},
	{0x01, 0x0A, 0x0, 0xe8400000},
	{0x01, 0x0B, 0x0, 0xff200000},
	{0x01, 0x0B, 0x1, 0xff030000},
	{0x01, 0x0B, 0x2, 0xff040000},
	{0x01, 0x0B, 0x3, 0xff100000},
	{0x01, 0x0C, 0x0, 0xff010000},
	{0x01, 0x0D, 0x0, 0xfdf30000},
	{0x01, 0x0E, 0x0, 0xff390000},
	{0x01, 0x0F, 0x0, 0xe82b0000},
	{0x01, 0x10, 0x0, 0xe82b8000},
	{0x01, 0x11, 0x0, 0xf0800000},
	{0x01, 0x11, 0x1, 0xf0000000},
	{0x01, 0x12, 0x0, 0xffd00000},
	{0x01, 0x13, 0x0, 0xffe00000},
	{0x01, 0x13, 0x1, 0xed000000},
	{0x01, 0x13, 0x2, 0xec000000},
	{0x01, 0x14, 0x0, 0xff37c000},
	{0x01, 0x15, 0x0, 0xe0000000},
	{0x01, 0x16, 0x0, 0xc0000000},
	{0x01, 0x16, 0x1, 0x80000000},
	{0x01, 0x16, 0x2, 0x0},
	{0x01, 0x17, 0x0, 0xea000000},
	{0x01, 0x17, 0x1, 0x0},
	{0x01, 0x18, 0x0, 0xe8a00000},
	{0x01, 0x19, 0x0, 0xe8800000},
	{0x01, 0x1A, 0x0, 0xfdf20000},
	{0x01, 0x1B, 0x0, 0xfdf00000},
	{0x01, 0x1C, 0x0, 0xfe000000},
	{0x01, 0x1D, 0x0, 0xe9840000},
	{0x0D, 0x00, 0x0, 0xe9830000},
	{0x0D, 0x01, 0x0, 0xe9800000},
	{0x0D, 0x02, 0x0, 0xe9870000},
	{0x0D, 0x03, 0x0, 0xe9860000},
	{0x0D, 0x04, 0x0, 0xe9890000},
	{0x0D, 0x05, 0x0, 0xe9880000},
	{0x0D, 0x06, 0x0, 0xe9820000},
	{0x0D, 0x07, 0x0, 0xea840000},
	{0x0D, 0x07, 0x1, 0xea800000},
	{0x0D, 0x08, 0x0, 0xe8000000},
	{0x0D, 0x09, 0x0, 0xe8100000},
	{0x0D, 0x0A, 0x0, 0xe8400000},
	{0x0D, 0x0B, 0x0, 0xff200000},
	{0x0D, 0x0B, 0x1, 0xff030000},
	{0x0D, 0x0B, 0x2, 0xff040000},
	{0x0D, 0x0B, 0x3, 0xff100000},
	{0x0D, 0x0C, 0x0, 0xff010000},
	{0x0D, 0x0D, 0x0, 0xfdf30000},
	{0x0D, 0x0E, 0x0, 0xff390000},
	{0x0D, 0x0F, 0x0, 0xe82b0000},
	{0x0D, 0x10, 0x0, 0xe82b8000},
	{0x0D, 0x11, 0x0, 0xf0800000},
	{0x0D, 0x11, 0x1, 0xf0000000},
	{0x0D, 0x12, 0x0, 0xffd00000},
	{0x0D, 0x13, 0x0, 0xffe00000},
	{0x0D, 0x13, 0x1, 0xed000000},
	{0x0D, 0x13, 0x2, 0xec000000},
	{0x0D, 0x14, 0x0, 0xff37c000},
	{0x0D, 0x16, 0x0, 0xc0000000},
	{0x0D, 0x16, 0x1, 0x80000000},
	{0x0D, 0x16, 0x2, 0x0},
	{0x0D, 0x17, 0x0, 0xea000000},
	{0x0D, 0x17, 0x1, 0x0},
	{0x0D, 0x18, 0x0, 0xe8a00000},
	{0x0D, 0x19, 0x0, 0xe8800000},
	{0x0D, 0x1A, 0x0, 0xfdf20000},
	{0x0D, 0x1B, 0x0, 0xfdf00000},
	{0x0D, 0x1C, 0x0, 0xfe000000},
	{0x0D, 0x1D, 0x0, 0xe9840000},
	{0x0E, 0x00, 0x0, 0xe9830000},
	{0x0E, 0x01, 0x0, 0xe9800000},
	{0x0E, 0x02, 0x0, 0xe9870000},
	{0x0E, 0x03, 0x0, 0xe9860000},
	{0x0E, 0x04, 0x0, 0xe9890000},
	{0x0E, 0x05, 0x0, 0xe9880000},
	{0x0E, 0x06, 0x0, 0xe9820000},
	{0x0E, 0x07, 0x0, 0xea840000},
	{0x0E, 0x07, 0x1, 0xea800000},
	{0x0E, 0x08, 0x0, 0xe8000000},
	{0x0E, 0x09, 0x0, 0xe8100000},
	{0x0E, 0x0A, 0x0, 0xe8400000},
	{0x0E, 0x0B, 0x0, 0xff200000},
	{0x0E, 0x0B, 0x1, 0xff030000},
	{0x0E, 0x0B, 0x2, 0xff040000},
	{0x0E, 0x0B, 0x3, 0xff100000},
	{0x0E, 0x0C, 0x0, 0xff010000},
	{0x0E, 0x0D, 0x0, 0xfdf30000},
	{0x0E, 0x0E, 0x0, 0xff390000},
	{0x0E, 0x0F, 0x0, 0xe82b0000},
	{0x0E, 0x10, 0x0, 0xe82b8000},
	{0x0E, 0x11, 0x0, 0xf0800000},
	{0x0E, 0x11, 0x1, 0xf0000000},
	{0x0E, 0x12, 0x0, 0xffd00000},
	{0x0E, 0x13, 0x0, 0xffe00000},
	{0x0E, 0x13, 0x1, 0xed000000},
	{0x0E, 0x13, 0x2, 0xec000000},
	{0x0E, 0x14, 0x0, 0xff37c000},
	{0x0E, 0x15, 0x0, 0xe0000000},
	{0x0E, 0x16, 0x0, 0xc0000000},
	{0x0E, 0x16, 0x1, 0x80000000},
	{0x0E, 0x16, 0x2, 0x0},
	{0x0E, 0x17, 0x0, 0xea000000},
	{0x0E, 0x17, 0x1, 0x0},
	{0x0E, 0x18, 0x0, 0xe8a00000},
	{0x0E, 0x19, 0x0, 0xe8800000},
	{0x0E, 0x1A, 0x0, 0xfdf20000},
	{0x0E, 0x1B, 0x0, 0xfdf00000},
	{0x0E, 0x1C, 0x0, 0xfe000000},
	{0x0E, 0x1D, 0x0, 0xe9840000},
	{0x0F, 0x00, 0x0, 0xe9830000},
	{0x0F, 0x01, 0x0, 0xe9800000},
	{0x0F, 0x02, 0x0, 0xe9870000},
	{0x0F, 0x03, 0x0, 0xe9860000},
	{0x0F, 0x04, 0x0, 0xe9890000},
	{0x0F, 0x05, 0x0, 0xe9880000},
	{0x0F, 0x06, 0x0, 0xe9820000},
	{0x0F, 0x07, 0x0, 0xea840000},
	{0x0F, 0x07, 0x1, 0xea800000},
	{0x0F, 0x08, 0x0, 0xe8000000},
	{0x0F, 0x09, 0x0, 0xe8100000},
	{0x0F, 0x0A, 0x0, 0xe8400000},
	{0x0F, 0x0B, 0x0, 0xff200000},
	{0x0F, 0x0B, 0x1, 0xff030000},
	{0x0F, 0x0B, 0x2, 0xff040000},
	{0x0F, 0x0B, 0x3, 0xff100000},
	{0x0F, 0x0C, 0x0, 0xff010000},
	{0x0F, 0x0D, 0x0, 0xfdf30000},
	{0x0F, 0x0E, 0x0, 0xff390000},
	{0x0F, 0x0F, 0x0, 0xe82b0000},
	{0x0F, 0x10, 0x0, 0xe82b8000},
	{0x0F, 0x11, 0x0, 0xf0800000},
	{0x0F, 0x11, 0x1, 0xf0000000},
	{0x0F, 0x12, 0x0, 0xffd00000},
	{0x0F, 0x13, 0x0, 0xffe00000},
	{0x0F, 0x13, 0x1, 0xed000000},
	{0x0F, 0x13, 0x2, 0xec000000},
	{0x0F, 0x14, 0x0, 0xff37c000},
	{0x0F, 0x15, 0x0, 0xe0000000},
	{0x0F, 0x16, 0x0, 0xc0000000},
	{0x0F, 0x16, 0x1, 0x80000000},
	{0x0F, 0x16, 0x2, 0x0},
	{0x0F, 0x17, 0x0, 0xea000000},
	{0x0F, 0x17, 0x1, 0x0},
	{0x0F, 0x18, 0x0, 0xe8a00000},
	{0x0F, 0x19, 0x0, 0xe8800000},
	{0x0F, 0x1A, 0x0, 0xfdf20000},
	{0x0F, 0x1B, 0x0, 0xfdf00000},
	{0x0F, 0x1C, 0x0, 0xfe000000},
	{0x0F, 0x1D, 0x0, 0xe9840000},
	{0x10, 0x00, 0x0, 0xe9830000},
	{0x10, 0x01, 0x0, 0xe9800000},
	{0x10, 0x02, 0x0, 0xe9870000},
	{0x10, 0x03, 0x0, 0xe9860000},
	{0x10, 0x04, 0x0, 0xe9890000},
	{0x10, 0x05, 0x0, 0xe9880000},
	{0x10, 0x06, 0x0, 0xe9820000},
	{0x10, 0x07, 0x0, 0xea840000},
	{0x10, 0x07, 0x1, 0xea800000},
	{0x10, 0x08, 0x0, 0xe8000000},
	{0x10, 0x09, 0x0, 0xe8100000},
	{0x10, 0x0A, 0x0, 0xe8400000},
	{0x10, 0x0B, 0x0, 0xff200000},
	{0x10, 0x0B, 0x1, 0xff030000},
	{0x10, 0x0B, 0x2, 0xff040000},
	{0x10, 0x0B, 0x3, 0xff100000},
	{0x10, 0x0C, 0x0, 0xff010000},
	{0x10, 0x0D, 0x0, 0xfdf30000},
	{0x10, 0x0E, 0x0, 0xff390000},
	{0x10, 0x0F, 0x0, 0xe82b0000},
	{0x10, 0x10, 0x0, 0xe82b8000},
	{0x10, 0x11, 0x0, 0xf0800000},
	{0x10, 0x11, 0x1, 0xf0000000},
	{0x10, 0x12, 0x0, 0xffd00000},
	{0x10, 0x13, 0x0, 0xffe00000},
	{0x10, 0x13, 0x1, 0xed000000},
	{0x10, 0x13, 0x2, 0xec000000},
	{0x10, 0x14, 0x0, 0xff37c000},
	{0x10, 0x15, 0x0, 0xe0000000},
	{0x10, 0x16, 0x0, 0xc0000000},
	{0x10, 0x16, 0x1, 0x80000000},
	{0x10, 0x16, 0x2, 0x0},
	{0x10, 0x17, 0x0, 0xea000000},
	{0x10, 0x17, 0x1, 0x0},
	{0x10, 0x18, 0x0, 0xe8a00000},
	{0x10, 0x19, 0x0, 0xe8800000},
	{0x10, 0x1A, 0x0, 0xfdf20000},
	{0x10, 0x1B, 0x0, 0xfdf00000},
	{0x10, 0x1C, 0x0, 0xfe000000},
	{0x10, 0x1D, 0x0, 0xe9840000},
	{0x11, 0x00, 0x0, 0xe9830000},
	{0x11, 0x01, 0x0, 0xe9800000},
	{0x11, 0x02, 0x0, 0xe9870000},
	{0x11, 0x03, 0x0, 0xe9860000},
	{0x11, 0x04, 0x0, 0xe9890000},
	{0x11, 0x05, 0x0, 0xe9880000},
	{0x11, 0x06, 0x0, 0xe9820000},
	{0x11, 0x07, 0x0, 0xea840000},
	{0x11, 0x07, 0x1, 0xea800000},
	{0x11, 0x08, 0x0, 0xe8000000},
	{0x11, 0x09, 0x0, 0xe8100000},
	{0x11, 0x0A, 0x0, 0xe8400000},
	{0x11, 0x0B, 0x0, 0xff200000},
	{0x11, 0x0B, 0x1, 0xff030000},
	{0x11, 0x0B, 0x2, 0xff040000},
	{0x11, 0x0B, 0x3, 0xff100000},
	{0x11, 0x0C, 0x0, 0xff010000},
	{0x11, 0x0D, 0x0, 0xfdf30000},
	{0x11, 0x0E, 0x0, 0xff390000},
	{0x11, 0x0F, 0x0, 0xe82b0000},
	{0x11, 0x10, 0x0, 0xe82b8000},
	{0x11, 0x11, 0x0, 0xf0800000},
	{0x11, 0x11, 0x1, 0xf0000000},
	{0x11, 0x12, 0x0, 0xffd00000},
	{0x11, 0x13, 0x0, 0xffe00000},
	{0x11, 0x13, 0x1, 0xed000000},
	{0x11, 0x13, 0x2, 0xec000000},
	{0x11, 0x14, 0x0, 0xff37c000},
	{0x11, 0x15, 0x0, 0xe0000000},
	{0x11, 0x16, 0x0, 0xc0000000},
	{0x11, 0x16, 0x1, 0x80000000},
	{0x11, 0x16, 0x2, 0x0},
	{0x11, 0x17, 0x0, 0xea000000},
	{0x11, 0x17, 0x1, 0x0},
	{0x11, 0x18, 0x0, 0xe8a00000},
	{0x11, 0x19, 0x0, 0xe8800000},
	{0x11, 0x1A, 0x0, 0xfdf20000},
	{0x11, 0x1B, 0x0, 0xfdf00000},
	{0x11, 0x1C, 0x0, 0xfe000000},
	{0x11, 0x1D, 0x0, 0xe9840000},
	{0x12, 0x00, 0x0, 0xe9830000},
	{0x12, 0x01, 0x0, 0xe9800000},
	{0x12, 0x02, 0x0, 0xe9870000},
	{0x12, 0x03, 0x0, 0xe9860000},
	{0x12, 0x04, 0x0, 0xe9890000},
	{0x12, 0x05, 0x0, 0xe9880000},
	{0x12, 0x06, 0x0, 0xe9820000},
	{0x12, 0x07, 0x0, 0xea840000},
	{0x12, 0x07, 0x1, 0xea800000},
	{0x12, 0x08, 0x0, 0xe8000000},
	{0x12, 0x09, 0x0, 0xe8100000},
	{0x12, 0x0A, 0x0, 0xe8400000},
	{0x12, 0x0B, 0x0, 0xff200000},
	{0x12, 0x0B, 0x1, 0xff030000},
	{0x12, 0x0B, 0x2, 0xff040000},
	{0x12, 0x0B, 0x3, 0xff100000},
	{0x12, 0x0C, 0x0, 0xff010000},
	{0x12, 0x0D, 0x0, 0xfdf30000},
	{0x12, 0x0E, 0x0, 0xff390000},
	{0x12, 0x0F, 0x0, 0xe82b0000},
	{0x12, 0x10, 0x0, 0xe82b8000},
	{0x12, 0x11, 0x0, 0xf0800000},
	{0x12, 0x11, 0x1, 0xf0000000},
	{0x12, 0x12, 0x0, 0xffd00000},
	{0x12, 0x13, 0x0, 0xffe00000},
	{0x12, 0x13, 0x1, 0xed000000},
	{0x12, 0x13, 0x2, 0xec000000},
	{0x12, 0x14, 0x0, 0xff37c000},
	{0x12, 0x15, 0x0, 0xe0000000},
	{0x12, 0x16, 0x0, 0xc0000000},
	{0x12, 0x16, 0x1, 0x80000000},
	{0x12, 0x16, 0x2, 0x0},
	{0x12, 0x17, 0x0, 0xea000000},
	{0x12, 0x17, 0x1, 0x0},
	{0x12, 0x18, 0x0, 0xe8a00000},
	{0x12, 0x19, 0x0, 0xe8800000},
	{0x12, 0x1A, 0x0, 0xfdf20000},
	{0x12, 0x1B, 0x0, 0xfdf00000},
	{0x12, 0x1C, 0x0, 0xfe000000},
	{0x12, 0x1D, 0x0, 0xe9840000},
	{0x13, 0x00, 0x0, 0xe9830000},
	{0x13, 0x01, 0x0, 0xe9800000},
	{0x13, 0x02, 0x0, 0xe9870000},
	{0x13, 0x03, 0x0, 0xe9860000},
	{0x13, 0x04, 0x0, 0xe9890000},
	{0x13, 0x05, 0x0, 0xe9880000},
	{0x13, 0x06, 0x0, 0xe9820000},
	{0x13, 0x07, 0x0, 0xea840000},
	{0x13, 0x07, 0x1, 0xea800000},
	{0x13, 0x08, 0x0, 0xe8000000},
	{0x13, 0x09, 0x0, 0xe8100000},
	{0x13, 0x0A, 0x0, 0xe8400000},
	{0x13, 0x0B, 0x0, 0xff200000},
	{0x13, 0x0B, 0x1, 0xff030000},
	{0x13, 0x0B, 0x2, 0xff040000},
	{0x13, 0x0B, 0x3, 0xff100000},
	{0x13, 0x0C, 0x0, 0xff010000},
	{0x13, 0x0D, 0x0, 0xfdf30000},
	{0x13, 0x0E, 0x0, 0xff390000},
	{0x13, 0x0F, 0x0, 0xe82b0000},
	{0x13, 0x10, 0x0, 0xe82b8000},
	{0x13, 0x11, 0x0, 0xf0800000},
	{0x13, 0x11, 0x1, 0xf0000000},
	{0x13, 0x12, 0x0, 0xffd00000},
	{0x13, 0x13, 0x0, 0xffe00000},
	{0x13, 0x13, 0x1, 0xed000000},
	{0x13, 0x13, 0x2, 0xec000000},
	{0x13, 0x14, 0x0, 0xff37c000},
	{0x13, 0x15, 0x0, 0xe0000000},
	{0x13, 0x16, 0x0, 0xc0000000},
	{0x13, 0x16, 0x1, 0x80000000},
	{0x13, 0x16, 0x2, 0x0},
	{0x13, 0x17, 0x0, 0xea000000},
	{0x13, 0x17, 0x1, 0x0},
	{0x13, 0x18, 0x0, 0xe8a00000},
	{0x13, 0x19, 0x0, 0xe8800000},
	{0x13, 0x1A, 0x0, 0xfdf20000},
	{0x13, 0x1B, 0x0, 0xfdf00000},
	{0x13, 0x1C, 0x0, 0xfe000000},
	{0x13, 0x1D, 0x0, 0xe9840000},
};

/* vivo_bus */
static const ROUTE_ID_ADDR_STRU vivo_routeid_addr_tbl[] = {
	/* Init_flow  Targ_flow  Targ_subrange Init_localAddress */
	/* ----------------------------------------------------- */
	{0x00, 0x04, 0x0, 0x0},
	{0x01, 0x05, 0x0, 0x0},
	{0x02, 0x04, 0x0, 0x0},
	{0x03, 0x06, 0x0, 0x0},
	{0x04, 0x07, 0x0, 0x0},
	{0x05, 0x06, 0x0, 0x0},
	{0x06, 0x05, 0x0, 0x0},
	{0x07, 0x00, 0x0, 0xe86c0000},
	{0x07, 0x00, 0x1, 0x0},
	{0x07, 0x01, 0x0, 0xe86d0000},
	{0x07, 0x02, 0x0, 0xe8600000},
	{0x07, 0x03, 0x0, 0xe8400000},
};

/* vcodec_bus */
static const ROUTE_ID_ADDR_STRU vcodec_routeid_addr_tbl[] = {
	/* Init_flow  Targ_flow  Targ_subrange : Init_mapping ,
	Init_localAddress , Targ_mapping , Targ_localAddress */
	/* ---------------------------------------------------*/
	{0x00, 0x01, 0x0, 0xe8800000},
	{0x00, 0x02, 0x0, 0xe8900000},
	{0x00, 0x03, 0x0, 0xe8920000},
	{0x00, 0x03, 0x1, 0x0},
	{0x00, 0x04, 0x0, 0xe8930000},
	{0x00, 0x05, 0x0, 0xe8940000},
	{0x01, 0x00, 0x0, 0x0},
	{0x02, 0x00, 0x0, 0x0},
};

struct noc_mid_info noc_mid_hi6250[] = {
	/* Bus ID,    init_flow   ,mask       ,mid_val        ,mid name */
	{0, 13, 0x1f, 0x12, "MCPU"},    /* noc_modem_mst, */
	{0, 13, 0x1f, 0x13, "TL_BBE16"},    /* noc_modem_mst, */
	{0, 13, 0x1f, 0x02, "CBBE16"},  /* noc_modem_mst, */
	{0, 13, 0x1f, 0x14, "BBP_DMA_0"},   /* noc_modem_mst, */
	{0, 13, 0x1f, 0x15, "BBP_DMA_1"},   /* noc_modem_mst, */
	{0, 13, 0x1f, 0x16, "GU_BBP_MST"},  /* noc_modem_mst, */
	{0, 13, 0x1f, 0x17, "GU_BBP_MST"},  /* noc_modem_mst, */
	{0, 13, 0x1f, 0x19, "EDMA"},    /* noc_modem_mst, */
	{0, 13, 0x1f, 0x18, "EDMA1"},   /* noc_modem_mst, */
	{0, 13, 0x1f, 0x1A, "CIPHER"},  /* noc_modem_mst, */
	{0, 13, 0x1f, 0x1C, "UPACC"},   /* noc_modem_mst, */
	{0, 13, 0x1f, 0x1D, "CICOM_0"}, /* noc_modem_mst, */
	{0, 13, 0x1f, 0x1E, "HDLC"},    /* noc_modem_mst, */
};

struct noc_sec_info noc_sec_hi6250[] = {
	/*mask value info  sec_mode*/
	{0x01, 0x00, "secure", "secure"},/*secure*/
	{0x01, 0x01, "non-secure", "secure"},/*secure*/
};

struct noc_dump_reg noc_dump_reg_list_hi6250[] = {
	{"PCTRL", (void *)NULL, 0x050},//PCTRLPERI_CTRL19
	{"PCTRL", (void *)NULL, 0x09c},//PCTRLPERI_STAT2},
	{"PMCTRL", (void *)NULL, 0x234},//PMCTRL_NOC_POWER_IDLEREQ},
	{"PMCTRL", (void *)NULL, 0x380},//PMCTRL_NOC_POWER_IDLEACK},
	{"PMCTRL", (void *)NULL, 0x384},//PMCTRL_NOC_POWER_IDLE},
	{"PMCTRL", (void *)NULL, 0x388},//PMCTRL_PERI_INT0_STAT},
	{"SCTRL", (void *)NULL, 0x3A4},//SCTRL_SCPERSTATUS6},
};

/*
 * if noc_happend's initflow is in the hisi_modemnoc_initflow,
 * firstly save log, then system reset.
 */
const struct noc_busid_initflow hisi_filter_initflow_hi6250[] = {
	/* Bus ID,    init_flow, Core-Id */
	{0, 11, RDR_CP},	/*ipf*/
	{0, 17, RDR_CP},	/*socp*/
	{0, 8, RDR_IOM3},
	{0, 9, RDR_IOM3},
	{0, 10, RDR_IOM3},
	{0, 3, RDR_HIFI},
	{ARRAY_END_FLAG, 0, RDR_AP},	/*end*/
};

const struct noc_bus_info noc_buses_info_hi6250[] = {
	[0] = {
		.name = "cfg_sys_noc_bus",
		.initflow_mask = ((0x1f) << 16),
		.targetflow_mask = ((0x1f) << 11),
		.targ_subrange_mask = ((0x3) << 9),
		.seq_id_mask = (0x1FF),
		.opc_mask = ((0xf) << 1),
		.err_code_mask = ((0x7) << 8),
		.opc_array = opc_array,
		.opc_array_size = OPC_NR,
		.err_code_array = err_code_array,
		.err_code_array_size = ERR_CODE_NR,
		.initflow_array = cfg_initflow_array,
		.initflow_array_size = CFG_INITFLOW_ARRAY_SIZE,
		.targetflow_array = cfg_targetflow_array,
		.targetflow_array_size = CFG_TARGETFLOW_ARRAY_SIZE,
		.routeid_tbl = cfgsys_routeid_addr_tbl,
		.routeid_tbl_size = ARRAY_SIZE_NOC(cfgsys_routeid_addr_tbl),
		.p_noc_mid_info = noc_mid_hi6250,
		.noc_mid_info_size = ARRAY_SIZE_NOC(noc_mid_hi6250),
		.p_noc_sec_info = noc_sec_hi6250,
		.noc_sec_info_size = ARRAY_SIZE_NOC(noc_sec_hi6250),
	},
	[1] = {
		.name = "vcodec_bus",
		.initflow_mask = ((0x3) << 12),
		.targetflow_mask = ((0x7) << 9),
		.targ_subrange_mask = ((0x1) << 8),
		.seq_id_mask = (0xFF),
		.opc_mask = ((0xf) << 1),
		.err_code_mask = ((0x7) << 8),
		.opc_array = opc_array,
		.opc_array_size = OPC_NR,
		.err_code_array = err_code_array,
		.err_code_array_size = ERR_CODE_NR,
		.initflow_array = vcodec_initflow_array,
		.initflow_array_size = VCODEC_INITFLOW_ARRAY_SIZE,
		.targetflow_array = vcodec_targetflow_array,
		.targetflow_array_size = VCODEC_TARGETFLOW_ARRAY_SIZE,
		.routeid_tbl = vcodec_routeid_addr_tbl,
		.routeid_tbl_size = ARRAY_SIZE_NOC(vcodec_routeid_addr_tbl),
		.p_noc_mid_info = noc_mid_hi6250,
		.noc_mid_info_size = ARRAY_SIZE_NOC(noc_mid_hi6250),
		.p_noc_sec_info = noc_sec_hi6250,
		.noc_sec_info_size = ARRAY_SIZE_NOC(noc_sec_hi6250),
	},
	[2] = {
		.name = "vivo_bus",
		.initflow_mask = ((0x7) << 12),
		.targetflow_mask = ((0x7) << 9),
		.targ_subrange_mask = ((0x1) << 8),
		.seq_id_mask = (0xFF),
		.opc_mask = ((0xf) << 1),
		.err_code_mask = ((0x7) << 8),
		.opc_array = opc_array,
		.opc_array_size = OPC_NR,
		.err_code_array = err_code_array,
		.err_code_array_size = ERR_CODE_NR,
		.initflow_array = vivo_initflow_array,
		.initflow_array_size = VIVO_INITFLOW_ARRAY_SIZE,
		.targetflow_array = vivo_targetflow_array,
		.targetflow_array_size = VIVO_TARGETFLOW_ARRAY_SIZE,
		.routeid_tbl = vivo_routeid_addr_tbl,
		.routeid_tbl_size = ARRAY_SIZE_NOC(vivo_routeid_addr_tbl),
		.p_noc_mid_info = noc_mid_hi6250,
		.noc_mid_info_size = ARRAY_SIZE_NOC(noc_mid_hi6250),
		.p_noc_sec_info = noc_sec_hi6250,
		.noc_sec_info_size = ARRAY_SIZE_NOC(noc_sec_hi6250),
	}
};

/* hisi_noc_get_array_size - get static array size
 * @bus_info_size : bus info array size
 * @dump_list_size: dump list array size
 */
void hisi_noc_get_array_size_hi6250(unsigned int *bus_info_size, unsigned int *dump_list_size)
{
	if ((NULL == bus_info_size)||(NULL == dump_list_size))
		return;

	*bus_info_size  = ARRAY_SIZE_NOC(noc_buses_info_hi6250);
	*dump_list_size = ARRAY_SIZE_NOC(noc_dump_reg_list_hi6250);
}

/* hisi_noc_clock_enable - check noc clock state : on or off
 * @noc_dev : hisi noc device poiter
 * @node: hisi noc node poiter
 *
 * If clock enable, return 1, else return 0;
 */
unsigned int hisi_noc_clock_enable_hi6250(struct hisi_noc_device *noc_dev,
				     struct noc_node *node)
{
    void __iomem *clock_reg = NULL;
	unsigned int reg_value;
	unsigned int i;
	unsigned int ret = 1;

	if ((NULL == noc_dev)||(NULL == node))
		return 0;

    for (i = 0; i < HISI_NOC_CLOCK_MAX; i++) {
		if (0xFFFFFFFF == node->crg_clk[i].offset)
			continue;

		clock_reg =
		    (void __iomem *)((unsigned long)noc_dev->pcrgctrl_base +
					node->crg_clk[i].offset);
		reg_value = readl_relaxed(clock_reg);

		/* Clock is enabled */
		if (reg_value & (1U << node->crg_clk[i].mask_bit))
			continue;
		else {
			ret = 0;
			break;
		}
	}

	if (noc_dev->noc_property->noc_debug)
		pr_err("%s: clock_reg = 0x%pK\n", __func__, clock_reg);

    return ret;
}
