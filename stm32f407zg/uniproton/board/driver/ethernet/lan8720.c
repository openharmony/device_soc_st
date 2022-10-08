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

#include "lan8720.h"
#include "stm32f4x7_eth.h"
#include "usart.h"
#include "prt_module.h"

uint8_t *Rx_Buff;
uint8_t *Tx_Buff;
ETH_DMADESCTypeDef *DMARxDscrTab;
ETH_DMADESCTypeDef *DMATxDscrTab;

static void EthernetNvicConf(void);
extern void LwipPktHandle(void);

u8 LAN8720Init(void)
{
	u8 ret = 0;
	GPIO_InitTypeDef gpioInitStruct;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOC |
						   RCC_AHB1Periph_GPIOG | RCC_AHB1Periph_GPIOD, ENABLE);
	SYSCFG_ETH_MediaInterfaceConfig(SYSCFG_ETH_MediaInterface_RMII);

	gpioInitStruct.GPIO_Pin = GPIO_Pin_7|GPIO_Pin_2|GPIO_Pin_1;
	gpioInitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	gpioInitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	gpioInitStruct.GPIO_OType = GPIO_OType_PP;
	gpioInitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_Init(GPIOA, &gpioInitStruct);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_ETH);

	gpioInitStruct.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_4 | GPIO_Pin_1;
	GPIO_Init(GPIOC, &gpioInitStruct);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource5, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource4, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource1, GPIO_AF_ETH);

	gpioInitStruct.GPIO_Pin =  GPIO_Pin_14 | GPIO_Pin_13 | GPIO_Pin_11;
	GPIO_Init(GPIOG, &gpioInitStruct);
	GPIO_PinAFConfig(GPIOG, GPIO_PinSource14, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOG, GPIO_PinSource13, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOG, GPIO_PinSource11, GPIO_AF_ETH);

	gpioInitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	gpioInitStruct.GPIO_Mode = GPIO_Mode_OUT;
	gpioInitStruct.GPIO_OType = GPIO_OType_PP;

	gpioInitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	gpioInitStruct.GPIO_Pin = GPIO_Pin_3;

	GPIO_Init(GPIOD, &gpioInitStruct);

	LAN8720_RST = 0; /* 0, register status */
	for (int i = 0; i < 2000; i++); /* 2000, delay */
	LAN8720_RST = 1; /* 1, register status */
	EthernetNvicConf();
	ret = EthMacdmaConf();
	return !ret;
}

static void EthernetNvicConf(void)
{
	NVIC_InitTypeDef nvicInitStruct;

	nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
	nvicInitStruct.NVIC_IRQChannelSubPriority = 0X00;
	nvicInitStruct.NVIC_IRQChannelPreemptionPriority = 0X00;
	nvicInitStruct.NVIC_IRQChannel = ETH_IRQn;

	NVIC_Init(&nvicInitStruct);
}

u8 LAN8720GetSpeed(void)
{
	return (u8)((ETH_ReadPHYRegister(0x00,31) & 0x1C) >> 2);
}

u8 EthMacdmaConf(void)
{
	u8 ret;
	ETH_InitTypeDef ethInitStruct;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_ETH_MAC | RCC_AHB1Periph_ETH_MAC_Tx |
						   RCC_AHB1Periph_ETH_MAC_Rx, ENABLE);
	ETH_DeInit();
	ETH_SoftwareReset();
	while (ETH_GetSoftwareResetStatus() == SET);
	ETH_StructInit(&ethInitStruct);

	ethInitStruct.ETH_AutoNegotiation = ETH_AutoNegotiation_Enable;
	ethInitStruct.ETH_UnicastFramesFilter = ETH_UnicastFramesFilter_Perfect;
	ethInitStruct.ETH_LoopbackMode = ETH_LoopbackMode_Disable;
	ethInitStruct.ETH_MulticastFramesFilter = ETH_MulticastFramesFilter_Perfect;
	ethInitStruct.ETH_RetryTransmission = ETH_RetryTransmission_Disable;
	ethInitStruct.ETH_PromiscuousMode = ETH_PromiscuousMode_Disable;
	ethInitStruct.ETH_AutomaticPadCRCStrip = ETH_AutomaticPadCRCStrip_Disable;
	ethInitStruct.ETH_BroadcastFramesReception = ETH_BroadcastFramesReception_Enable;
	ethInitStruct.ETH_ReceiveAll = ETH_ReceiveAll_Disable;

#ifdef CHECKSUM_BY_HARDWARE
	ethInitStruct.ETH_ChecksumOffload = ETH_ChecksumOffload_Enable;
