/*
 * Copyright (c) 2022-2022 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "lwip.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "ethernetif.h"
#include "lwip/timeouts.h"
#include "lwip/tcp.h"
#include "lwip/ip4_frag.h"
#include "lwip/tcpip.h"
#include "usart.h"
#include <stdio.h>
#include "prt_module.h"
#include "prt_task.h"
#include "lwip/priv/tcp_priv.h"
#include "lwipopts.h"
#include "lwip/netifapi.h"
#include "stm32f4xx.h"
#include "os_cpu_armv7_m_external.h"
#include "prt_hwi.h"
#include "lan8720.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"

#define NETIF_SETUP_OVERTIME       100

struct lwipDev lwipdev;
struct netif lwip_netif;
TskHandle lwipDhcpTaskId;

#if LWIP_DHCP
void LwipDhcpDelete(void)
{
	U32 ret;
	dhcp_stop(&lwip_netif);
	ret = PRT_TaskDelete(lwipDhcpTaskId);
	if (ret != OS_OK) {
		printf("LwipDhcpDelete failed!!!\n\r");
	}
}

void LwipDhcpTask(void *pdata)
{
	uintptr_t intSave = PRT_HwiLock();

	U32 ip = 0; /* 0, initial value */
	U32 netmask = 0; /* 0, initial value */
	U32 gw = 0; /* 0, initial value */

	dhcp_start(&lwip_netif);

	while(1) {
		ip = lwip_netif.ip_addr.addr;
		netmask	= lwip_netif.netmask.addr;
		gw = lwip_netif.gw.addr;
		struct dhcp *dhcp = netif_dhcp_data(&lwip_netif);

		if(ip != 0) {
			lwipdev.ip[3] = (uint8_t)(ip >> 24);
			lwipdev.ip[2] = (uint8_t)(ip >> 16);
			lwipdev.ip[1] = (uint8_t)(ip >> 8);
			lwipdev.ip[0] = (uint8_t)(ip);
			lwipdev.netmask[3] = (uint8_t)(netmask >> 24);
			lwipdev.netmask[2] = (uint8_t)(netmask >> 16);
			lwipdev.netmask[1] = (uint8_t)(netmask >> 8);
			lwipdev.netmask[0] = (uint8_t)(netmask);
			lwipdev.gateway[3] = (uint8_t)(gw >> 24);
			lwipdev.gateway[2] = (uint8_t)(gw >> 16);
			lwipdev.gateway[1] = (uint8_t)(gw >> 8);
			lwipdev.gateway[0] = (uint8_t)(gw);
			break;
		} else if (dhcp->tries > LWIP_MAX_DHCP_TRIES) {
			IP4_ADDR(&(lwip_netif.ip_addr), lwipdev.ip[0],
					 lwipdev.ip[1], lwipdev.ip[2], lwipdev.ip[3]);
			IP4_ADDR(&(lwip_netif.netmask), lwipdev.netmask[0],
					 lwipdev.netmask[1], lwipdev.netmask[2],lwipdev.netmask[3]);
			IP4_ADDR(&(lwip_netif.gw), lwipdev.gateway[0], lwipdev.gateway[1],
					 lwipdev.gateway[2], lwipdev.gateway[3]);
			break;
		}
	}
	LwipDhcpDelete();
	PRT_HwiRestore(intSave);
}

U32 LwipDhcpCreate(void)
{
    U32 ret;
    struct TskInitParam taskParam;

    taskParam.taskEntry = (TskEntryFunc)LwipDhcpTask;
    taskParam.stackSize = 0x800;
    taskParam.name = "lwipDhcpTask";
    taskParam.taskPrio = OS_TSK_PRIORITY_06;
    taskParam.stackAddr = 0;

    ret = PRT_TaskCreate(&lwipDhcpTaskId, &taskParam);
    if (ret != OS_OK) {
        return ret;
    }

    ret = PRT_TaskResume(lwipDhcpTaskId);
    if (ret != OS_OK) {
        return ret;
    }

    return OS_OK;
}
#endif	/* #if LWIP_DHCP */

void LwipPktHandle(void)
{
	EthernetifInput(&lwip_netif);
}

