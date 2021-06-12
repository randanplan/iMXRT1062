#include "driver.h"

#if OLED_ENABLE

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "grbl/report.h"
#include "grbl/nvs_buffer.h"
#include "csrc/u8g2.h"
#include "csrc/u8x8.h"

static u8g2_t u8g2; // a structure which will contain all the data for one display
static u8x8_t u8x8(u8x8_d_ssd1327_ws_128x128,24,25);
void oled_init(){
    u8g2_Setup_ssd1327_i2c_ws_128x128_f(&u8g2, U8G2_R0, u8x8.byte_cb, u8x8.gpio_and_delay_cb);  // init u8g2 structure
    // u8g2_Setup_ssd1327_i2c_ws_128x128_f(&u8g2, U8G2_R0, u8x8_byte_sw_i2c, delayNanoseconds);
    u8g2_InitDisplay(&u8g2); // send init sequence to the display, display is in sleep mode after this,
    u8g2_SetPowerSave(&u8g2, 1); // wake up display
    u8g2_DrawBox(&u8g2,3,7,25,15);
    u8g2_SetContrast(&u8g2, 50);
    u8g2_UpdateDisplay(&u8g2);
}


#endif