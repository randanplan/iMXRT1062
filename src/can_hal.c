#include "driver.h"

#if defined(CAN_ENABLE) && ODRIVE_SPINDLE == 5

#include <string.h>

#include "can_hal.h"
// #include "spindle/CAN_T4.h"


// #define PINCONFIG (IOMUXC_PAD_ODE | IOMUXC_PAD_SRE | IOMUXC_PAD_DSE(4) | IOMUXC_PAD_SPEED(1) | IOMUXC_PAD_PKE | IOMUXC_PAD_PUE | IOMUXC_PAD_PUS(3))

typedef struct {
    volatile uint32_t *clock_gate_register;
    uint32_t clock_gate_mask;
    uint32_t clockSpeed;
    volatile IMXRT_REGISTER32_t *port;
    enum IRQ_NUMBER_t irq;
    uint32_t baudrate;
} can_hardware_t;
// CCM_CCGR0 |= CCM_CCGR0_CAN2(CCM_CCGR_ON) | CCM_CCGR0_CAN2_SERIAL(CCM_CCGR_ON);
static const can_hardware_t can2_hardware = {
    .clock_gate_register = &CCM_CCGR0,
    .clock_gate_mask = CCM_CCGR0_CAN2(CCM_CCGR_ON) | CCM_CCGR0_CAN2_SERIAL(CCM_CCGR_ON),
    .clockSpeed = 60 * 1000 *1000,
    .port = &IMXRT_FLEXCAN2,
    .irq = IRQ_CAN2,
    .baudrate = 500000
};

static const can_hardware_t *hardware;
static IMXRT_REGISTER32_t *port = NULL;
void can_interrupt(void);

void can_init_hal (void)
{
    static bool init_ok = false;

    if(!init_ok) {

        init_ok = true;

        hardware = &can2_hardware;

        port = &hardware->port;

        *hardware->clock_gate_register |= hardware->clock_gate_mask;
        // port. = FLEXCAN_MCR_MDIS;
        // CCM_CSCMR2 = (CCM_CSCMR2 & 0xFFFFFC03) | CCM_CSCMR2_CAN_CLK_SEL(1) | CCM_CSCMR2_CAN_CLK_PODF(0); // CLK_24MHz
        // CCM_CSCMR2 = (CCM_CSCMR2 & 0xFFFFFC03) | CCM_CSCMR2_CAN_CLK_SEL(0) | CCM_CSCMR2_CAN_CLK_PODF(0); // CLK_60MHz

        // Setup SDA register
        IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_02 = 0x10; // pin 1 T4B1+B2
        IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_02 = 0x10B0; // pin 1 T4B1+B2
        IOMUXC_FLEXCAN2_RX_SELECT_INPUT = 0x01;
        IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_03 = 0x10; // pin 0 T4B1+B2
        IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_03 = 0x10B0; // pin 0 T4B1+B2

        attachInterruptVector(hardware->irq, can_interrupt);

        NVIC_SET_PRIORITY(hardware->irq, 8);
        NVIC_ENABLE_IRQ(hardware->irq);
    }
}

void can_interrupt(void) {
  // CAN_message_t msg; // setup a temporary storage buffer
//   uint64_t imask = readIMASK(), iflag = readIFLAG();

  if ( !(port->offset074 & (1UL << 15))) { /* if DMA is disabled, ONLY THEN you can handle FIFO in ISR */
    
  }

  asm volatile ("dsb");	
}

#endif