#endif
	ethInitStruct.ETH_DropTCPIPChecksumErrorFrame = ETH_DropTCPIPChecksumErrorFrame_Enable;
	ethInitStruct.ETH_TransmitStoreForward = ETH_TransmitStoreForward_Enable;
	ethInitStruct.ETH_ReceiveStoreForward = ETH_ReceiveStoreForward_Enable;

	ethInitStruct.ETH_ForwardErrorFrames = ETH_ForwardErrorFrames_Disable;
	ethInitStruct.ETH_DMAArbitration = ETH_DMAArbitration_RoundRobin_RxTx_2_1;
	ethInitStruct.ETH_ForwardUndersizedGoodFrames = ETH_ForwardUndersizedGoodFrames_Disable;
	ethInitStruct.ETH_TxDMABurstLength = ETH_TxDMABurstLength_32Beat;
	ethInitStruct.ETH_SecondFrameOperate = ETH_SecondFrameOperate_Enable;
	ethInitStruct.ETH_RxDMABurstLength = ETH_RxDMABurstLength_32Beat;
	ethInitStruct.ETH_AddressAlignedBeats = ETH_AddressAlignedBeats_Enable;
	ethInitStruct.ETH_FixedBurst = ETH_FixedBurst_Enable;

	ret = ETH_Init(&ethInitStruct, LAN8720_PHY_ADDRESS);
	if (ret == ETH_SUCCESS) {
		ETH_DMAITConfig(ETH_DMA_IT_NIS | ETH_DMA_IT_R, ENABLE);
	}
	return ret;
}

void EthIrqHandler(void)
{
	uint32_t lenght = 0; /* 0, lenght */
	while ((lenght = ETH_GetRxPktSize(DMARxDescToGet)) != 0) { /* 0, lenght */
		LwipPktHandle();
	}
	ETH_DMAClearITPendingBit(ETH_DMA_IT_R);
	ETH_DMAClearITPendingBit(ETH_DMA_IT_NIS);
}

FrameTypeDef EthRxPacket(void)
{
	u32 framlength = 0;  /* 0, framlength */
	FrameTypeDef fram = {0, 0}; /* 0, initial value */

	if((DMARxDescToGet->Status & ETH_DMARxDesc_OWN) != (u32)RESET) {
		fram.length = ETH_ERROR;
		if ((ETH->DMASR & ETH_DMASR_RBUS) != (u32)RESET) {
			ETH->DMASR = ETH_DMASR_RBUS;
			ETH->DMARPDR = 0; /* register satus */
		}
		return fram;
	}

	if (((DMARxDescToGet->Status & ETH_DMARxDesc_LS)!=(u32)RESET) &&
		((DMARxDescToGet->Status & ETH_DMARxDesc_FS)!=(u32)RESET) &&
		((DMARxDescToGet->Status&ETH_DMARxDesc_ES)==(u32)RESET)) {
		framlength = ((DMARxDescToGet->Status & ETH_DMARxDesc_FL) >> ETH_DMARxDesc_FrameLengthShift) - 4;
 		fram.buffer = DMARxDescToGet->Buffer1Addr;
	} else {
		framlength = ETH_ERROR;
	}

	fram.descriptor = DMARxDescToGet;
	fram.length = framlength;
	DMARxDescToGet = (ETH_DMADESCTypeDef*)(DMARxDescToGet->Buffer2NextDescAddr);
	return fram;
}

u8 EthTxPacket(u16 frameLength)
{
	if ((DMATxDescToSet->Status & ETH_DMATxDesc_OWN) != (u32)RESET) {
		return ETH_ERROR;
	}

 	DMATxDescToSet->Status |= ETH_DMATxDesc_LS | ETH_DMATxDesc_FS | ETH_DMATxDesc_OWN;
	DMATxDescToSet->ControlBufferSize = (frameLength & ETH_DMATxDesc_TBS1);

	if((ETH->DMASR & ETH_DMASR_TBUS) != (u32)RESET) {
		ETH->DMATPDR = 0; /* 0, register status */
		ETH->DMASR = ETH_DMASR_TBUS;
	}
	DMATxDescToSet = (ETH_DMADESCTypeDef*)(DMATxDescToSet->Buffer2NextDescAddr);
	return ETH_SUCCESS;
}

u32 EthGetCurrentTxBuffer(void)
{
	return DMATxDescToSet->Buffer1Addr;
}

u8 EthMemMalloc(void)
{
	Tx_Buff = PRT_MemAlloc(OS_MID_MEM, 0, ETH_TX_BUF_SIZE * ETH_TXBUFNB);
	Rx_Buff = PRT_MemAlloc(OS_MID_MEM, 0, ETH_RX_BUF_SIZE * ETH_RXBUFNB);
	DMATxDscrTab = PRT_MemAlloc(OS_MID_MEM, 0, ETH_TXBUFNB * sizeof(ETH_DMADESCTypeDef));
	DMARxDscrTab = PRT_MemAlloc(OS_MID_MEM, 0, ETH_RXBUFNB * sizeof(ETH_DMADESCTypeDef));
	if(!Tx_Buff || !Rx_Buff || !DMATxDscrTab || !DMARxDscrTab) {
		EthMemFree();
		return 1; /* 1, PRT_MemAlloc error */
	}
	return 0; /* 0, PRT_MemAlloc ok */
}

void EthMemFree(void)
{
	PRT_MemFree(OS_MID_MEM, DMARxDscrTab);
	PRT_MemFree(OS_MID_MEM, DMATxDscrTab);
	PRT_MemFree(OS_MID_MEM, Rx_Buff);
	PRT_MemFree(OS_MID_MEM, Tx_Buff);
}
