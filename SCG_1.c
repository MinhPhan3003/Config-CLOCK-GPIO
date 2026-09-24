#include "SCG.h"

/* ------------------------------------------------------------------------
 *  Truy cập trực tiếp SCG->XXX.BITS.field
 *  - Mỗi dòng là 1 lần đọc-sửa-ghi 32-bit (GCC ARM: -fstrict-volatile-bitfields
 *    mặc định bật, không được tắt vì HCCR/RCCR chỉ nhận ghi 32-bit).
 *  - .REG = 0u dùng khi muốn xóa toàn bộ thanh ghi (tắt clock).
 *  - RCCR có hiệu lực ngay khi đang ở RUN -> thứ tự ghi field quan trọng.
 * ------------------------------------------------------------------------ */

/* Bước 0: về RUN + FIRC 48/48/24 MHz.
 * Cần khi chạy lại từ debugger mà chip chưa reset: khi SPLL/SOSC đang là
 * system clock thì không tắt được, SPLLCFG/SOSCCFG cũng không ghi được. */
static void clock_run_on_firc(void)
{
    if (SMC->PMSTAT.BITS.PMSTAT == SMC_PMSTAT_HSRUN) {
        CLOCK_EnterRUN();
    }

    /* Đổi nguồn TRƯỚC, bộ chia SAU: lúc đổi, DIVCORE vẫn là /2 (hoặc /1 sau reset)
     * nên core không vượt quá 48 MHz ở bất kỳ thời điểm nào. */
    SCG->RCCR.BITS.SCS = SCG_SCS_FIRC;
    while (SCG->CSR.BITS.SCS != SCG_SCS_FIRC) {}

    SCG->RCCR.BITS.DIVCORE = 0u;            /* 48 MHz */
    SCG->RCCR.BITS.DIVBUS  = 0u;            /* 48 MHz */
    SCG->RCCR.BITS.DIVSLOW = 1u;            /* 24 MHz */
}

/* SIRCDIV2 = 8 MHz, FIRCDIV2 = 48 MHz (nguồn PCS cho ngoại vi) */
static void clock_sirc_firc_div_init(void)
{
    SCG->SIRCDIV.BITS.DIV1 = SCG_DIV_1;
    SCG->SIRCDIV.BITS.DIV2 = SCG_DIV_1;
    SCG->FIRCDIV.BITS.DIV1 = SCG_DIV_1;
    SCG->FIRCDIV.BITS.DIV2 = SCG_DIV_1;
}

/* SOSC 8 MHz crystal - 28.3.8 .. 28.3.10 */
static void clock_sosc_init(void)
{
    SCG->SPLLCSR.REG = 0u;                  /* SPLL dùng SOSC -> tắt SPLL trước */
    SCG->SOSCCSR.REG = 0u;                  /* SOSCCFG chỉ ghi khi SOSCEN = 0  */

    SCG->SOSCDIV.BITS.DIV1 = SCG_DIV_1;     /* SOSCDIV1 = 8 MHz */
    SCG->SOSCDIV.BITS.DIV2 = SCG_DIV_1;     /* SOSCDIV2 = 8 MHz */

    SCG->SOSCCFG.BITS.EREFS = 1u;           /* crystal         */
    SCG->SOSCCFG.BITS.HGO   = 0u;           /* low-gain        */
    SCG->SOSCCFG.BITS.RANGE = 2u;           /* medium, 4-8 MHz */

    SCG->SOSCCSR.BITS.SOSCEN = 1u;
    while (SCG->SOSCCSR.BITS.SOSCVLD == 0u) {}
}

/* SPLL: VCO 224 MHz, SPLL_CLK 112 MHz - 28.3.17 .. 28.3.19 */
static void clock_spll_init(void)
{
    SCG->SPLLCSR.REG = 0u;                  /* SPLLCFG chỉ ghi khi SPLLEN = 0 */

    SCG->SPLLDIV.BITS.DIV1 = SCG_DIV_2;     /* SPLLDIV1 = 56 MHz */
    SCG->SPLLDIV.BITS.DIV2 = SCG_DIV_4;     /* SPLLDIV2 = 28 MHz */

    SCG->SPLLCFG.BITS.SOURCE = 0u;          /* SOSC */
    SCG->SPLLCFG.BITS.PREDIV = SPLL_PREDIV(1);
    SCG->SPLLCFG.BITS.MULT   = SPLL_MULT(28);

    SCG->SPLLCSR.BITS.SPLLEN = 1u;
    while (SCG->SPLLCSR.BITS.SPLLVLD == 0u) {}
}

