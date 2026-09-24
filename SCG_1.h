#ifndef SCG_H_
#define SCG_H_

#include <stdint.h>
#include <stddef.h>

/* ==========================================================================
 *  Clock plan - S32K144, SOSC = 8 MHz crystal
 *
 *  SPLL : VCO      = 8 MHz / (PREDIV + 1) * (MULT + 16) = 8 / 1 * 28 = 224 MHz
 *         SPLL_CLK = VCO / 2                                        = 112 MHz
 *         SPLLDIV1 = /2 -> 56 MHz,  SPLLDIV2 = /4 -> 28 MHz
 *
 *  HSRUN (HCCR): SPLL, CORE /1 = 112 MHz, BUS /2 = 56 MHz, SLOW /4 = 28 MHz
 *  RUN   (RCCR): SPLL, CORE /2 =  56 MHz, BUS /2 = 28 MHz, SLOW /3 = 18.67 MHz
 *                (RUN mode limit with SPLL: core 80, bus 40, flash 26.67 MHz)
 *
 *  Flash program/erase is not allowed in HSRUN -> call CLOCK_EnterRUN() first,
 *  then CLOCK_EnterHSRUN() afterwards.
 * ========================================================================== */

#define CLOCK_SOSC_FREQ_HZ      8000000u    /* External crystal          */
#define CLOCK_FIRC_FREQ_HZ      48000000u   /* FIRCCFG.RANGE = 0         */
#define CLOCK_SIRC_HIGH_HZ      8000000u    /* SIRCCFG.RANGE = 1 (reset) */
#define CLOCK_SIRC_LOW_HZ       2000000u    /* SIRCCFG.RANGE = 0         */

/* SCS code (28.3.4) - cũng dùng cho CLKOUTCNFG.CLKOUTSEL */
#define SCG_SCS_SOSC            1u
#define SCG_SCS_SIRC            2u
#define SCG_SCS_FIRC            3u
#define SCG_SCS_SPLL            6u

/* Mã DIV1 / DIV2 của SOSCDIV, SIRCDIV, FIRCDIV, SPLLDIV (28.3.9) */
#define SCG_DIV_OFF             0u
#define SCG_DIV_1               1u
#define SCG_DIV_2               2u
#define SCG_DIV_4               3u
#define SCG_DIV_8               4u
#define SCG_DIV_16              5u
#define SCG_DIV_32              6u
#define SCG_DIV_64              7u

/* SPLLCFG (28.3.19): nhập hệ số thật, macro đổi sang giá trị field */
#define SPLL_PREDIV(x)          ((uint32_t)(x) - 1u)    /* x = 1..8   */
#define SPLL_MULT(x)            ((uint32_t)(x) - 16u)   /* x = 16..47 */

/* SMC (Chapter 39) */
#define SMC_RUNM_RUN            0u
#define SMC_RUNM_HSRUN          3u
#define SMC_PMSTAT_RUN          0x01u
#define SMC_PMSTAT_HSRUN        0x80u

/* PCC.PCS (29.6.x) */
#define PCC_PCS_OFF             0u
#define PCC_PCS_SOSCDIV2        1u
#define PCC_PCS_SIRCDIV2        2u
#define PCC_PCS_FIRCDIV2        3u
#define PCC_PCS_SPLLDIV2        6u

#define CLOCK_STATIC_ASSERT(cond, name)  typedef char clock_sa_##name[(cond) ? 1 : -1]

/* ========================================================================
 *  SCG - Register definition
 * ======================================================================== */

/* CSR / RCCR / VCCR / HCCR - 28.3.3 .. 28.3.6 (chỉ ghi 32-bit) */
typedef union {
    uint32_t REG;
    struct {
        uint32_t DIVSLOW : 4;   /* bit 0-3   : SLOW = CORE / (DIVSLOW + 1) */
        uint32_t DIVBUS  : 4;   /* bit 4-7   : BUS  = CORE / (DIVBUS + 1)  */
        uint32_t         : 8;   /* bit 8-15  */
        uint32_t DIVCORE : 4;   /* bit 16-19 : CORE = SRC  / (DIVCORE + 1) */
        uint32_t         : 4;   /* bit 20-23 */
        uint32_t SCS     : 4;   /* bit 24-27 : system clock source         */
        uint32_t         : 4;   /* bit 28-31 */
    } BITS;
} SCG_CCR_t;

