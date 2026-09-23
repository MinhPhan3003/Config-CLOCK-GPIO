/**
 * @file    clock.c
 * @brief   Hiện thực driver clock (SCG + PCC) cho S32K144.
 *
 * Quy tắc ghi thanh ghi trong file này:
 *   - Chuẩn bị giá trị trong biến cục bộ (union), rồi ghi MỘT LẦN qua .REG.
 *   - Không gán trực tiếp .BITS lên các thanh ghi có cờ w1c
 *     (SOSCCSR, FIRCCSR, SPLLCSR), để tránh vô tình xóa cờ lỗi.
 *
 * Trình tự và giá trị cấu hình tham khảo:
 *   - RM Rev.13, chương 27, 28, 29 (số trang ghi trong clock.h)
 *   - AN5413 S32K1xx Series Cookbook, ví dụ hello_clocks
 */

#include "clock.h"

/* Số vòng chờ tối đa khi đợi cờ VLD hoặc chuyển clock */
#define CLOCK_TIMEOUT_LOOPS     1000000u


/* ------------------------------------------------------------------------
 *  Hàm nội bộ
 * ------------------------------------------------------------------------ */

/* Đổi mã bộ chia DIV1/DIV2 sang hệ số chia thật: 1 -> /1, 2 -> /2, 3 -> /4 ... */
static uint32_t clock_async_div_value(uint32_t code)
{
    return (code == 0u) ? 0u : (1u << (code - 1u));
}

/* Tần số SPLL_CLK tính từ SPLLCFG (28.3.19) */
static uint32_t clock_get_spll_freq(void)
{
    SCG_SPLLCFG_t cfg;
    uint32_t vco;

    cfg.REG = SCG->SPLLCFG.REG;
    vco = (CLOCK_SOSC_FREQ_HZ / (cfg.BITS.PREDIV + 1u)) * (cfg.BITS.MULT + 16u);
    return vco / 2u;
}

/* Tần số của một nguồn clock theo mã SCS */
static uint32_t clock_get_source_freq(uint32_t scs)
{
    switch (scs) {
    case CLOCK_SCS_SOSC:
        return CLOCK_SOSC_FREQ_HZ;
    case CLOCK_SCS_SIRC:
        return (SCG->SIRCCFG.BITS.RANGE != 0u) ? CLOCK_SIRC_HIGH_HZ : CLOCK_SIRC_LOW_HZ;
    case CLOCK_SCS_FIRC:
        return CLOCK_FIRC_FREQ_HZ;
    case CLOCK_SCS_SPLL:
        return clock_get_spll_freq();
    default:
        return 0u;
    }
}


/* ------------------------------------------------------------------------
 *  Nguồn clock
 * ------------------------------------------------------------------------ */

/**
 * Bật SOSC với thạch anh 8 MHz trên EVB.
 * RM: SOSCDIV 28.3.9 tr.598, SOSCCFG 28.3.10 tr.599, SOSCCSR 28.3.8 tr.596.
 * SOSCCFG chỉ được ghi khi SOSC đang tắt.
 */
CLOCK_Status CLOCK_SOSC_Init_8MHz(void)
{
    SCG_DIV_t     div = { .REG = 0u };
    SCG_SOSCCFG_t cfg = { .REG = 0u };
    SCG_SOSCCSR_t csr = { .REG = 0u };
    uint32_t      timeout = CLOCK_TIMEOUT_LOOPS;

    if (SCG->SOSCCSR.BITS.LK != 0u) {
        return CLOCK_ERR_LOCKED;
    }

    /* 1. Bộ chia cho ngoại vi: SOSCDIV1 = /1, SOSCDIV2 = /1 -> 8 MHz */
    div.BITS.DIV1 = CLOCK_DIV_1;
    div.BITS.DIV2 = CLOCK_DIV_1;
    SCG->SOSCDIV.REG = div.REG;

    /* 2. Cấu hình thạch anh: EREFS = 1 (crystal), HGO = 0 (low-gain),
     *    RANGE = 2 (medium). Đối chiếu bảng giá trị RANGE trong 28.3.10. */
    cfg.BITS.EREFS = 1u;
    cfg.BITS.HGO   = 0u;
    cfg.BITS.RANGE = 2u;
    SCG->SOSCCFG.REG = cfg.REG;

    /* 3. Bật SOSC (ghi cả thanh ghi, các bit w1c được ghi 0 nên không bị xóa) */
    csr.BITS.SOSCEN = 1u;
    SCG->SOSCCSR.REG = csr.REG;

    /* 4. Chờ SOSC ổn định */
    while (SCG->SOSCCSR.BITS.SOSCVLD == 0u) {
        if (--timeout == 0u) {
            return CLOCK_ERR_TIMEOUT;
        }
    }
    return CLOCK_OK;
}

