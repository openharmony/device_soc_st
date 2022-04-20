/*
 * Copyright (c) 2021 Nanjing Xiaoxiongpai Intelligent Technology Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "device_resource_if.h"
#include "hdf_device_desc.h"
#include "osal_io.h"
#include "osal.h"

#include "watchdog_if.h"
#include "watchdog_core.h"

#include "securec.h"

#define HDF_LOG_TAG Mp1xxIwdg

#define BITS_PER_LONG 32

#define GENMASK(h, l) \
    (((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

/* IWDG registers */
#define IWDG_KR		0x00 /* Key register */
#define IWDG_PR		0x04 /* Prescaler Register */
#define IWDG_RLR	0x08 /* ReLoad Register */
#define IWDG_SR		0x0C /* Status Register */
#define IWDG_WINR	0x10 /* Windows Register */

/* IWDG_KR register bit mask */
#define KR_KEY_RELOAD	0xAAAA /* reload counter enable */
#define KR_KEY_ENABLE	0xCCCC /* peripheral enable */
#define KR_KEY_EWA	0x5555 /* write access enable */
#define KR_KEY_DWA	0x0000 /* write access disable */

/* IWDG_PR register */
#define PR_SHIFT	2
#define PR_MIN		BIT(PR_SHIFT)

/* IWDG_RLR register values */
#define RLR_MIN		0x2		/* min value recommended */
#define RLR_MAX		GENMASK(11, 0)	/* max value of reload register */

/* IWDG_SR register bit mask */
#define SR_PVU	BIT(0) /* Watchdog prescaler value update */
#define SR_RVU	BIT(1) /* Watchdog counter reload value update */

/* set timeout to 100000 us */
#define TIMEOUT_US	100000
#define SLEEP_US	1000

#define DEFAULT_TIMEOUT             (32)
#define DEFAULT_TASK_STACK_SIZE     (0x800)
#define DEFAULT_CLOCK_RATE          (32000)

#define DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))

/**
 * fls - find last (most-significant) bit set
 * @x: the word to search
 *
 * This is defined the same way as ffs.
 * Note fls(0) = 0, fls(1) = 1, fls(0x80000000) = 32.
 */
static inline int generic_fls(int x)
{
    int r = 32;

    if (!x) {
        return 0;
    }
    if (!(x & 0xffff0000u)) {
        x <<= 16;
        r -= 16;
    }
    if (!(x & 0xff000000u)) {
        x <<= 8;
        r -= 8;
    }
    if (!(x & 0xf0000000u)) {
        x <<= 4;
        r -= 4;
    }
    if (!(x & 0xc0000000u)) {
        x <<= 2;
        r -= 2;
    }
    if (!(x & 0x80000000u)) {
        x <<= 1;
        r -= 1;
    }
    return r;
}

static inline int ilog2(unsigned int x)
{
    return generic_fls(x) - 1;
}

unsigned long roundup_pow_of_two(unsigned long n)
{
    return 1UL << generic_fls(n - 1);
}

struct Mp1xxIwdg {
    struct WatchdogCntlr wdt;   // 控制器

    uint32_t num;               // 当前独立看门狗编号

    void volatile *base;        // 虚拟地址
    uint32_t phy_base;           // 物理地址
    uint32_t reg_step;           // 映射大小

    uint32_t seconds;           // 当前设置的超时值(s)

    bool start;                 // 当前iwdg是否已经启动

    uint32_t rate;              // 时钟源频率
    char *clock_source;          // 时钟源名称

    uint32_t min_timeout;           // 最小超时时间
    uint32_t max_hw_heartbeat_ms;   // 最大超时时间
};

static inline uint32_t reg_read(void volatile *base, uint32_t reg)
{
    return OSAL_READL((uintptr_t)base + reg);
}

static inline void reg_write(void volatile *base, uint32_t reg, uint32_t val)
{
    OSAL_WRITEL(val, (uintptr_t)base + reg);
}

// get clock source real rate
static inline int32_t Mp1xxIwdgGetClockRate(struct Mp1xxIwdg *iwdg)
{
    int ret = HDF_SUCCESS;

    /*
        if "clock_source" is set, use the real rate of clock source
        otherwise, use the default clock rate
    */
    if (iwdg->clock_source != NULL) {
        // get clock source real rate.
        // ...
        ret = HDF_SUCCESS;
    }

    return ret;
}

static inline uint32_t Mp1xxIwdgGetSr(struct Mp1xxIwdg *iwdg)
{
    return reg_read(iwdg->base, IWDG_SR);
}

