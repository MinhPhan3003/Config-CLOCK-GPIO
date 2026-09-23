/**
 * @file    gpio.h
 * @brief   Định nghĩa thanh ghi PORT + GPIO và API driver GPIO cho S32K144.
 *
 * Trên S32K144, một chân I/O được điều khiển bởi HAI khối:
 *   - PORT (PORTA..PORTE): chọn chức năng chân (MUX), pull, ngắt theo chân.
 *   - GPIO (PTA..PTE)    : hướng vào/ra, ghi và đọc mức logic.
 *
 * Tham chiếu: S32K1xx Series Reference Manual (S32K1XXRM), Rev. 13, 04/2020
 *   Chương 12 Port Control and Interrupts (PORT)
 *     12.1.1 Number of PCRs .......................... tr. 205
 *     12.1.3 I/O configuration sequence .............. tr. 206
 *     12.5   Memory map and register definition ...... tr. 210
 *     12.5.1 PORT_PCRn ............................... tr. 213
 *   Chương 13 General-Purpose Input/Output (GPIO)
 *     13.1.2 GPIO ports memory map ................... tr. 225
 *     13.3.1 GPIO register descriptions .............. tr. 228 - 236
 *   Chương 4 Signal Multiplexing: IO Signal Table 4.5.1, tr. 90
 *
 * Lưu ý: không include chung với S32K144.h của SDK trong cùng một file .c.
 */

#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include <stddef.h>

#define GPIO_STATIC_ASSERT(cond, msg)   _Static_assert(cond, msg)


/* ========================================================================
 *  PORT - ĐỊNH NGHĨA THANH GHI
 * ======================================================================== */

/* PORT_PCRn - 12.5.1 tr.213 */
typedef union {
    uint32_t REG;
    struct {
        uint32_t PS   : 1;    /* bit 0     : 0 = pull-down, 1 = pull-up    */
        uint32_t PE   : 1;    /* bit 1     : bật pull                      */
        uint32_t      : 2;    /* bit 2-3   */
        uint32_t PFE  : 1;    /* bit 4     : bật bộ lọc thụ động           */
        uint32_t      : 1;    /* bit 5     */
        uint32_t DSE  : 1;    /* bit 6     : tăng dòng ra                  */
        uint32_t      : 1;    /* bit 7     */
        uint32_t MUX  : 3;    /* bit 8-10  : 0 = tắt chân, 1 = GPIO        */
        uint32_t      : 4;    /* bit 11-14 */
        uint32_t LK   : 1;    /* bit 15    : khóa PCR tới lần reset sau    */
        uint32_t IRQC : 4;    /* bit 16-19 : cấu hình ngắt theo chân       */
        uint32_t      : 4;    /* bit 20-23 */
        uint32_t ISF  : 1;    /* bit 24    : cờ ngắt (w1c)                 */
        uint32_t      : 7;    /* bit 25-31 */
    } BITS;
} PORT_PCR_t;

/* Bố cục khối PORT - 12.5 tr.210 */
typedef struct {
    PORT_PCR_t PCR[32];       /* 0x00 - 0x7C */
    uint32_t   GPCLR;         /* 0x80 (WO)   */
    uint32_t   GPCHR;         /* 0x84 (WO)   */
    uint32_t   GICLR;         /* 0x88 (WO)   */
    uint32_t   GICHR;         /* 0x8C (WO)   */
    uint32_t   RESERVED0[4];  /* 0x90 - 0x9C */
    uint32_t   ISFR;          /* 0xA0 (w1c)  */
    uint32_t   RESERVED1[7];  /* 0xA4 - 0xBC */
    uint32_t   DFER;          /* 0xC0 */
    uint32_t   DFCR;          /* 0xC4 */
    uint32_t   DFWR;          /* 0xC8 */
} PORT_typedef;

GPIO_STATIC_ASSERT(sizeof(PORT_PCR_t) == 4u, "PORT_PCR_t phai 32 bit");
GPIO_STATIC_ASSERT(offsetof(PORT_typedef, GPCLR) == 0x80u, "PORT GPCLR sai offset");
GPIO_STATIC_ASSERT(offsetof(PORT_typedef, ISFR)  == 0xA0u, "PORT ISFR sai offset");
GPIO_STATIC_ASSERT(offsetof(PORT_typedef, DFER)  == 0xC0u, "PORT DFER sai offset");
GPIO_STATIC_ASSERT(offsetof(PORT_typedef, DFWR)  == 0xC8u, "PORT DFWR sai offset");

#define ADDRESS_PORTA           0x40049000UL
#define ADDRESS_PORTB           0x4004A000UL
#define ADDRESS_PORTC           0x4004B000UL
#define ADDRESS_PORTD           0x4004C000UL
#define ADDRESS_PORTE           0x4004D000UL

#define PORTA                   ((volatile PORT_typedef *)ADDRESS_PORTA)
#define PORTB                   ((volatile PORT_typedef *)ADDRESS_PORTB)
#define PORTC                   ((volatile PORT_typedef *)ADDRESS_PORTC)
#define PORTD                   ((volatile PORT_typedef *)ADDRESS_PORTD)
#define PORTE                   ((volatile PORT_typedef *)ADDRESS_PORTE)


