include $(LITEOSTOPDIR)/../../drivers/hdf_core/adapter/khdf/liteos/lite.mk
MODULE_NAME := $(notdir $(shell pwd))

ifdef LOSCFG_HW_RANDOM_ENABLE
LOCAL_INCLUDE += -I ../stm32mp1xx_hal/STM32MP1xx_HAL_Driver/Inc

LOCAL_CFLAGS += $(LOCAL_INCLUDE)

LOCAL_SRCS += stm32mp1_rng.c

endif

include $(HDF_DRIVER)
