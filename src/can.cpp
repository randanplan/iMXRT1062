// #include <string.h>
// #include "Arduino.h"
#include "driver.h"
#include "grbl/hal.h"
#include "grbl/report.h"

#if ODRIVE_SPINDLE == -1
#include "can.h"
#include "FlexCAN_T4.h"

FlexCAN_T4<CAN2, RX_SIZE_128, TX_SIZE_32> Can2;

#define nodeID(id) ((id >> 5) & 0x03F)
#define cmdID(id) (id & 0x01F)
#define setID(id,cmd) ((id << 5) + cmd)

#define CAN_MAX_TX_PERIODIC_FRAMES 10
#define CAN_MAX_RX_FRAMES 10

static can_rx_ptr_t rxPtrs[CAN_MAX_RX_FRAMES];
static can_txPeriodframe_t txFrames[CAN_MAX_TX_PERIODIC_FRAMES];
static int rx_ptr_count=0,tx_frame_count=0;
bool CanInitOk;
// static on_frame_RX_ptr *on_frame_RX;

void output_frame(const CAN_message_t &inMsg)
{
    if (on_frame_RX){
        CAN_frame_t outMsg;
        // // memset(&outMsg,0,sizeof(CAN_message_t));
        outMsg.id = inMsg.id;
        outMsg.len = inMsg.len;
        outMsg.flags.remote = inMsg.flags.remote;
        memcpy(&outMsg.buf,&inMsg.buf,inMsg.len);
        // // for (uint8_t i = 0;i<8;i++){
        // //     outMsg.buf[i] = inMsg.buf[i];
        // // }
        on_frame_RX(&outMsg);
        // char buf[70];
        // sprintf(buf,"Buffer: %02x %02x %02x %02x %02x %02x %02x %02x",
        //     inMsg.buf[0],inMsg.buf[1],inMsg.buf[2],inMsg.buf[3],inMsg.buf[4],inMsg.buf[5],inMsg.buf[6],inMsg.buf[7]);
        // report_message(buf,Message_Plain);
    }
    else{
        char buf[70];
        sprintf(buf,"ext_out Buffer: %02x %02x %02x %02x %02x %02x %02x %02x",
            inMsg.buf[0],inMsg.buf[1],inMsg.buf[2],inMsg.buf[3],inMsg.buf[4],inMsg.buf[5],inMsg.buf[6],inMsg.buf[7]);
        report_message(buf,Message_Plain);
    }
    // for (uint8_t i = 0;i<rx_ptr_count;i++){
    //     if (rxPtrs[i].onFrameRX){
    //         if (rxPtrs[i].id == inMsg.id){
    //             CAN_message_t inFrame;
    //             memcpy(&inFrame.buf,&inMsg.buf,sizeof(inMsg.buf));
    //             inFrame.id = inMsg.id;
    //             inFrame.flags.remote = inMsg.flags.remote;
    //             rxPtrs[i].onFrameRX(&inFrame);
    //         }
    //     }
    // }
    // return;
//   char buf[70];
//   sprintf(buf,"Buffer: %02x %02x %02x %02x %02x %02x %02x %02x",
//             inMsg.buf[0],inMsg.buf[1],inMsg.buf[2],inMsg.buf[3],inMsg.buf[4],inMsg.buf[5],inMsg.buf[6],inMsg.buf[7]);
//   report_message(buf,Message_Plain);
//   return;
//   static bool lastLED;
//   CAN_message_t *msg = &inMsg;
//   Serial.printf("[MB %2d ID %2d CMD %2d LEN: %1d EXT: %1d TS: %5d ID: 0x%02x\t",
//             (int)msg->mb,(int)nodeID(msg->id),(int)cmdID(msg->id)
//             ,(int)msg->len,(int)msg->flags.extended,(int)msg->timestamp,(int)msg->id);
//   Serial.print("Buffer: ");
//   for ( uint8_t i = 0; i < inMsg.len; i++ ) {
//     Serial.print(inMsg.buf[i], HEX);
//     if (inMsg.buf[i] == 0)Serial.print("0");
//     Serial.print(" ");
//   }
//   Serial.print("]\r\n");
//   digitalWrite(LED_BUILTIN,lastLED=!lastLED);
}

