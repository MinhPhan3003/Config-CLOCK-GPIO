/**
 * @file    clock.h
 * @brief   Cấu hình clock cho S32K144: SCG (nguồn clock, clock hệ thống)
 *          và PCC (bật clock cho từng ngoại vi).
 *          Trên S32K144, hai khối này thay cho RCC của STM32.
 *
 * Tham chiếu: S32K1xx Series Reference Manual (S32K1XXRM), Rev. 13, 04/2020
 *   Chương 27 Clock Distribution
 *     27.2   High level clocking diagram ............. tr. 555
 *     27.4   Internal clocking requirements .......... tr. 558
 *     27.4.1 Clock divider values after reset ........ tr. 563
 *   Chương 28 System Clock Generator (SCG)
 *     28.1.2 Supported frequency ranges .............. tr. 579
 *     28.1.3 Oscillator and SPLL guidelines .......... tr. 580
 *     28.1.4 System clock switching .................. tr. 580
 *     28.3   Memory map / register definition ........ tr. 583
 *   Chương 29 Peripheral Clock Controller (PCC)
 *     29.4   Functional description .................. tr. 623
 *     29.6.1 PCC memory map .......................... tr. 623
 *
 * Lưu ý:
 *   - Không include chung với S32K144.h của SDK trong cùng một file .c
 *     (trùng tên SCG, PCC).
 *   - Đối chiếu mọi offset và vị trí bit với RM trước khi đưa vào báo cáo.
 */

#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>
#include <stddef.h>

/* Kiểm tra offset/kích thước lúc biên dịch (C11) */
#define CLOCK_STATIC_ASSERT(cond, msg)  _Static_assert(cond, msg)

/* ========================================================================
 *  THÔNG SỐ BOARD
 * ======================================================================== */
/* Thạch anh trên S32K144EVB (xem schematic/user guide của board) */
#define CLOCK_SOSC_FREQ_HZ      8000000u
#define CLOCK_FIRC_FREQ_HZ      48000000u   /* FIRCCFG.RANGE = 0 */
#define CLOCK_SIRC_HIGH_HZ      8000000u    /* SIRCCFG.RANGE = 1 */
#define CLOCK_SIRC_LOW_HZ       2000000u    /* SIRCCFG.RANGE = 0 */


/* ========================================================================
 *  SCG - ĐỊNH NGHĨA THANH GHI
 * ======================================================================== */

/* CSR / RCCR / VCCR / HCCR có chung bố cục field
 * 28.3.3 CSR tr.585, 28.3.4 RCCR tr.588, 28.3.5 VCCR tr.590, 28.3.6 HCCR tr.592 */
typedef union {
    uint32_t REG;
    struct {
        uint32_t DIVSLOW : 4;   /* bit 0-3   : SLOW = CORE / (DIVSLOW + 1) */
        uint32_t DIVBUS  : 4;   /* bit 4-7   : BUS  = CORE / (DIVBUS + 1)  */
        uint32_t         : 8;   /* bit 8-15  */
        uint32_t DIVCORE : 4;   /* bit 16-19 : CORE = SRC  / (DIVCORE + 1) */
        uint32_t         : 4;   /* bit 20-23 */
        uint32_t SCS     : 4;   /* bit 24-27 : nguồn clock hệ thống        */
        uint32_t         : 4;   /* bit 28-31 */
    } BITS;
} SCG_CCR_t;

/* CLKOUTCNFG - 28.3.7 tr.594 */
typedef union {
    uint32_t REG;
    struct {
        uint32_t           : 24;  /* bit 0-23  */
        uint32_t CLKOUTSEL : 4;   /* bit 24-27 */
        uint32_t           : 4;   /* bit 28-31 */
    } BITS;
} SCG_CLKOUTCNFG_t;