/* CLKOUTCNFG - 28.3.7 */
typedef union {
    uint32_t REG;
    struct {
        uint32_t           : 24;
        uint32_t CLKOUTSEL : 4;   /* bit 24-27 */
        uint32_t           : 4;
    } BITS;
} SCG_CLKOUTCNFG_t;

/* SOSCCSR - 28.3.8 */
typedef union {
    uint32_t REG;
    struct {
        uint32_t SOSCEN   : 1;    /* bit 0  */
        uint32_t          : 15;
        uint32_t SOSCCM   : 1;    /* bit 16 */
        uint32_t SOSCCMRE : 1;    /* bit 17 */
        uint32_t          : 5;
        uint32_t LK       : 1;    /* bit 23 */
        uint32_t SOSCVLD  : 1;    /* bit 24 (RO)  */
        uint32_t SOSCSEL  : 1;    /* bit 25 (RO)  */
        uint32_t SOSCERR  : 1;    /* bit 26 (w1c) */
        uint32_t          : 5;
    } BITS;
} SCG_SOSCCSR_t;

/* SOSCDIV / SIRCDIV / FIRCDIV / SPLLDIV - cùng bố cục */
typedef union {
    uint32_t REG;
    struct {
        uint32_t DIV1 : 3;        /* bit 0-2  */
        uint32_t      : 5;
        uint32_t DIV2 : 3;        /* bit 8-10 */
        uint32_t      : 21;
    } BITS;
} SCG_DIV_t;

/* SOSCCFG - 28.3.10 (chỉ ghi khi SOSCEN = 0) */
typedef union {
    uint32_t REG;
    struct {
        uint32_t       : 2;
        uint32_t EREFS : 1;       /* bit 2   : 1 = crystal   */
        uint32_t HGO   : 1;       /* bit 3   : 1 = high-gain */
        uint32_t RANGE : 2;       /* bit 4-5 */
        uint32_t       : 26;
    } BITS;
} SCG_SOSCCFG_t;

/* SIRCCSR - 28.3.11 */
typedef union {
    uint32_t REG;
    struct {
        uint32_t SIRCEN   : 1;    /* bit 0  */
        uint32_t SIRCSTEN : 1;    /* bit 1  */
        uint32_t SIRCLPEN : 1;    /* bit 2  */
        uint32_t          : 20;
        uint32_t LK       : 1;    /* bit 23 */
        uint32_t SIRCVLD  : 1;    /* bit 24 */
        uint32_t SIRCSEL  : 1;    /* bit 25 */
        uint32_t          : 6;
    } BITS;
} SCG_SIRCCSR_t;

/* SIRCCFG - 28.3.13 */
typedef union {
    uint32_t REG;
    struct {
        uint32_t RANGE : 1;       /* 0 = 2 MHz, 1 = 8 MHz */
        uint32_t       : 31;
    } BITS;
} SCG_SIRCCFG_t;

/* FIRCCSR - 28.3.14 */
typedef union {
    uint32_t REG;
    struct {
        uint32_t FIRCEN     : 1;  /* bit 0  */
        uint32_t            : 2;
        uint32_t FIRCREGOFF : 1;  /* bit 3  */
        uint32_t            : 19;
        uint32_t LK         : 1;  /* bit 23 */
        uint32_t FIRCVLD    : 1;  /* bit 24 */
        uint32_t FIRCSEL    : 1;  /* bit 25 */
        uint32_t FIRCERR    : 1;  /* bit 26 (w1c) */
        uint32_t            : 5;
    } BITS;
} SCG_FIRCCSR_t;

/* FIRCCFG - 28.3.16 */
typedef union {
    uint32_t REG;
    struct {
        uint32_t RANGE : 2;       /* 00 = 48 MHz */
        uint32_t       : 30;
    } BITS;
} SCG_FIRCCFG_t;

/* SPLLCSR - 28.3.17 */
typedef union {
    uint32_t REG;
    struct {
        uint32_t SPLLEN   : 1;    /* bit 0  */
        uint32_t          : 15;
        uint32_t SPLLCM   : 1;    /* bit 16 */
        uint32_t SPLLCMRE : 1;    /* bit 17 */
        uint32_t          : 5;
        uint32_t LK       : 1;    /* bit 23 */
        uint32_t SPLLVLD  : 1;    /* bit 24 (RO)  */
        uint32_t SPLLSEL  : 1;    /* bit 25 (RO)  */
        uint32_t SPLLERR  : 1;    /* bit 26 (w1c) */
        uint32_t          : 5;
    } BITS;
} SCG_SPLLCSR_t;

