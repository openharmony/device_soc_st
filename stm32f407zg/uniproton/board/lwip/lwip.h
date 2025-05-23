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

#ifndef _LWIP_COMM_H
#define _LWIP_COMM_H

#include "lan8720.h"

#define LWIP_MAX_DHCP_TRIES		4

typedef struct lwipDev {
	u8 mac[6];
	u8 remoteip[4];
	u8 ip[4];
	u8 netmask[4];
	u8 gateway[4];
};

extern struct lwipDev lwipdev;
extern struct netif lwip_netif;

void LwipPktHandle(void);
void LwipDefaultIpSet(struct lwipDev *lwipx);
u8 LwipInit(void);

#endif