int32_t Mp1xxIwdgStart(struct WatchdogCntlr *wdt)
{
    struct Mp1xxIwdg *iwdg = NULL;
    uint32_t tout, presc, iwdg_pr, iwdg_rlr, iwdg_sr;
    uint32_t i = 10;

    if (wdt == NULL) {
        return HDF_ERR_INVALID_OBJECT;
    }
    iwdg = (struct Mp1xxIwdg *)wdt->priv;

    // 计算装载值
    tout = iwdg->seconds;// 超时秒数

    // 计算边界
    if (tout > (iwdg->max_hw_heartbeat_ms * 1000)) {
        tout = iwdg->max_hw_heartbeat_ms * 1000;
    }
    if (tout < iwdg->min_timeout) {
        tout = iwdg->min_timeout;
    }

    presc = DIV_ROUND_UP(tout * iwdg->rate, RLR_MAX + 1);

    /* The prescaler is align on power of 2 and start at 2 ^ PR_SHIFT. */
    presc = roundup_pow_of_two(presc);
    iwdg_pr = (presc <= (1 << PR_SHIFT)) ? 0 : ilog2(presc) - PR_SHIFT;
    iwdg_rlr = ((tout * iwdg->rate) / presc) - 1;

    /* enable write access */
    reg_write(iwdg->base, IWDG_KR, KR_KEY_EWA);

    /* set prescaler & reload registers */
    reg_write(iwdg->base, IWDG_PR, iwdg_pr);
    reg_write(iwdg->base, IWDG_RLR, iwdg_rlr);
    reg_write(iwdg->base, IWDG_KR, KR_KEY_ENABLE);

    // 等待状态寄存器 SR_PVU | SR_RVU 复位
    while ((iwdg_sr = Mp1xxIwdgGetSr(iwdg)) & (SR_PVU | SR_RVU)) {
        if (!(--i)) {
            HDF_LOGE("Fail to set prescaler, reload regs.");
            return HDF_FAILURE;
        }
    }

    /* reload watchdog */
    reg_write(iwdg->base, IWDG_KR, KR_KEY_RELOAD);

    /* iwdg start */
    iwdg->start = true;

    return HDF_SUCCESS;
}

int32_t Mp1xxIwdgSetTimeout(struct WatchdogCntlr *wdt, uint32_t seconds)
{
    struct Mp1xxIwdg *iwdg = NULL;

    if (wdt == NULL) {
        return HDF_ERR_INVALID_OBJECT;
    }

    iwdg = (struct Mp1xxIwdg *)wdt->priv;
    iwdg->seconds = seconds;

    // 如果iwdg已经是启动状态, 需要重新装载超时值并继续喂狗操作
    if (iwdg->start) {
        return Mp1xxIwdgStart(wdt);
    }

    return HDF_SUCCESS;
}

int32_t Mp1xxIwdgGetTimeout(struct WatchdogCntlr *wdt, uint32_t *seconds)
{
    struct Mp1xxIwdg *iwdg = NULL;
    if (wdt == NULL || seconds == NULL) {
        return HDF_ERR_INVALID_OBJECT;
    }
    iwdg = (struct Mp1xxIwdg *)wdt->priv;

    *seconds = iwdg->seconds;

    return HDF_SUCCESS;
}

static int32_t Mp1xxIwdgFeed(struct WatchdogCntlr *wdt)
{
    struct Mp1xxIwdg *iwdg = NULL;

    if (wdt == NULL) {
        return HDF_ERR_INVALID_OBJECT;
    }
    iwdg = (struct Mp1xxIwdg *)wdt->priv;

    /* reload watchdog */
    reg_write(iwdg->base, IWDG_KR, KR_KEY_RELOAD);

    return HDF_SUCCESS;
}

static int32_t Mp1xxIwdgGetStatus(struct WatchdogCntlr *wdt, int32_t *status)
{
    (void)status;
    int32_t ret = WATCHDOG_STOP;
    struct Mp1xxIwdg *iwdg = NULL;

    if (wdt == NULL) {
        return HDF_ERR_INVALID_OBJECT;
    }
    iwdg = (struct Mp1xxIwdg *)wdt->priv;

    if (iwdg->start) {
        ret = WATCHDOG_START;
    }
    return ret;
}

/* WatchdogOpen 的时候被调用 */
static int32_t Mp1xxIwdgGetPriv(struct WatchdogCntlr *wdt)
{
    int32_t ret;
    struct Mp1xxIwdg *iwdg = NULL;

    if (wdt == NULL) {
        return HDF_ERR_INVALID_OBJECT;
    }
    iwdg = (struct Mp1xxIwdg *)wdt->priv;

    // 获取当前时钟源频率
    ret = Mp1xxIwdgGetClockRate(iwdg);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("Mp1xxIwdgGetClockRate fail, ret : %#x.", ret);
        return HDF_FAILURE;
    }

    // 计算最大最小的超时时间
    iwdg->min_timeout = DIV_ROUND_UP((RLR_MIN + 1) * PR_MIN, iwdg->rate);
    iwdg->max_hw_heartbeat_ms = ((RLR_MAX + 1) * 1024 * 1000) / iwdg->rate;

    return ret;
}

/* WatchdogClose 的时候被调用 */
static void Mp1xxIwdgReleasePriv(struct WatchdogCntlr *wdt)
{
    (void)wdt;
}

