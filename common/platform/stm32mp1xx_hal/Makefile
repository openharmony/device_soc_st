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

MODULE_NAME := stm32mp1x_hal

LOCAL_CFLAGS += $(HDF_INCLUDE)

LOCAL_SRCS += STM32MP1xx_HAL_Driver/Src/system_stm32mp1xx.c \
			  STM32MP1xx_HAL_Driver/Src/stm32mp1xx_hal.c \
			  STM32MP1xx_HAL_Driver/Src/stm32mp1xx_hal_gpio.c \
			  STM32MP1xx_HAL_Driver/Src/stm32mp1xx_hal_i2c.c \
			  STM32MP1xx_HAL_Driver/Src/stm32mp1xx_hal_exti.c \
			  STM32MP1xx_HAL_Driver/Src/stm32mp1xx_hal_ltdc.c \
			  STM32MP1xx_HAL_Driver/Src/stm32mp1xx_hal_rcc.c \
			  STM32MP1xx_HAL_Driver/Src/stm32mp1xx_hal_rcc_ex.c \
			  STM32MP1xx_HAL_Driver/Src/stm32mp1xx_hal_rng.c \
                          STM32MP1xx_HAL_Driver/Src/stm32mp1xx_hal_tim.c \
			  STM32MP1xx_HAL_Driver/Src/stm32mp1xx_hal_tim_ex.c \

LOCAL_INCLUDE += -I STM32MP1xx_HAL_Driver/Inc



include $(HDF_DRIVER)