/**
 * Bật SPLL 160 MHz từ SOSC 8 MHz:
 *   VCO = 8 MHz / (0 + 1) * (24 + 16) = 320 MHz, SPLL_CLK = 160 MHz.
 * SPLLDIV1 = /2 -> 80 MHz, SPLLDIV2 = /4 -> 40 MHz.
 * RM: SPLLCSR 28.3.17 tr.608, SPLLDIV 28.3.18 tr.610, SPLLCFG 28.3.19 tr.611.
 * Giới hạn tần số VCO/SPLL: 28.1.2 tr.579. SPLLCFG chỉ ghi khi SPLL tắt.
 */
CLOCK_Status CLOCK_SPLL_Init_160MHz(void)
{
    SCG_DIV_t     div = { .REG = 0u };
    SCG_SPLLCFG_t cfg = { .REG = 0u };
    SCG_SPLLCSR_t csr = { .REG = 0u };
    uint32_t      timeout = CLOCK_TIMEOUT_LOOPS;

    if (SCG->SPLLCSR.BITS.LK != 0u) {
        return CLOCK_ERR_LOCKED;
    }

    /* 1. Tắt SPLL trước khi cấu hình */
    SCG->SPLLCSR.REG = 0u;

    /* 2. Bộ chia cho ngoại vi */
    div.BITS.DIV1 = CLOCK_DIV_2;
    div.BITS.DIV2 = CLOCK_DIV_4;
    SCG->SPLLDIV.REG = div.REG;

    /* 3. PREDIV = 0, MULT = 24, nguồn SOSC */
    cfg.BITS.SOURCE = 0u;
    cfg.BITS.PREDIV = 0u;
    cfg.BITS.MULT   = 24u;
    SCG->SPLLCFG.REG = cfg.REG;

    /* 4. Bật SPLL */
    csr.BITS.SPLLEN = 1u;
    SCG->SPLLCSR.REG = csr.REG;

    /* 5. Chờ PLL khóa pha */
    while (SCG->SPLLCSR.BITS.SPLLVLD == 0u) {
        if (--timeout == 0u) {
            return CLOCK_ERR_TIMEOUT;
        }
    }
    return CLOCK_OK;
}

/* Sau reset các bộ chia DIV1/DIV2 bằng 0 (tắt) - xem 27.4.1 tr.563.
 * Ngoại vi chọn PCS = SIRCDIV2 / FIRCDIV2 cần đặt bộ chia khác 0. */
void CLOCK_SIRC_SetDividers(CLOCK_AsyncDiv div1, CLOCK_AsyncDiv div2)
{
    SCG_DIV_t div = { .REG = 0u };
    div.BITS.DIV1 = (uint32_t)div1;
    div.BITS.DIV2 = (uint32_t)div2;
    SCG->SIRCDIV.REG = div.REG;
}

void CLOCK_FIRC_SetDividers(CLOCK_AsyncDiv div1, CLOCK_AsyncDiv div2)
{
    SCG_DIV_t div = { .REG = 0u };
    div.BITS.DIV1 = (uint32_t)div1;
    div.BITS.DIV2 = (uint32_t)div2;
    SCG->FIRCDIV.REG = div.REG;
}


