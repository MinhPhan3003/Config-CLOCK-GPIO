/**
 * @file    gpio.c
 * @brief   Hiện thực driver GPIO cho S32K144.
 *
 * Trình tự khởi tạo một chân (RM Rev.13):
 *   1. Bật clock PORTx qua PCC ........... 29.6.22 - 29.6.26, tr. 656 - 662
 *   2. Ghi PORTx_PCRn (MUX = GPIO, pull) .. 12.5.1, tr. 213
 *   3. Đặt mức ra ban đầu (PSOR/PCOR) ..... 13.3.1, tr. 228 - 236
 *   4. Đặt hướng (PDDR) ................... 13.3.1, tr. 228 - 236
 *   Tham khảo thêm: 12.1.3 I/O configuration sequence, tr. 206
 */

#include "gpio.h"
#include "clock.h"

/* Bảng tra theo GPIO_Port */
static volatile PORT_typedef * const s_port_regs[GPIO_PORT_COUNT] = {
    PORTA, PORTB, PORTC, PORTD, PORTE
};

static volatile GPIO_typedef * const s_gpio_regs[GPIO_PORT_COUNT] = {
    PTA, PTB, PTC, PTD, PTE
};

static const uint32_t s_pcc_index[GPIO_PORT_COUNT] = {
    PCC_PORTA_INDEX, PCC_PORTB_INDEX, PCC_PORTC_INDEX, PCC_PORTD_INDEX, PCC_PORTE_INDEX
};

#define GPIO_MUX_GPIO   1u      /* ALT1 = GPIO (IO Signal Table 4.5.1, tr. 90) */


GPIO_Status GPIO_Init(const GPIO_Config *cfg)
{
    volatile PORT_typedef *port;
    volatile GPIO_typedef *gpio;
    PORT_PCR_t pcr = { .REG = 0u };
    uint32_t   mask;

    if ((cfg == NULL) || (cfg->port >= GPIO_PORT_COUNT) || (cfg->pin > 31u)) {
        return GPIO_ERR_PARAM;
    }

    port = s_port_regs[cfg->port];
    gpio = s_gpio_regs[cfg->port];
    mask = (1u << cfg->pin);

    /* 1. Bật clock cho PORT. Truy cập PORT khi chưa có clock sẽ gây lỗi bus. */
    if (CLOCK_EnablePeripheral(s_pcc_index[cfg->port]) != CLOCK_OK) {
        return GPIO_ERR_CLOCK;
    }

    if (port->PCR[cfg->pin].BITS.LK != 0u) {
        return GPIO_ERR_LOCKED;
    }

    /* 2. Cấu hình PCR trong biến cục bộ rồi ghi một lần.
     *    Ghi cả thanh ghi với ISF = 0 nên không xóa nhầm cờ ngắt. */
    pcr.BITS.MUX = GPIO_MUX_GPIO;
    switch (cfg->pull) {
    case GPIO_PULL_UP:
        pcr.BITS.PE = 1u;
        pcr.BITS.PS = 1u;
        break;
    case GPIO_PULL_DOWN:
        pcr.BITS.PE = 1u;
        pcr.BITS.PS = 0u;
        break;
    default:
        break;
    }
    if ((cfg->dir == GPIO_DIR_INPUT) && (cfg->filter != 0u)) {
        pcr.BITS.PFE = 1u;
    }
    port->PCR[cfg->pin].REG = pcr.REG;

    if (cfg->dir == GPIO_DIR_OUTPUT) {
        /* 3. Đặt mức ra trước khi đổi hướng, tránh chân chớp mức ngoài ý muốn */
        if (cfg->init_level == GPIO_HIGH) {
            gpio->PSOR = mask;
        } else {
            gpio->PCOR = mask;
        }
        /* 4. Hướng ra */
        gpio->PDDR |= mask;
    } else {
        /* 4. Hướng vào, đảm bảo input không bị tắt */
        gpio->PDDR &= ~mask;
        gpio->PIDR &= ~mask;
    }

    return GPIO_OK;
}

void GPIO_WritePin(GPIO_Port port, uint8_t pin, GPIO_Level level)
{
    if (level == GPIO_HIGH) {
        GPIO_SetPin(port, pin);
    } else {
        GPIO_ClearPin(port, pin);
    }
}

/* PSOR/PCOR/PTOR chỉ tác động lên bit được ghi 1, không cần đọc-sửa-ghi */
void GPIO_SetPin(GPIO_Port port, uint8_t pin)
{
    if ((port < GPIO_PORT_COUNT) && (pin <= 31u)) {
        s_gpio_regs[port]->PSOR = (1u << pin);
    }
}

void GPIO_ClearPin(GPIO_Port port, uint8_t pin)
{
    if ((port < GPIO_PORT_COUNT) && (pin <= 31u)) {
        s_gpio_regs[port]->PCOR = (1u << pin);
    }
}

void GPIO_TogglePin(GPIO_Port port, uint8_t pin)
{
    if ((port < GPIO_PORT_COUNT) && (pin <= 31u)) {
        s_gpio_regs[port]->PTOR = (1u << pin);
    }
}

GPIO_Level GPIO_ReadPin(GPIO_Port port, uint8_t pin)
{
    if ((port >= GPIO_PORT_COUNT) || (pin > 31u)) {
        return GPIO_LOW;
    }
    return ((s_gpio_regs[port]->PDIR & (1u << pin)) != 0u) ? GPIO_HIGH : GPIO_LOW;
}