/* SOSCCSR - 28.3.8 tr.596 */
typedef union {
    uint32_t REG;
    struct {
        uint32_t SOSCEN   : 1;    /* bit 0     : bật SOSC                    */
        uint32_t          : 15;   /* bit 1-15  */
        uint32_t SOSCCM   : 1;    /* bit 16    : bật clock monitor          */
        uint32_t SOSCCMRE : 1;    /* bit 17    : monitor gây reset          */
        uint32_t          : 5;    /* bit 18-22 */
        uint32_t LK       : 1;    /* bit 23    : khóa thanh ghi             */
        uint32_t SOSCVLD  : 1;    /* bit 24    : SOSC đã ổn định (RO)       */
        uint32_t SOSCSEL  : 1;    /* bit 25    : SOSC đang là system clock  */
        uint32_t SOSCERR  : 1;    /* bit 26    : lỗi clock (w1c)            */
        uint32_t          : 5;    /* bit 27-31 */
    } BITS;
} SCG_SOSCCSR_t;

/* SOSCDIV / SIRCDIV / FIRCDIV / SPLLDIV có chung bố cục
 * 28.3.9 tr.598, 28.3.12 tr.602, 28.3.15 tr.606, 28.3.18 tr.610 */
typedef union {
    uint32_t REG;
    struct {
        uint32_t DIV1 : 3;        /* bit 0-2  */
        uint32_t      : 5;        /* bit 3-7  */
        uint32_t DIV2 : 3;        /* bit 8-10 */
        uint32_t      : 21;       /* bit 11-31 */
    } BITS;
} SCG_DIV_t;

/* SOSCCFG - 28.3.10 tr.599 (chỉ ghi khi SOSC đang tắt) */
typedef union {
    uint32_t REG;
    struct {
        uint32_t       : 2;       /* bit 0-1  */
        uint32_t EREFS : 1;       /* bit 2    : 1 = dùng thạch anh          */
        uint32_t HGO   : 1;       /* bit 3    : 1 = high-gain               */
        uint32_t RANGE : 2;       /* bit 4-5  : dải tần thạch anh           */
        uint32_t       : 26;      /* bit 6-31 */
    } BITS;
} SCG_SOSCCFG_t;

/* SIRCCSR - 28.3.11 tr.601 */
typedef union {
    uint32_t REG;
    struct {
        uint32_t SIRCEN   : 1;    /* bit 0     */
        uint32_t SIRCSTEN : 1;    /* bit 1     : chạy trong STOP mode        */
        uint32_t SIRCLPEN : 1;    /* bit 2     : chạy trong VLP mode         */
        uint32_t          : 20;   /* bit 3-22  */
        uint32_t LK       : 1;    /* bit 23    */
        uint32_t SIRCVLD  : 1;    /* bit 24    */
        uint32_t SIRCSEL  : 1;    /* bit 25    */
        uint32_t          : 6;    /* bit 26-31 */
    } BITS;
} SCG_SIRCCSR_t;

/* SIRCCFG - 28.3.13 tr.603 */
typedef union {
    uint32_t REG;
    struct {
        uint32_t RANGE : 1;       /* bit 0 : 0 = 2 MHz, 1 = 8 MHz */
        uint32_t       : 31;
    } BITS;
} SCG_SIRCCFG_t;

/* FIRCCSR - 28.3.14 tr.604 */
typedef union {
    uint32_t REG;
    struct {
        uint32_t FIRCEN     : 1;  /* bit 0     */
        uint32_t            : 2;  /* bit 1-2   */
        uint32_t FIRCREGOFF : 1;  /* bit 3     */
        uint32_t            : 19; /* bit 4-22  */
        uint32_t LK         : 1;  /* bit 23    */
        uint32_t FIRCVLD    : 1;  /* bit 24    */
        uint32_t FIRCSEL    : 1;  /* bit 25    */
        uint32_t FIRCERR    : 1;  /* bit 26 (w1c) */
        uint32_t            : 5;  /* bit 27-31 */
    } BITS;
} SCG_FIRCCSR_t;

/* FIRCCFG - 28.3.16 tr.607 */
typedef union {
    uint32_t REG;
    struct {
        uint32_t RANGE : 2;       /* bit 0-1 : 00 = 48 MHz */
        uint32_t       : 30;
    } BITS;
} SCG_FIRCCFG_t;