/* ------------------------------------------------------------------------
 *  Clock monitor SOSC / SPLL
 *
 *  Cả hai hàm SetMonitor dùng cùng một cách:
 *    1. Đọc thanh ghi CSR hiện tại (giữ nguyên SOSCEN/SPLLEN = 1).
 *    2. Ép cờ ERR = 0 trong biến cục bộ. Nếu ERR đang bằng 1 mà ghi lại
 *       nguyên giá trị vừa đọc, cờ w1c này sẽ bị xóa ngoài ý muốn.
 *    3. Đặt CM / CMRE theo mode, rồi ghi một lần qua .REG.
 *  Các bit chỉ đọc (VLD, SEL) được ghi lại đúng giá trị đọc, phần cứng bỏ qua.
 *
 *  Monitor chỉ được bật khi nguồn đã ổn định (VLD = 1).
 *  Đọc thêm ràng buộc ở RM 28.1.5, tr.581.
 * ------------------------------------------------------------------------ */

CLOCK_Status CLOCK_SOSC_SetMonitor(CLOCK_MonitorMode mode)
{
    SCG_SOSCCSR_t csr;

    if (mode > CLOCK_CM_RESET) {
        return CLOCK_ERR_PARAM;
    }

    csr.REG = SCG->SOSCCSR.REG;
    if (csr.BITS.LK != 0u) {
        return CLOCK_ERR_LOCKED;
    }
    if ((mode != CLOCK_CM_DISABLE) && (csr.BITS.SOSCVLD == 0u)) {
        return CLOCK_ERR_NOT_VALID;
    }

    csr.BITS.SOSCERR  = 0u;                                 /* không xóa nhầm cờ lỗi */
    csr.BITS.SOSCCM   = (mode != CLOCK_CM_DISABLE) ? 1u : 0u;
    csr.BITS.SOSCCMRE = (mode == CLOCK_CM_RESET)   ? 1u : 0u;
    SCG->SOSCCSR.REG = csr.REG;

    return CLOCK_OK;
}

CLOCK_Status CLOCK_SPLL_SetMonitor(CLOCK_MonitorMode mode)
{
    SCG_SPLLCSR_t csr;

    if (mode > CLOCK_CM_RESET) {
        return CLOCK_ERR_PARAM;
    }

    csr.REG = SCG->SPLLCSR.REG;
    if (csr.BITS.LK != 0u) {
        return CLOCK_ERR_LOCKED;
    }
    if ((mode != CLOCK_CM_DISABLE) && (csr.BITS.SPLLVLD == 0u)) {
        return CLOCK_ERR_NOT_VALID;
    }

    csr.BITS.SPLLERR  = 0u;                                 /* không xóa nhầm cờ lỗi */
    csr.BITS.SPLLCM   = (mode != CLOCK_CM_DISABLE) ? 1u : 0u;
    csr.BITS.SPLLCMRE = (mode == CLOCK_CM_RESET)   ? 1u : 0u;
    SCG->SPLLCSR.REG = csr.REG;

    return CLOCK_OK;
}

uint8_t CLOCK_SOSC_HasError(void)
{
    return (uint8_t)SCG->SOSCCSR.BITS.SOSCERR;
}

uint8_t CLOCK_SPLL_HasError(void)
{
    return (uint8_t)SCG->SPLLCSR.BITS.SPLLERR;
}

/* Xóa cờ lỗi w1c: ghi lại cấu hình hiện tại, chỉ đặt ERR = 1 */
void CLOCK_SOSC_ClearError(void)
{
    SCG_SOSCCSR_t csr;
    csr.REG = SCG->SOSCCSR.REG;
    csr.BITS.SOSCERR = 1u;
    SCG->SOSCCSR.REG = csr.REG;
}

void CLOCK_SPLL_ClearError(void)
{
    SCG_SPLLCSR_t csr;
    csr.REG = SCG->SPLLCSR.REG;
    csr.BITS.SPLLERR = 1u;
    SCG->SPLLCSR.REG = csr.REG;
}


