#ifndef _CAN_H_
#define _CAN_H_

#include <stdbool.h>
#include <stdint.h>

#if ODRIVE_SPINDLE == -1

typedef struct CAN_frame_t{
  uint32_t id;          // can identifier
  uint16_t timestamp;   // FlexCAN time when message arrived
  uint8_t idhit; // filter that id came from
  struct {
    bool extended; // identifier is extended (29-bit)
    bool remote;  // remote transmission request packet type
    bool overrun; // message overrun
    bool reserved;
  } flags;
  uint8_t len;      // length of data
  uint8_t buf[8];       // data
  int8_t mb;       // used to identify mailbox reception
  uint8_t bus;      // used to identify where the message came from when events() is used.
  bool seq;         // sequential frames
} CAN_frame_t;

typedef void (*on_frame_RX_ptr)(CAN_frame_t *rxFrame);

typedef struct can_txPeriodframe_t
{
    // CAN_frame_t frame;
    uint8_t id;
    bool rtr;
    bool isPeriodic;
    volatile uint32_t next;
    uint16_t wait;
}can_txPeriodframe_t;

typedef struct can_rx_ptr_t
{
    uint8_t id;
    bool rtr;
    on_frame_RX_ptr onFrameRX;
}can_rx_ptr_t;

// typedef volatile uint8_t vuint8_t;
// typedef volatile uint16_t vuint16_t;
// typedef volatile uint32_t vuint32_t;
// typedef uint32_t canmbx_t;
// typedef uint32_t canmsg_t;
// typedef struct CAN_TxMailBox_TypeDef{
//   union {
//     vuint32_t R;
//     struct {
//       vuint8_t:4;
//       vuint8_t CODE:4;
//       vuint8_t:1;
//       vuint8_t SRR:1;
//       vuint8_t IDE:1;
//       vuint8_t RTR:1;
//       vuint8_t LENGTH:4;
//       vuint16_t TIMESTAMP:16;
//     } B;
//   } CS;
//   union {
//     vuint32_t R;
//     struct {
//       vuint8_t PRIO:3;
//       vuint32_t ID:29;
//     } B;
//   } ID;
//   vuint32_t DATA[2];     /* Data buffer in words (32 bits) */
// } CAN_TxMailBox_TypeDef;
// typedef struct CANTxFrame{ 
//   uint8_t                 LENGTH:4;       /**< @brief Data length.        */
//   uint8_t                 RTR:1;          /**< @brief Frame type.         */
//   uint8_t                 IDE:1;          /**< @brief Identifier type.    */
//   union { 
//     uint32_t              SID:11;         /**< @brief Standard identifier.*/ 
//     uint32_t              EID:29;         /**< @brief Extended identifier.*/  
//   };
//   union {
//     uint8_t                 data8[8];       /**< @brief Frame data.         */
//     uint16_t                data16[4];      /**< @brief Frame data.         */
//     uint32_t                data32[2];      /**< @brief Frame data.         */
//   };
// } CANTxFrame;
// typedef struct CANRxFrame{
//   uint16_t                TIME;           /**< @brief Time stamp.         */
//   uint8_t                 LENGTH:4;       /**< @brief Data length.        */
//   uint8_t                 RTR:1;          /**< @brief Frame type.         */
//   uint8_t                 IDE:1;          /**< @brief Identifier type.    */
//   union { 
//     uint32_t              SID:11;         /**< @brief Standard identifier.*/
//     uint32_t              EID:29;         /**< @brief Extended identifier.*/  
//   };
//   union {
//     uint8_t                 data8[8];       /**< @brief Frame data.         */
//     uint16_t                data16[4];      /**< @brief Frame data.         */
//     uint32_t                data32[2];      /**< @brief Frame data.         */
//   };
// } CANRxFrame;

#ifdef __cplusplus
extern "C" {
#endif
// #define can_poll() can_execute_realtime(0)
#define CAN_ENABLE
#define CAN_ID_ALL_IDS 255

 on_frame_RX_ptr on_frame_RX;
// void (*on_frame_RX)(CAN_frame_t *rxFrame);
void can_poll(void);
void can_Init(void);
void can_write(CAN_frame_t *frame);
void can_add_rx_ptr(can_rx_ptr_t rx_ptr);
void can_add_tx_ptr(can_txPeriodframe_t tx_frame);
// void ext_output1(const CAN_message_t &msg);
#ifdef __cplusplus
}
#endif

#endif

#endif