/* SPLLCSR - 28.3.17 tr.608 */
typedef union {
    uint32_t REG;
    struct {
        uint32_t SPLLEN   : 1;    /* bit 0     */
        uint32_t          : 15;   /* bit 1-15  */
        uint32_t SPLLCM   : 1;    /* bit 16    */
        uint32_t SPLLCMRE : 1;    /* bit 17    */
        uint32_t          : 5;    /* bit 18-22 */
        uint32_t LK       : 1;    /* bit 23    */
        uint32_t SPLLVLD  : 1;    /* bit 24    : PLL đã khóa pha (RO)       */
        uint32_t SPLLSEL  : 1;    /* bit 25    */
        uint32_t SPLLERR  : 1;    /* bit 26 (w1c) */
        uint32_t          : 5;    /* bit 27-31 */
    } BITS;
} SCG_SPLLCSR_t;

/* SPLLCFG - 28.3.19 tr.611 (chỉ ghi khi SPLL đang tắt)
 * VCO      = SOSC / (PREDIV + 1) * (MULT + 16)
 * SPLL_CLK = VCO / 2                                                    */
typedef union {
    uint32_t REG;
    struct {
        uint32_t SOURCE : 1;      /* bit 0     : 0 = SOSC                   */
        uint32_t        : 7;      /* bit 1-7   */
        uint32_t PREDIV : 3;      /* bit 8-10  */
        uint32_t        : 5;      /* bit 11-15 */
        uint32_t MULT   : 5;      /* bit 16-20 */
        uint32_t        : 11;     /* bit 21-31 */
    } BITS;
} SCG_SPLLCFG_t;

/* Bố cục khối SCG - memory map 28.3, tr.583 */
typedef struct {
    uint32_t          VERID;          /* 0x000 */
    uint32_t          PARAM;          /* 0x004 */
    uint32_t          RESERVED0[2];   /* 0x008 - 0x00F */
    SCG_CCR_t         CSR;            /* 0x010 (chỉ đọc) */
    SCG_CCR_t         RCCR;           /* 0x014 */
    SCG_CCR_t         VCCR;           /* 0x018 */
    SCG_CCR_t         HCCR;           /* 0x01C */
    SCG_CLKOUTCNFG_t  CLKOUTCNFG;     /* 0x020 */
    uint32_t          RESERVED1[55];  /* 0x024 - 0x0FF */
    SCG_SOSCCSR_t     SOSCCSR;        /* 0x100 */
    SCG_DIV_t         SOSCDIV;        /* 0x104 */
    SCG_SOSCCFG_t     SOSCCFG;        /* 0x108 */
    uint32_t          RESERVED2[61];  /* 0x10C - 0x1FF */
    SCG_SIRCCSR_t     SIRCCSR;        /* 0x200 */
    SCG_DIV_t         SIRCDIV;        /* 0x204 */
    SCG_SIRCCFG_t     SIRCCFG;        /* 0x208 */
    uint32_t          RESERVED3[61];  /* 0x20C - 0x2FF */
    SCG_FIRCCSR_t     FIRCCSR;        /* 0x300 */
    SCG_DIV_t         FIRCDIV;        /* 0x304 */
    SCG_FIRCCFG_t     FIRCCFG;        /* 0x308 */
    uint32_t          RESERVED4[189]; /* 0x30C - 0x5FF */
    SCG_SPLLCSR_t     SPLLCSR;        /* 0x600 */
    SCG_DIV_t         SPLLDIV;        /* 0x604 */
    SCG_SPLLCFG_t     SPLLCFG;        /* 0x608 */
} SCG_typedef;

CLOCK_STATIC_ASSERT(sizeof(SCG_CCR_t)     == 4u, "SCG_CCR_t phai 32 bit");
CLOCK_STATIC_ASSERT(sizeof(SCG_SOSCCSR_t) == 4u, "SCG_SOSCCSR_t phai 32 bit");
CLOCK_STATIC_ASSERT(sizeof(SCG_SPLLCFG_t) == 4u, "SCG_SPLLCFG_t phai 32 bit");
CLOCK_STATIC_ASSERT(offsetof(SCG_typedef, CSR)        == 0x010u, "SCG CSR sai offset");
CLOCK_STATIC_ASSERT(offsetof(SCG_typedef, CLKOUTCNFG) == 0x020u, "SCG CLKOUTCNFG sai offset");
CLOCK_STATIC_ASSERT(offsetof(SCG_typedef, SOSCCSR)    == 0x100u, "SCG SOSCCSR sai offset");
CLOCK_STATIC_ASSERT(offsetof(SCG_typedef, SIRCCSR)    == 0x200u, "SCG SIRCCSR sai offset");
CLOCK_STATIC_ASSERT(offsetof(SCG_typedef, FIRCCSR)    == 0x300u, "SCG FIRCCSR sai offset");
CLOCK_STATIC_ASSERT(offsetof(SCG_typedef, SPLLCSR)    == 0x600u, "SCG SPLLCSR sai offset");
CLOCK_STATIC_ASSERT(offsetof(SCG_typedef, SPLLCFG)    == 0x608u, "SCG SPLLCFG sai offset");