/* ------------------------------------------------------------------------
 *  Clock hệ thống
 * ------------------------------------------------------------------------ */

/**
 * Chuyển clock hệ thống sang SPLL trong RUN mode:
 *   CORE = 160 / 2 = 80 MHz, BUS = 80 / 2 = 40 MHz, SLOW = 80 / 3 = 26.67 MHz.
 * RM: RCCR 28.3.4 tr.588, CSR 28.3.3 tr.585,
 *     giới hạn tần số RUN mode: 27.4 tr.558, chuyển clock: 28.1.4 tr.580.
 * RCCR được ghi một lần bằng lệnh 32-bit.
 */
CLOCK_Status CLOCK_RunMode_80MHz(void)
{
    SCG_CCR_t rccr = { .REG = 0u };
    uint32_t  timeout = CLOCK_TIMEOUT_LOOPS;

    rccr.BITS.SCS     = CLOCK_SCS_SPLL;
    rccr.BITS.DIVCORE = 1u;     /* /2 */
    rccr.BITS.DIVBUS  = 1u;     /* /2 */
    rccr.BITS.DIVSLOW = 2u;     /* /3 */
    SCG->RCCR.REG = rccr.REG;

    /* CSR phản ánh clock đang thực sự dùng: chờ tới khi đổi xong */
    while (SCG->CSR.BITS.SCS != CLOCK_SCS_SPLL) {
        if (--timeout == 0u) {
            return CLOCK_ERR_TIMEOUT;
        }
    }
    return CLOCK_OK;
}

/**
 * Trình tự đầy đủ:
 *   SOSC -> monitor SOSC -> SPLL -> monitor SPLL
 *   -> bộ chia SIRC/FIRC -> RUN 80 MHz.
 * Nguồn clock phải VLD trước khi bật monitor và trước khi được chọn
 * làm system clock. Chế độ monitor lấy từ CLOCK_INIT_MONITOR_MODE.
 */
CLOCK_Status CLOCK_Init_80MHz(void)
{
    CLOCK_Status st;

    st = CLOCK_SOSC_Init_8MHz();
    if (st != CLOCK_OK) {
        return st;
    }

    st = CLOCK_SOSC_SetMonitor(CLOCK_INIT_MONITOR_MODE);
    if (st != CLOCK_OK) {
        return st;
    }

    st = CLOCK_SPLL_Init_160MHz();
    if (st != CLOCK_OK) {
        return st;
    }

    st = CLOCK_SPLL_SetMonitor(CLOCK_INIT_MONITOR_MODE);
    if (st != CLOCK_OK) {
        return st;
    }

    /* Mở bộ chia để ngoại vi có thể chọn SIRCDIV2 / FIRCDIV2 */
    CLOCK_SIRC_SetDividers(CLOCK_DIV_1, CLOCK_DIV_1);   /* 8 MHz  */
    CLOCK_FIRC_SetDividers(CLOCK_DIV_1, CLOCK_DIV_1);   /* 48 MHz */

    return CLOCK_RunMode_80MHz();
}


/* ------------------------------------------------------------------------
 *  Clock ngoại vi (PCC)
 * ------------------------------------------------------------------------ */

/* Bật clock cho ngoại vi không cần chọn nguồn (ví dụ PORT).
 * RM: 29.4 tr.623 và thanh ghi PCC tương ứng. */
CLOCK_Status CLOCK_EnablePeripheral(uint32_t pcc_index)
{
    PCC_REG_t reg;

    if (pcc_index >= PCC_PCCn_COUNT) {
        return CLOCK_ERR_PARAM;
    }

    reg.REG = PCC->PCCn[pcc_index].REG;
    if (reg.BITS.PR == 0u) {
        return CLOCK_ERR_NOT_PRESENT;
    }

    reg.BITS.CGC = 1u;
    PCC->PCCn[pcc_index].REG = reg.REG;
    return CLOCK_OK;
}

/* Bật clock cho ngoại vi cần clock chức năng (LPIT, LPSPI, LPUART, FTM...).
 * PCS chỉ được thay đổi khi CGC = 0: tắt -> chọn PCS -> bật. */
