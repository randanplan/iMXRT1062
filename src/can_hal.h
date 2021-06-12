#ifndef __CAN_DRIVER_H__
#define __CAN_DRIVER_H__

#include "driver.h"

// typedef struct {
// 	const uint32_t BAUD;
// 	const uint32_t CLOCK;
// 	volatile uint32_t *MCR;
// 	volatile uint32_t *CTRL1;
// 	volatile uint32_t *TIMER;
// 	volatile uint32_t *RXMGMASK;
// 	volatile uint32_t *RX14MASK;
// 	volatile uint32_t *RX15MASK;
// 	volatile uint32_t *ECR;
// 	volatile uint32_t *ESR1;
// 	volatile uint32_t *IMASK2;
// 	volatile uint32_t *IMASK1;
// 	volatile uint32_t *IFLAG2;
// 	volatile uint32_t *IFLAG1;
// 	volatile uint32_t *CTRL2;
// 	volatile uint32_t *ESR2;
// 	volatile uint32_t *CRCR;
// 	volatile uint32_t *RXFGMASK;
// 	volatile uint32_t *RXFIR;
// 	// volatile uint32_t RXIMR;//(uint8_t mb);
// 	// volatile uint32_t GFWR;//(uint8_t mb);

// 	// volatile uint32_t MBn_CS;//(uint8_t mb);
// 	// volatile uint32_t MBn_ID;//(uint8_t mb);
// 	// volatile uint32_t FLEXCANb_MBn_WORD0;//(uint8_t mb);
// 	// volatile uint32_t FLEXCANb_MBn_WORD1;//(uint8_t mb);
//     // volatile uint32_t FLEXCANb_IDFLT_TAB;//(uint8_t mb);
// 	// volatile uint32_t MAXMB_SIZE;//(uint8_t mb);
// } IMXRT_FLEXCAN_t;

// const IMXRT_FLEXCAN_t IMXRT_CAN2 = {
//     .BAUD=500000,
//     .CLOCK=60*1000*1000
//     .MCS=0x401D4000,
// };

// void can_init (void);
// uint8_t *I2C_Receive (uint32_t i2cAddr, uint8_t *buf, uint16_t bytes, bool block);
// void can_Send ();
// uint8_t *I2C_ReadRegister (uint32_t i2cAddr, uint8_t *buf, uint8_t abytes, uint16_t bytes, bool block);

#endif