/* SPLLCFG - 28.3.19 (chỉ ghi khi SPLLEN = 0) */
typedef union {
    uint32_t REG;
    struct {
        uint32_t SOURCE : 1;      /* bit 0    : 0 = SOSC */
        uint32_t        : 7;
        uint32_t PREDIV : 3;      /* bit 8-10  */
        uint32_t        : 5;
        uint32_t MULT   : 5;      /* bit 16-20 */
        uint32_t        : 11;
    } BITS;
} SCG_SPLLCFG_t;

typedef struct {
    uint32_t          VERID;          /* 0x000 */
    uint32_t          PARAM;          /* 0x004 */
    uint32_t          RESERVED0[2];   /* 0x008 */
    const SCG_CCR_t   CSR;            /* 0x010 (read only) */
    SCG_CCR_t         RCCR;           /* 0x014 */
    SCG_CCR_t         VCCR;           /* 0x018 */
    SCG_CCR_t         HCCR;           /* 0x01C */
    SCG_CLKOUTCNFG_t  CLKOUTCNFG;     /* 0x020 */
    uint32_t          RESERVED1[55];  /* 0x024 */
    SCG_SOSCCSR_t     SOSCCSR;        /* 0x100 */
    SCG_DIV_t         SOSCDIV;        /* 0x104 */
    SCG_SOSCCFG_t     SOSCCFG;        /* 0x108 */
    uint32_t          RESERVED2[61];  /* 0x10C */
    SCG_SIRCCSR_t     SIRCCSR;        /* 0x200 */
    SCG_DIV_t         SIRCDIV;        /* 0x204 */
    SCG_SIRCCFG_t     SIRCCFG;        /* 0x208 */
    uint32_t          RESERVED3[61];  /* 0x20C */
    SCG_FIRCCSR_t     FIRCCSR;        /* 0x300 */
    SCG_DIV_t         FIRCDIV;        /* 0x304 */
    SCG_FIRCCFG_t     FIRCCFG;        /* 0x308 */
    uint32_t          RESERVED4[189]; /* 0x30C */
    SCG_SPLLCSR_t     SPLLCSR;        /* 0x600 */
    SCG_DIV_t         SPLLDIV;        /* 0x604 */
    SCG_SPLLCFG_t     SPLLCFG;        /* 0x608 */
} SCG_typedef;

#define SCG                     ((volatile SCG_typedef *)0x40064000UL)

CLOCK_STATIC_ASSERT(offsetof(SCG_typedef, HCCR)    == 0x01Cu, scg_hccr);
CLOCK_STATIC_ASSERT(offsetof(SCG_typedef, SOSCCSR) == 0x100u, scg_sosccsr);
CLOCK_STATIC_ASSERT(offsetof(SCG_typedef, FIRCCSR) == 0x300u, scg_firccsr);
CLOCK_STATIC_ASSERT(offsetof(SCG_typedef, SPLLCFG) == 0x608u, scg_spllcfg);

/* ========================================================================
 *  SMC - Register definition (Chapter 39)
 * ======================================================================== */

/* PMPROT - write-once sau reset */
typedef union {
    uint32_t REG;
    struct {
        uint32_t        : 5;
        uint32_t AVLP   : 1;      /* bit 5 : cho phép VLPR/VLPS */
        uint32_t        : 1;
        uint32_t AHSRUN : 1;      /* bit 7 : cho phép HSRUN     */
        uint32_t        : 24;
    } BITS;
} SMC_PMPROT_t;

/* PMCTRL */
typedef union {
    uint32_t REG;
    struct {
        uint32_t STOPM : 3;       /* bit 0-2 */
        uint32_t VLPSA : 1;       /* bit 3 (RO) */
        uint32_t       : 1;
        uint32_t RUNM  : 2;       /* bit 5-6 : 00 RUN, 10 VLPR, 11 HSRUN */
        uint32_t       : 25;
    } BITS;
} SMC_PMCTRL_t;

/* PMSTAT (RO) */
typedef union {
    uint32_t REG;
    struct {
        uint32_t PMSTAT : 8;      /* 0x01 RUN, 0x04 VLPR, 0x80 HSRUN */
        uint32_t        : 24;
    } BITS;
} SMC_PMSTAT_t;