CLOCK_Status CLOCK_EnablePeripheralWithSource(uint32_t pcc_index, CLOCK_PcsSource src)
{
    PCC_REG_t reg = { .REG = 0u };

    if (pcc_index >= PCC_PCCn_COUNT) {
        return CLOCK_ERR_PARAM;
    }
    if (PCC->PCCn[pcc_index].BITS.PR == 0u) {
        return CLOCK_ERR_NOT_PRESENT;
    }

    /* 1. Tắt clock */
    PCC->PCCn[pcc_index].REG = 0u;

    /* 2. Chọn nguồn khi CGC = 0 */
    reg.BITS.PCS = (uint32_t)src;
    PCC->PCCn[pcc_index].REG = reg.REG;

    /* 3. Bật clock */
    reg.BITS.CGC = 1u;
    PCC->PCCn[pcc_index].REG = reg.REG;
    return CLOCK_OK;
}

void CLOCK_DisablePeripheral(uint32_t pcc_index)
{
    PCC_REG_t reg;

    if (pcc_index >= PCC_PCCn_COUNT) {
        return;
    }
    reg.REG = PCC->PCCn[pcc_index].REG;
    reg.BITS.CGC = 0u;
    PCC->PCCn[pcc_index].REG = reg.REG;
}


/* ------------------------------------------------------------------------
 *  Đọc tần số
 * ------------------------------------------------------------------------ */

uint32_t CLOCK_GetCoreFreq(void)
{
    SCG_CCR_t csr;
    csr.REG = SCG->CSR.REG;
    return clock_get_source_freq(csr.BITS.SCS) / (csr.BITS.DIVCORE + 1u);
}

uint32_t CLOCK_GetBusFreq(void)
{
    SCG_CCR_t csr;
    csr.REG = SCG->CSR.REG;
    return CLOCK_GetCoreFreq() / (csr.BITS.DIVBUS + 1u);
}

uint32_t CLOCK_GetSlowFreq(void)
{
    SCG_CCR_t csr;
    csr.REG = SCG->CSR.REG;
    return CLOCK_GetCoreFreq() / (csr.BITS.DIVSLOW + 1u);
}

/* Tần số DIV2 của một nguồn, tức tần số ngoại vi nhận được khi chọn PCS đó */
uint32_t CLOCK_GetDiv2Freq(CLOCK_PcsSource src)
{
    uint32_t src_freq;
    uint32_t div;

    /* Nguồn chưa bật hoặc chưa ổn định (VLD = 0) thì ngoại vi không có clock */
    switch (src) {
    case CLOCK_PCS_SOSCDIV2:
        if (SCG->SOSCCSR.BITS.SOSCVLD == 0u) { return 0u; }
        src_freq = CLOCK_SOSC_FREQ_HZ;
        div      = clock_async_div_value(SCG->SOSCDIV.BITS.DIV2);
        break;
    case CLOCK_PCS_SIRCDIV2:
        if (SCG->SIRCCSR.BITS.SIRCVLD == 0u) { return 0u; }
        src_freq = clock_get_source_freq(CLOCK_SCS_SIRC);
        div      = clock_async_div_value(SCG->SIRCDIV.BITS.DIV2);
        break;
    case CLOCK_PCS_FIRCDIV2:
        if (SCG->FIRCCSR.BITS.FIRCVLD == 0u) { return 0u; }
        src_freq = CLOCK_FIRC_FREQ_HZ;
        div      = clock_async_div_value(SCG->FIRCDIV.BITS.DIV2);
        break;
    case CLOCK_PCS_SPLLDIV2:
        if (SCG->SPLLCSR.BITS.SPLLVLD == 0u) { return 0u; }
        src_freq = clock_get_spll_freq();
        div      = clock_async_div_value(SCG->SPLLDIV.BITS.DIV2);
        break;
    default:
        return 0u;
    }

    return (div == 0u) ? 0u : (src_freq / div);
}