/* ========================================================================
 *  GPIO - ĐỊNH NGHĨA THANH GHI
 *  Để uint32_t thường, không dùng bitfield: PSOR/PCOR/PTOR là thanh ghi
 *  chỉ ghi, mỗi bit tương ứng một chân, ghi mask là cách dùng đúng.
 * ======================================================================== */
typedef struct {
    uint32_t PDOR;    /* 0x00: Port Data Output                    */
    uint32_t PSOR;    /* 0x04: Port Set Output    (WO, ghi 1 = set)     */
    uint32_t PCOR;    /* 0x08: Port Clear Output  (WO, ghi 1 = clear)   */
    uint32_t PTOR;    /* 0x0C: Port Toggle Output (WO, ghi 1 = đảo)     */
    uint32_t PDIR;    /* 0x10: Port Data Input    (RO)             */
    uint32_t PDDR;    /* 0x14: Port Data Direction (1 = output)    */
    uint32_t PIDR;    /* 0x18: Port Input Disable                  */
} GPIO_typedef;

GPIO_STATIC_ASSERT(offsetof(GPIO_typedef, PDIR) == 0x10u, "GPIO PDIR sai offset");
GPIO_STATIC_ASSERT(offsetof(GPIO_typedef, PDDR) == 0x14u, "GPIO PDDR sai offset");
GPIO_STATIC_ASSERT(offsetof(GPIO_typedef, PIDR) == 0x18u, "GPIO PIDR sai offset");

#define ADDRESS_PTA             0x400FF000UL
#define ADDRESS_PTB             0x400FF040UL
#define ADDRESS_PTC             0x400FF080UL
#define ADDRESS_PTD             0x400FF0C0UL
#define ADDRESS_PTE             0x400FF100UL

#define PTA                     ((volatile GPIO_typedef *)ADDRESS_PTA)
#define PTB                     ((volatile GPIO_typedef *)ADDRESS_PTB)
#define PTC                     ((volatile GPIO_typedef *)ADDRESS_PTC)
#define PTD                     ((volatile GPIO_typedef *)ADDRESS_PTD)
#define PTE                     ((volatile GPIO_typedef *)ADDRESS_PTE)


/* ========================================================================
 *  KIỂU DỮ LIỆU CHO DRIVER
 * ======================================================================== */
/* Kiểu dữ liệu: số nguyên 8-bit, giá trị hợp lệ lấy từ các #define bên dưới */
typedef uint8_t GPIO_Port;
typedef uint8_t GPIO_Dir;
typedef uint8_t GPIO_Pull;
typedef uint8_t GPIO_Level;
typedef uint8_t GPIO_Status;

/* Port */
#define GPIO_PORT_A             (0u)
#define GPIO_PORT_B             (1u)
#define GPIO_PORT_C             (2u)
#define GPIO_PORT_D             (3u)
#define GPIO_PORT_E             (4u)
#define GPIO_PORT_COUNT         (5u)

/* Hướng */
#define GPIO_DIR_INPUT          (0u)
#define GPIO_DIR_OUTPUT         (1u)

/* Pull */
#define GPIO_PULL_NONE          (0u)
#define GPIO_PULL_DOWN          (1u)
#define GPIO_PULL_UP            (2u)

/* Mức logic */
#define GPIO_LOW                (0u)
#define GPIO_HIGH               (1u)

/* Mã trả về */
#define GPIO_OK                 (0u)
#define GPIO_ERR_PARAM          (1u)
#define GPIO_ERR_CLOCK          (2u)    /* không bật được clock PORT */
#define GPIO_ERR_LOCKED         (3u)    /* PCR đang bị khóa (LK = 1) */

typedef struct {
    GPIO_Port  port;          /* GPIO_PORT_A .. GPIO_PORT_E        */
    uint8_t    pin;           /* 0 - 31                            */
    GPIO_Dir   dir;           /* GPIO_DIR_INPUT / GPIO_DIR_OUTPUT  */
    GPIO_Pull  pull;          /* GPIO_PULL_NONE / DOWN / UP        */
    uint8_t    filter;        /* 1 = bật bộ lọc thụ động (PFE), cho input */
    GPIO_Level init_level;    /* mức ra ban đầu, cho output        */
} GPIO_Config;


/* ========================================================================
 *  API
 * ======================================================================== */
GPIO_Status GPIO_Init(const GPIO_Config *cfg);

void       GPIO_WritePin(GPIO_Port port, uint8_t pin, GPIO_Level level);
void       GPIO_SetPin(GPIO_Port port, uint8_t pin);
void       GPIO_ClearPin(GPIO_Port port, uint8_t pin);
void       GPIO_TogglePin(GPIO_Port port, uint8_t pin);
GPIO_Level GPIO_ReadPin(GPIO_Port port, uint8_t pin);

#endif /* GPIO_H */
