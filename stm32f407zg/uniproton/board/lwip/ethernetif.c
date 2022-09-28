/*
 * <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
 *
 * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *        http://www.st.com/software_license_agreement_liberty_v2
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ethernetif.h"
#include "lan8720.h"
#include "lwip.h"
#include "netif/etharp.h"
#include "string.h"

static err_t LowLevelInit(struct netif *netif)
{
#ifdef CHECKSUM_BY_HARDWARE
	int i;
#endif
	netif->hwaddr_len = ETHARP_HWADDR_LEN;
	netif->hwaddr[5] = lwipdev.mac[5];
	netif->hwaddr[4] = lwipdev.mac[4];
	netif->hwaddr[3] = lwipdev.mac[3];
	netif->hwaddr[2] = lwipdev.mac[2];
	netif->hwaddr[1] = lwipdev.mac[1];
	netif->hwaddr[0] = lwipdev.mac[0];
	netif->flags = NETIF_FLAG_ETHARP | NETIF_FLAG_BROADCAST | NETIF_FLAG_LINK_UP;
	netif->mtu = 1500; /* 1500, maximum transfer unit (in bytes) */
	ETH_MACAddressConfig(ETH_MAC_Address0, netif->hwaddr);
	ETH_DMATxDescChainInit(DMATxDscrTab, Tx_Buff, ETH_TXBUFNB);
	ETH_DMARxDescChainInit(DMARxDscrTab, Rx_Buff, ETH_RXBUFNB);
#ifdef CHECKSUM_BY_HARDWARE
	for (i = 0; i < ETH_TXBUFNB; i++) {
		ETH_DMATxDescChecksumInsertionConfig(&DMATxDscrTab[i], ETH_DMATxDesc_ChecksumTCPUDPICMPFull);
	}
#endif
	ETH_Start();
	return ERR_OK;
}

static err_t LowLevelOutput(struct netif *netif, struct pbuf *p)
{
	u8 res;
	int lenght = 0; /* 0, initial value */
	struct pbuf *qbuffer;
	u8 *buffer = (u8*)EthGetCurrentTxBuffer();
	for (qbuffer = p; qbuffer != NULL; qbuffer = qbuffer->next) {
		memcpy((u8_t*)&buffer[lenght], qbuffer->payload, qbuffer->len);
		lenght = lenght + qbuffer->len;
	}
	res = EthTxPacket(lenght);
	if(res == ETH_ERROR) {
		return ERR_MEM;
	}
	printf("		LowLevelOutput end\n\r");
	return ERR_OK;
}

static struct pbuf *LowLevelInput(struct netif *netif)
{
	int l = 0; /* 0, initial value */
	u16_t lenght;
	u8 *buffer;
	struct pbuf *pbuffer;
	struct pbuf *qbuffer;
	FrameTypeDef frame;

	pbuffer = NULL;
	frame = EthRxPacket();
	lenght = frame.length;
	buffer = (u8*)frame.buffer;
	pbuffer = pbuf_alloc(PBUF_RAW, lenght, PBUF_POOL);
	if (pbuffer != NULL) {
		for (qbuffer = pbuffer; qbuffer != NULL; qbuffer = qbuffer->next) {
			memcpy((u8_t*)qbuffer->payload, (u8_t*)&buffer[l], qbuffer->len);
			l = l + qbuffer->len;
		}
	}
	frame.descriptor->Status = ETH_DMARxDesc_OWN;
	if ((ETH->DMASR & ETH_DMASR_RBUS) != (u32)RESET) {
		ETH->DMASR = ETH_DMASR_RBUS;
		ETH->DMARPDR = 0; /* 0, register status */
	}
	return pbuffer;
}

err_t EthernetifInput(struct netif *netif)
{
	err_t error;
	struct pbuf *pbuffer;
	pbuffer = LowLevelInput(netif);
	if (pbuffer == NULL) {
		return ERR_MEM;
	}
	error = netif->input(pbuffer, netif);
	if (error != ERR_OK) {
		LWIP_DEBUGF(NETIF_DEBUG, ("EthernetifInput: IP input error\n"));
		pbuf_free(pbuffer);
		pbuffer = NULL;
	}
	return error;
}

err_t EthernetifInit(struct netif *netif)
{
	LWIP_ASSERT("netif!=NULL", (netif != NULL));
#if LWIP_NETIF_HOSTNAME
	netif->hostname="lwip";
#endif
	netif->output = etharp_output;
	netif->linkoutput = LowLevelOutput;
	netif->name[0] = IFNAME0;
	netif->name[1] = IFNAME1;
	LowLevelInit(netif);
	return ERR_OK;
}