void canSniff(const CAN_message_t &msg) {
  Serial.print("MB "); Serial.print(msg.mb);
  Serial.print("  OVERRUN: "); Serial.print(msg.flags.overrun);
  Serial.print("  LEN: "); Serial.print(msg.len);
  Serial.print(" EXT: "); Serial.print(msg.flags.extended);
  Serial.print(" TS: "); Serial.print(msg.timestamp);
  Serial.print(" ID: "); Serial.print(msg.id, HEX);
  Serial.print(" Buffer: ");
  for ( uint8_t i = 0; i < msg.len; i++ ) {
    Serial.print(msg.buf[i], HEX); Serial.print(" ");
  } Serial.println();
}

void can_Init()
{   
    rx_ptr_count = 0;
    tx_frame_count = 0;
    // on_frame_RX = 0;
    // txFrames[0].id = 23;
    // txFrames[0].next = 0;
    // txFrames[0].wait = 1000;
    // txFrames[0].rtr = true;
    // txFrames[0].isPeriodic = true;
    // tx_frame_count+=1;
    // txFrames[1].id = 10;
    // txFrames[1].next = 0;
    // txFrames[1].wait = 150;
    // txFrames[1].rtr = true;
    // txFrames[1].isPeriodic = true;
    // tx_frame_count+=1;
    pinMode(LED_BUILTIN,OUTPUT);
    Can2.begin();
    Can2.setBaudRate(500000);
    Can2.setMaxMB(16);
    Can2.enableFIFO(1);
    Can2.enableFIFOInterrupt(1);
    Can2.setFIFOFilter(ACCEPT_ALL);
    // Can2.distribute(1);
    // Can2.setMB(MB2,RX,STD);
    // Can2.setMBFilter(MB2,0x10,0x09,0x01,0x23);
    Can2.setRRS(0);
    // Can2.enableMBInterrupts();
    // Can2.enhanceFilter(MB2);
    Can2.onReceive(canSniff);
    CanInitOk = true;
}

void can_add_rx_ptr(can_rx_ptr_t rx_ptr){
    if (rx_ptr_count < CAN_MAX_RX_FRAMES){
        // memcpy(&txFrames[rx_ptr_count],&rx_ptr,sizeof(rx_ptr));
        rxPtrs[rx_ptr_count] = rx_ptr;
        rx_ptr_count++;
    }
}

void can_add_tx_ptr(can_txPeriodframe_t tx_frame){
    if (tx_frame_count < CAN_MAX_RX_FRAMES){
        uint8_t i = tx_frame_count;
        txFrames[i].id = tx_frame.id;
        txFrames[i].isPeriodic = tx_frame.isPeriodic;
        txFrames[i].next = tx_frame.next;
        txFrames[i].rtr = tx_frame.rtr;
        txFrames[i].wait = tx_frame.wait;
        tx_frame_count++;
    }
}

void can_write(CAN_message_t *frame)
{   
    if (!CanInitOk) return;
    CAN_message_t outMsg;
    memcpy(&outMsg.buf,frame->buf,frame->len);
    outMsg.id = frame->id;
    outMsg.flags.remote = frame->flags.remote;
    outMsg.len = frame->len;
    Can2.write(outMsg);
}

void can_poll()
{
    Can2.events();
    if (!CanInitOk) return;
    static uint32_t last_ms,delay_ms = 2000;
    uint32_t ms = systick_millis_count;
    if(ms == last_ms) // check once every ms
        return;


    if(delay_ms && !(--delay_ms)) {
        delay_ms = 2000;
        Can2.mailboxStatus();
    //     // char buf[30];
    //     // sprintf(buf,"CAN poll tx count:%d t:%d",tx_frame_count,ms);
    //     // report_message(buf,Message_Plain);
    }
    uint8_t cycle_limit = 1;
    for (int i = 0;i<tx_frame_count;i++){
        // can_txPeriodframe_t t = txFrames[0];
        if ((ms >= txFrames[i].next) && txFrames[i].isPeriodic){
            txFrames[i].next = ms + txFrames[i].wait;
            CAN_message_t outMsg;
            outMsg.id=txFrames[i].id;
            outMsg.flags.remote=1;
            outMsg.flags.extended=0;
            outMsg.len=0;
            // bool out_OK = 0;
            // Can2.struct2queueTx(outMsg);
            Can2.write(outMsg);
            if ( !--cycle_limit ) i = CAN_MAX_TX_PERIODIC_FRAMES;
            // char buf[40];
            // sprintf(buf,"CAN write ok:%d id:%d t:%d w:%d",out_OK,outMsg.id,txFrames[i].next,txFrames[i].wait);
            // report_message(buf,Message_Plain);
        } 
    }
    last_ms = ms;
}

#endif
