/**
  ******************************************************************************
  * @file    stm32f4x7_eth_conf_template.h
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    31-July-2013
  * @brief   Configuration file for the STM32F4x7xx Ethernet driver.
  *          This file should be copied to the application folder and renamed to
  *          stm32f4x7_eth_conf.h    
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
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
  *
  ******************************************************************************
  */

#ifndef __STM32F4x7_ETH_CONF_H
#define __STM32F4x7_ETH_CONF_H
#include "stm32f4xx.h"

#define USE_ENHANCED_DMA_DESCRIPTORS

#ifdef USE_Delay
#define _eth_delay_         Delay
#else
#define _eth_delay_         ETH_Delay
#endif

#ifdef  CUSTOM_DRIVER_BUFFERS_CONFIG
#define ETH_RX_BUF_SIZE     ETH_MAX_PACKET_SIZE
#define ETH_TX_BUF_SIZE     ETH_MAX_PACKET_SIZE
#define ETH_RXBUFNB         20
#define ETH_TXBUFNB         5
#endif

#ifdef USE_Delay
#define PHY_RESET_DELAY     ((uint32_t)0x000000FF)
#define PHY_CONFIG_DELAY    ((uint32_t)0x00000FFF)
#define ETH_REG_WRITE_DELAY ((uint32_t)0x00000001)
#else
#define PHY_RESET_DELAY     ((uint32_t)0x000FFFFF)
#define PHY_CONFIG_DELAY    ((uint32_t)0x00FFFFFF)
#define ETH_REG_WRITE_DELAY ((uint32_t)0x0000FFFF)
#endif

#define PHY_SR              ((uint16_t)31)
#define PHY_SPEED_STATUS    ((uint16_t)0x0004)
#define PHY_DUPLEX_STATUS   ((uint16_t)0x00010)

#endif