void LwipDefaultIpSet(struct lwipDev *lwipx)
{
	u32 sn = (*(u32*)(0x1FFF7A10));

	lwipx->remoteip[0] = 192;
	lwipx->remoteip[1] = 168;
	lwipx->remoteip[2] = 1;
	lwipx->remoteip[3] = 11;

	lwipx->mac[0] = 2;
	lwipx->mac[1] = 0;
	lwipx->mac[2] = 0;
	lwipx->mac[3] = (sn >> 16) & 0XFF;
	lwipx->mac[4] = (sn >> 8) & 0XFFF;;
	lwipx->mac[5] = sn & 0XFF;

	lwipx->ip[0] = 192;
	lwipx->ip[1] = 168;
	lwipx->ip[2] = 1;
	lwipx->ip[3] = 30;

	lwipx->netmask[0] = 255;
	lwipx->netmask[1] = 255;
	lwipx->netmask[2] = 255;
	lwipx->netmask[3] = 0;

	lwipx->gateway[0] = 192;
	lwipx->gateway[1] = 168;
	lwipx->gateway[2] = 1;
	lwipx->gateway[3] = 1;
}

u8 LwipInit(void)
{
	printf("	LwipInit:\n\r");
	u8 res;
	uintptr_t intSave;
	struct netif *netifInitFlag;
	static uint32_t overtime = 0;
	ip4_addr_t ipaddr;
	ip4_addr_t netmask;
	ip4_addr_t gw;

	res = EthMemMalloc();
	if(res != 0) {
		printf("		ErhMemMalloc failed\n\r");
		return 1;
	}

	res = LAN8720Init();
	if(res != 0) {
		printf("		LAN8720Init failed\n\r");
		return 2;
	}

	tcpip_init(NULL,NULL);

	LwipDefaultIpSet(&lwipdev);

#if LWIP_DHCP
	ip4_addr_set_zero(&gw);
	ip4_addr_set_zero(&ipaddr);
	ip4_addr_set_zero(&netmask);
#else
	IP4_ADDR(&ipaddr, lwipdev.ip[0], lwipdev.ip[1],
			 lwipdev.ip[2], lwipdev.ip[3]);
	IP4_ADDR(&netmask, lwipdev.netmask[0], lwipdev.netmask[1],
			 lwipdev.netmask[2], lwipdev.netmask[3]);
	IP4_ADDR(&gw,lwipdev.gateway[0], lwipdev.gateway[1],
			 lwipdev.gateway[2], lwipdev.gateway[3]);
	printf("LwipDefaultIpSet OK: \n\r");
	printf("MAC......%d.%d.%d.%d.%d.%d\r\n", lwipdev.mac[0], lwipdev.mac[1],
											 lwipdev.mac[2], lwipdev.mac[3],
											 lwipdev.mac[4], lwipdev.mac[5]);
	printf("IP.......%d.%d.%d.%d\r\n", lwipdev.ip[0], lwipdev.ip[1],
									   lwipdev.ip[2], lwipdev.ip[3]);
	printf("MASK.....%d.%d.%d.%d\r\n", lwipdev.netmask[0], lwipdev.netmask[1],
									   lwipdev.netmask[2], lwipdev.netmask[3]);
	printf("GATE.....%d.%d.%d.%d\r\n", lwipdev.gateway[0], lwipdev.gateway[1],
									   lwipdev.gateway[2], lwipdev.gateway[3]);
#endif

	netifInitFlag = netif_add(&lwip_netif, &ipaddr, &netmask, &gw,
							  NULL, &EthernetifInit, &tcpip_input);
	if (netifInitFlag = NULL) {
		printf("		netif_add failed!!!\n\r");
	}
    netif_set_default(&lwip_netif);
    netif_set_up(&lwip_netif);
    do {
		for (int i = 0; i < 2000; i++);
        overtime++;
        if (overtime > NETIF_SETUP_OVERTIME) {
            printf("		netif_is_link_up overtime!\n\r");
            break;
        }
    } while (netif_is_link_up(&lwip_netif) == 0);
	if (overtime <= NETIF_SETUP_OVERTIME) {
		printf("		netif init succeed!\n\r");
	}

	intSave = PRT_HwiSetAttr(ETH_IRQn, OS_HWI_PRI_HIGHEST, OS_HWI_MODE_ENGROSS);
	if (intSave != OS_OK) {
		printf("		PRT_HwiSetAttr failed!!!\n\r");
	}
	intSave = PRT_HwiCreate(ETH_IRQn, EthIrqHandler, NULL);
	if (intSave != OS_OK) {
		printf("		PRT_HwiCreate failed!!!\n\r");
	}
	PRT_HwiEnable(ETH_IRQn);

#if LWIP_DHCP
	U32 ret = LwipDhcpCreate();
	if (ret != OS_OK) {
		printf("LwipDhcpCreate error, ret = 0x%x!!!", ret);
	}
#endif

	return 0;
}