/* HCCR (HSRUN) và RCCR (RUN) - cả hai lấy SPLL làm nguồn */
static void clock_ccr_init(void)
{
    /* HSRUN: 112 / 56 / 28 MHz
     * HCCR chưa có hiệu lực khi đang ở RUN -> thứ tự ghi không quan trọng */
    SCG->HCCR.BITS.SCS     = SCG_SCS_SPLL;
    SCG->HCCR.BITS.DIVCORE = 0u;            /* /1 */
    SCG->HCCR.BITS.DIVBUS  = 1u;            /* /2 */
    SCG->HCCR.BITS.DIVSLOW = 3u;            /* /4 */

    /* RUN: 56 / 28 / 18.67 MHz
     * RCCR có hiệu lực ngay -> bộ chia TRƯỚC, nguồn SAU.
     * Nếu ghi SCS = SPLL trước khi DIVCORE = /2, core sẽ chạy 112 MHz ở RUN
     * (vượt giới hạn 80 MHz). */
    SCG->RCCR.BITS.DIVSLOW = 2u;            /* /3 */
    SCG->RCCR.BITS.DIVBUS  = 1u;            /* /2 */
    SCG->RCCR.BITS.DIVCORE = 1u;            /* /2 */
    SCG->RCCR.BITS.SCS     = SCG_SCS_SPLL;
}

/* ------------------------------------------------------------------------
 *  Public
 * ------------------------------------------------------------------------ */

void CLOCK_Init(void)
{
    clock_run_on_firc();
    clock_sirc_firc_div_init();
    clock_sosc_init();
    clock_spll_init();
    clock_ccr_init();

    /* RUN đã chuyển sang SPLL */
    while (SCG->CSR.BITS.SCS != SCG_SCS_SPLL) {}

    /* PMPROT write-once sau reset: nếu cần thêm VLPR thì phải đặt
     * AVLP trong CÙNG lần ghi này (ghi .REG = 0xA0u) */
    SMC->PMPROT.BITS.AHSRUN = 1u;

    CLOCK_EnterHSRUN();
}

void CLOCK_EnterHSRUN(void)
{
    /* HSRUN chỉ vào được từ RUN */
    if (SMC->PMSTAT.BITS.PMSTAT != SMC_PMSTAT_RUN) {
        return;
    }

    SMC->PMCTRL.BITS.RUNM = SMC_RUNM_HSRUN;
    while (SMC->PMSTAT.BITS.PMSTAT != SMC_PMSTAT_HSRUN) {}
    while (SCG->CSR.REG != SCG->HCCR.REG) {}    /* SCS + 3 bộ chia đã áp dụng */
}

void CLOCK_EnterRUN(void)
{
    SMC->PMCTRL.BITS.RUNM = SMC_RUNM_RUN;
    while (SMC->PMSTAT.BITS.PMSTAT != SMC_PMSTAT_RUN) {}
    while (SCG->CSR.REG != SCG->RCCR.REG) {}
}

/* PCS chỉ ghi được khi CGC = 0: tắt -> chọn nguồn -> bật.
 * Ngoại vi không có PCS (PORTx...) thì truyền PCC_PCS_OFF. */
void CLOCK_EnablePeripheral(uint32_t pcc_index, uint32_t pcs)
{
    if (pcc_index >= PCC_PCCn_COUNT) {
        return;
    }

    PCC->PCCn[pcc_index].BITS.CGC = 0u;
    PCC->PCCn[pcc_index].BITS.PCS = pcs;
    PCC->PCCn[pcc_index].BITS.CGC = 1u;
}

void CLOCK_DisablePeripheral(uint32_t pcc_index)
{
    if (pcc_index >= PCC_PCCn_COUNT) {
        return;
    }

    PCC->PCCn[pcc_index].BITS.CGC = 0u;
}