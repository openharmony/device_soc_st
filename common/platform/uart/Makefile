# Copyright (c) 2022 Nanjing Xiaoxiongpai Intelligent Technology Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

include $(LITEOSTOPDIR)/config.mk
include $(LITEOSTOPDIR)/../../drivers/hdf_core/adapter/khdf/liteos/lite.mk

MODULE_NAME := hdf_uart

LOCAL_CFLAGS += $(HDF_INCLUDE)

LOCAL_SRCS += stm32mp1_uart.c \
			  stm32mp1_uart_hw.c \
			  KRecvBuf.c \

LOCAL_CFLAGS += -fstack-protector-strong -Wextra -Wall -Werror -fsigned-char -fno-strict-aliasing -fno-common

include $(HDF_DRIVER)