#define ADDRESS_SCG             0x40064000UL
#define SCG                     ((volatile SCG_typedef *)ADDRESS_SCG)


/* ========================================================================
 *  PCC - ĐỊNH NGHĨA THANH GHI
 * ======================================================================== */

/* Mỗi ngoại vi một thanh ghi PCC, bố cục chung (29.6.x) */
typedef union {
    uint32_t REG;
    struct {
        uint32_t PCD   : 3;       /* bit 0-2   : bộ chia (chỉ một số ngoại vi) */
        uint32_t FRAC  : 1;       /* bit 3     */
        uint32_t       : 20;      /* bit 4-23  */
        uint32_t PCS   : 3;       /* bit 24-26 : nguồn clock chức năng     */
        uint32_t       : 2;       /* bit 27-28 */
        uint32_t INUSE : 1;       /* bit 29    */
        uint32_t CGC   : 1;       /* bit 30    : bật clock                  */
        uint32_t PR    : 1;       /* bit 31    : ngoại vi có tồn tại (RO)   */
    } BITS;
} PCC_REG_t;

#define PCC_PCCn_COUNT          116u

typedef struct {
    PCC_REG_t PCCn[PCC_PCCn_COUNT];
} PCC_typedef;

CLOCK_STATIC_ASSERT(sizeof(PCC_REG_t) == 4u, "PCC_REG_t phai 32 bit");

#define ADDRESS_PCC             0x40065000UL
#define PCC                     ((volatile PCC_typedef *)ADDRESS_PCC)

/* Chỉ số = offset / 4, offset tra bảng 29.6.1 (tr.623) */
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

CLOCK_STATIC_ASSERT(offsetof(PCC_typedef, PCCn[PCC_LPIT_INDEX])  == 0x0DCu, "PCC_LPIT sai offset");
CLOCK_STATIC_ASSERT(offsetof(PCC_typedef, PCCn[PCC_PORTD_INDEX]) == 0x130u, "PCC_PORTD sai offset");


/* ========================================================================
 *  KIỂU DỮ LIỆU CHO DRIVER
 * ======================================================================== */

/* Giá trị field SCS (28.3.4) */
typedef enum {
    CLOCK_SCS_SOSC = 1u,
    CLOCK_SCS_SIRC = 2u,
    CLOCK_SCS_FIRC = 3u,
    CLOCK_SCS_SPLL = 6u
} CLOCK_Scs;

/* Mã bộ chia DIV1/DIV2 của các nguồn bất đồng bộ (28.3.9) */
typedef enum {
    CLOCK_DIV_DISABLE = 0u,
    CLOCK_DIV_1       = 1u,
    CLOCK_DIV_2       = 2u,
    CLOCK_DIV_4       = 3u,
    CLOCK_DIV_8       = 4u,
    CLOCK_DIV_16      = 5u,
    CLOCK_DIV_32      = 6u,
    CLOCK_DIV_64      = 7u
} CLOCK_AsyncDiv;

/* Giá trị field PCS trong PCC (29.6.x) */
typedef enum {
    CLOCK_PCS_OFF      = 0u,
    CLOCK_PCS_SOSCDIV2 = 1u,
    CLOCK_PCS_SIRCDIV2 = 2u,
    CLOCK_PCS_FIRCDIV2 = 3u,
    CLOCK_PCS_SPLLDIV2 = 6u
} CLOCK_PcsSource;

