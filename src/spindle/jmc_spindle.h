#ifndef _JMC_SPINDLE_H_
#define _JMC_SPINDLE_H_

#ifdef ARDUINO
#include "../driver.h"
#else
#include "driver.h"
#endif

#if SPINDLE_JMC

// #ifdef JMC_SPINDLE
// #undef JMC_SPINDLE
// #endif
// #define JMC_SPINDLE 1

#include "modbus.h"
// #undef MODBUS_MAX_ADU_SIZE
// #define MODBUS_MAX_ADU_SIZE 16
void jmc_spindle_init (modbus_stream_t *stream);

#endif // _JMC_SPINDLE_H_

#endif