static struct WatchdogMethod g_stm32mp1_iwdg_ops = {
    .feed = Mp1xxIwdgFeed,
    .getPriv = Mp1xxIwdgGetPriv,
    .getStatus = Mp1xxIwdgGetStatus,
    .getTimeout = Mp1xxIwdgGetTimeout,
    .releasePriv = Mp1xxIwdgReleasePriv,
    .setTimeout = Mp1xxIwdgSetTimeout,
    .start = Mp1xxIwdgStart,

// stm32mp1的iwdg不支持软件停止
    .stop = NULL
};

static int32_t Mp1xxIwdgReadDrs(struct Mp1xxIwdg *iwdg, const struct DeviceResourceNode *node)
{
    int32_t ret;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL) {
        HDF_LOGE("%s: invalid drs ops!", __func__);
        return HDF_FAILURE;
    }

    // num
    ret = drsOps->GetUint32(node, "num", &iwdg->num, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read num fail!", __func__);
        return ret;
    }

    // reg_base
    ret = drsOps->GetUint32(node, "reg_base", &iwdg->phy_base, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read regBase fail!", __func__);
        return ret;
    }

    // reg_step
    ret = drsOps->GetUint32(node, "reg_step", &iwdg->reg_step, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read regStep fail!", __func__);
        return ret;
    }

    // default timeout
    ret = drsOps->GetUint32(node, "timeout_sec", &iwdg->seconds, DEFAULT_TIMEOUT);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read timeout fail!", __func__);
        return ret;
    }

    // default source rate
    ret = drsOps->GetUint32(node, "clock_rate", &iwdg->rate, DEFAULT_CLOCK_RATE);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read clock_rate fail!", __func__);
        return ret;
    }

    // start
    iwdg->start = drsOps->GetBool(node, "start");

    return HDF_SUCCESS;
}

static int32_t Mp1xxIwdgBind(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct Mp1xxIwdg *iwdg = NULL;

    if (device == NULL || device->property == NULL) {
        HDF_LOGE("%s: device or property is null!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    // 申请内存空间
    iwdg = (struct Mp1xxIwdg *)OsalMemCalloc(sizeof(struct Mp1xxIwdg));
    if (iwdg == NULL) {
        HDF_LOGE("%s: malloc iwdg fail!", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    // 解析配置
    ret = Mp1xxIwdgReadDrs(iwdg, device->property);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read drs fail:%d", __func__, ret);
        OsalMemFree(iwdg);
        return ret;
    }

    // 寄存器映射
    iwdg->base = OsalIoRemap(iwdg->phy_base, iwdg->reg_step);
    if (iwdg->base == NULL) {
        HDF_LOGE("%s: ioremap regbase fail!", __func__);
        OsalMemFree(iwdg);
        return HDF_ERR_IO;
    }

    // 填充操作符
    iwdg->wdt.priv = (void *)iwdg;
    iwdg->wdt.ops = &g_stm32mp1_iwdg_ops;
    iwdg->wdt.device = device;
    iwdg->wdt.wdtId = iwdg->num;

    // add device
    ret = WatchdogCntlrAdd(&iwdg->wdt);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: err add watchdog:%d.", __func__, ret);
        OsalIoUnmap((void *)iwdg->base);
        OsalMemFree(iwdg);
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t Mp1xxIwdgInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct WatchdogCntlr *wdt = NULL;
    struct Mp1xxIwdg *iwdg = NULL;

    // get WatchdogCntlr
    wdt = WatchdogCntlrFromDevice(device);
    if (wdt == NULL) {
        return HDF_FAILURE;
    }
    iwdg = (struct Mp1xxIwdg *)wdt->priv;

    // get priv data(get clock source)
    ret = Mp1xxIwdgGetPriv(wdt);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("Mp1xxIwdgGetPriv fail.");
        return HDF_FAILURE;
    }

    // set default timeout
    ret = Mp1xxIwdgSetTimeout(wdt, iwdg->seconds);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("Mp1xxIwdgSetTimeout fail.");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static void Mp1xxIwdgRelease(struct HdfDeviceObject *device)
{
    struct WatchdogCntlr *wdt = NULL;
    struct Mp1xxIwdg *iwdg = NULL;

    if (device == NULL) {
        return;
    }

    wdt = WatchdogCntlrFromDevice(device);
    if (wdt == NULL) {
        return;
    }
    WatchdogCntlrRemove(wdt);

    iwdg = (struct Mp1xxIwdg *)wdt->priv;
    if (iwdg->base != NULL) {
        OsalIoUnmap((void *)iwdg->base);
        iwdg->base = NULL;
    }
    OsalMemFree(iwdg);
}

struct HdfDriverEntry g_hdf_driver_iwdg_entry = {
    .moduleVersion = 1,
    .Bind = Mp1xxIwdgBind,
    .Init = Mp1xxIwdgInit,
    .Release = Mp1xxIwdgRelease,
    .moduleName = "stm32mp1_iwdg",
};
HDF_INIT(g_hdf_driver_iwdg_entry);