typedef enum {
    CLOCK_OK = 0,
    CLOCK_ERR_TIMEOUT,      /* chờ VLD hoặc chuyển clock quá lâu */
    CLOCK_ERR_LOCKED,       /* thanh ghi đang bị khóa (LK = 1)   */
    CLOCK_ERR_NOT_PRESENT,  /* ngoại vi không có trên chip       */
    CLOCK_ERR_PARAM,
    CLOCK_ERR_NOT_VALID     /* nguồn clock chưa ổn định (VLD = 0) */
} CLOCK_Status;

/* ------------------------------------------------------------------------
 * Clock monitor cho SOSC và SPLL
 *   SOSCCSR[SOSCCM, SOSCCMRE] 28.3.8 tr.596
 *   SPLLCSR[SPLLCM, SPLLCMRE] 28.3.17 tr.608
 *   Ràng buộc khi dùng monitor: 28.1.5 tr.581
 *   Tổng quan an toàn clock:    6.2.4 Clock monitoring tr.109
 *   Nguồn reset tương ứng trong RCM_SRS (26.4.3 tr.544):
 *     SOSC mất clock -> LOC (Loss Of Clock)
 *     SPLL mất khóa  -> LOL (Loss Of Lock)
 * ------------------------------------------------------------------------ */
typedef uint8_t CLOCK_MonitorMode;

#define CLOCK_CM_DISABLE        (0u)    /* CM = 0                          */
#define CLOCK_CM_INTERRUPT      (1u)    /* CM = 1, CMRE = 0: báo ngắt SCG  */
#define CLOCK_CM_RESET          (2u)    /* CM = 1, CMRE = 1: reset chip    */

/* Chế độ monitor mà CLOCK_Init_80MHz() sẽ bật cho SOSC và SPLL.
 * Có thể định nghĩa lại trước khi include clock.h, hoặc trong
 * cài đặt compiler (-DCLOCK_INIT_MONITOR_MODE=0u để tắt). */
#ifndef CLOCK_INIT_MONITOR_MODE
#define CLOCK_INIT_MONITOR_MODE CLOCK_CM_RESET
#endif


/* ========================================================================
 *  API
 * ======================================================================== */

/* Nguồn clock */
CLOCK_Status CLOCK_SOSC_Init_8MHz(void);
CLOCK_Status CLOCK_SPLL_Init_160MHz(void);
void         CLOCK_SIRC_SetDividers(CLOCK_AsyncDiv div1, CLOCK_AsyncDiv div2);
void         CLOCK_FIRC_SetDividers(CLOCK_AsyncDiv div1, CLOCK_AsyncDiv div2);

/* Clock monitor SOSC / SPLL (chỉ bật được khi nguồn đã VLD) */
CLOCK_Status CLOCK_SOSC_SetMonitor(CLOCK_MonitorMode mode);
CLOCK_Status CLOCK_SPLL_SetMonitor(CLOCK_MonitorMode mode);
uint8_t      CLOCK_SOSC_HasError(void);     /* 1 = monitor đã phát hiện lỗi */
uint8_t      CLOCK_SPLL_HasError(void);
void         CLOCK_SOSC_ClearError(void);   /* ghi 1 vào SOSCERR (w1c) */
void         CLOCK_SPLL_ClearError(void);   /* ghi 1 vào SPLLERR (w1c) */

/* Clock hệ thống */
CLOCK_Status CLOCK_RunMode_80MHz(void);
CLOCK_Status CLOCK_Init_80MHz(void);    /* SOSC -> SPLL -> RUN 80 MHz */

/* Clock ngoại vi (PCC) */
CLOCK_Status CLOCK_EnablePeripheral(uint32_t pcc_index);
CLOCK_Status CLOCK_EnablePeripheralWithSource(uint32_t pcc_index, CLOCK_PcsSource src);
void         CLOCK_DisablePeripheral(uint32_t pcc_index);

/* Đọc tần số hiện tại (Hz) */
uint32_t CLOCK_GetCoreFreq(void);
uint32_t CLOCK_GetBusFreq(void);
uint32_t CLOCK_GetSlowFreq(void);
uint32_t CLOCK_GetDiv2Freq(CLOCK_PcsSource src);   /* dùng cho ngoại vi chọn PCS */

#endif /* CLOCK_H */