typedef struct {
    uint32_t            VERID;      /* 0x00 */
    uint32_t            PARAM;      /* 0x04 */
    SMC_PMPROT_t        PMPROT;     /* 0x08 */
    SMC_PMCTRL_t        PMCTRL;     /* 0x0C */
    uint32_t            STOPCTRL;   /* 0x10 */
    const SMC_PMSTAT_t  PMSTAT;     /* 0x14 */
} SMC_typedef;

#define SMC                     ((volatile SMC_typedef *)0x4007E000UL)

CLOCK_STATIC_ASSERT(offsetof(SMC_typedef, PMSTAT) == 0x14u, smc_pmstat);

/* ========================================================================
 *  PCC - Register definition (Chapter 29)
 * ======================================================================== */

typedef union {
    uint32_t REG;
    struct {
        uint32_t PCD   : 3;       /* bit 0-2   */
        uint32_t FRAC  : 1;       /* bit 3     */
        uint32_t       : 20;
        uint32_t PCS   : 3;       /* bit 24-26 */
        uint32_t       : 2;
        uint32_t INUSE : 1;       /* bit 29    */
        uint32_t CGC   : 1;       /* bit 30    */
        uint32_t PR    : 1;       /* bit 31 (RO) */
    } BITS;
} PCC_REG_t;

#define PCC_PCCn_COUNT          116u

typedef struct {
    PCC_REG_t PCCn[PCC_PCCn_COUNT];
} PCC_typedef;

#define PCC                     ((volatile PCC_typedef *)0x40065000UL)

/* Index = offset / 4 (bảng 29.6.1) */
#define PCC_FLEXCAN0_INDEX      36u     /* 0x090 */
#define PCC_FLEXCAN1_INDEX      37u     /* 0x094 */
#define PCC_FTM3_INDEX          38u     /* 0x098 */
#define PCC_ADC1_INDEX          39u     /* 0x09C */
#define PCC_FLEXCAN2_INDEX      43u     /* 0x0AC */
#define PCC_LPSPI0_INDEX        44u     /* 0x0B0 */
#define PCC_LPSPI1_INDEX        45u     /* 0x0B4 */
#define PCC_LPSPI2_INDEX        46u     /* 0x0B8 */
#define PCC_LPIT_INDEX          55u     /* 0x0DC */
#define PCC_FTM0_INDEX          56u     /* 0x0E0 */
#define PCC_FTM1_INDEX          57u     /* 0x0E4 */
#define PCC_FTM2_INDEX          58u     /* 0x0E8 */
#define PCC_ADC0_INDEX          59u     /* 0x0EC */
#define PCC_LPTMR0_INDEX        64u     /* 0x100 */
#define PCC_PORTA_INDEX         73u     /* 0x124 */
#define PCC_PORTB_INDEX         74u     /* 0x128 */
#define PCC_PORTC_INDEX         75u     /* 0x12C */
#define PCC_PORTD_INDEX         76u     /* 0x130 */
#define PCC_PORTE_INDEX         77u     /* 0x134 */
#define PCC_LPI2C0_INDEX        102u    /* 0x198 */
#define PCC_LPUART0_INDEX       106u    /* 0x1A8 */
#define PCC_LPUART1_INDEX       107u    /* 0x1AC */
#define PCC_LPUART2_INDEX       108u    /* 0x1B0 */

CLOCK_STATIC_ASSERT(offsetof(PCC_typedef, PCCn[PCC_LPIT_INDEX])  == 0x0DCu, pcc_lpit);
CLOCK_STATIC_ASSERT(offsetof(PCC_typedef, PCCn[PCC_PORTD_INDEX]) == 0x130u, pcc_portd);

/* ========================================================================
 *  API
 * ======================================================================== */

void CLOCK_Init(void);          /* SOSC -> SPLL -> RCCR/HCCR -> vào HSRUN 112 MHz */
void CLOCK_EnterHSRUN(void);    /* RUN   -> HSRUN (112 / 56 / 28)    */
void CLOCK_EnterRUN(void);      /* HSRUN -> RUN   (56 / 28 / 18.67)  */

void CLOCK_EnablePeripheral(uint32_t pcc_index, uint32_t pcs);  /* pcs = PCC_PCS_xxx */
void CLOCK_DisablePeripheral(uint32_t pcc_index);

#endif /* SCG_H_ */