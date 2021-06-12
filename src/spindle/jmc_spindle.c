
#include "jmc_spindle.h"

#if SPINDLE_JMC

#include <math.h>
#include <string.h>

#ifdef ARDUINO
#include "../grbl/hal.h"
#include "../grbl/state_machine.h"
#include "../grbl/report.h"
#else
#include "grbl/hal.h"
#include "grbl/state_machine.h"
#include "grbl/report.h"
#endif

#ifdef SPINDLE_PWM_DIRECT
// #error "Uncomment SPINDLE_RPM_CONTROLLED in grbl/config.h to add JMC spindle support!"
#endif

#ifndef JMC_ADDRESS
#define JMC_ADDRESS 0x01
#endif

typedef enum {
    JMC_Idle = 0,
    JMC_GetRPM,
    JMC_SetRPM,
    JMC_GetStatus,
    JMC_GetLoad,
    JMC_GetVbus,
} jmc_response_t;

typedef enum {
  regs_startup = 0x0866,
  regs_status = 0x07e6,
  regs_rpm_cmd = 0x0841,
  regs_rpm_real = 0x0842,
  regs_rpm_set = 0x0192,
  regs_load = 0x0844,
  regs_vbus = 0x0850
}regs_jmc;

static float rpm_programmed = -1.0f, rpm_low_limit = 0.0f, rpm_high_limit = 0.0f;
static spindle_state_t jmc_state = {0};
static spindle_data_t spindle_data = {0};
static settings_changed_ptr settings_changed;
static on_report_options_ptr on_report_options;

static void spindleSetRPM (float rpm, bool block)
{
    modbus_message_t rpm_cmd;

    if (rpm != rpm_programmed) {

        rpm_cmd.xx = (void *)JMC_SetRPM;
        rpm_cmd.adu[0] = JMC_ADDRESS;

        uint16_t data = jmc_state.ccw ? -1*(uint32_t)(rpm) : (uint32_t)(rpm);

        rpm_cmd.adu[1] = ModBus_WriteRegister;
        rpm_cmd.adu[2] = regs_rpm_set >> 8;
        rpm_cmd.adu[3] = regs_rpm_set & 0XFF;
        rpm_cmd.adu[4] = data >> 8;
        rpm_cmd.adu[5] = data & 0xFF;
        rpm_cmd.tx_length = 8;
        rpm_cmd.rx_length = 8;

        jmc_state.at_speed = false;
        hal.stream.write("[MSG:JMC SPINDLE Set Speed()]" ASCII_EOL);
        modbus_send(&rpm_cmd, block);

        if(settings.spindle.at_speed_tolerance > 0.0f) {
            rpm_low_limit = rpm / (1.0f + settings.spindle.at_speed_tolerance);
            rpm_high_limit = rpm * (1.0f + settings.spindle.at_speed_tolerance);
        }
        rpm_programmed = rpm;
    }
}

static void spindleUpdateRPM (float rpm)
{
    spindleSetRPM(rpm, false);
}

// Start or stop spindle
static void spindleSetState (spindle_state_t state, float rpm)
{
    modbus_message_t mode_cmd;
    mode_cmd.xx = (void *)JMC_GetStatus;
    mode_cmd.adu[0] = JMC_ADDRESS;
#if SPINDLE_HUANYANG == 2
    mode_cmd.adu[1] = ModBus_WriteRegister;
    mode_cmd.adu[2] = 0x20;
    mode_cmd.adu[3] = 0x00;
    mode_cmd.adu[4] = 0x00;
    mode_cmd.adu[5] = (!state.on || rpm == 0.0f) ? 6 : (state.ccw ? 2 : 1);
    mode_cmd.tx_length = 8;
    mode_cmd.rx_length = 8;
#else
    mode_cmd.adu[1] = ModBus_ReadHoldingRegisters;
    mode_cmd.adu[2] = regs_status >> 8;
    mode_cmd.adu[3] = regs_status & 0xFF;
    mode_cmd.adu[4] = 0x00;
    mode_cmd.adu[5] = 0x02;
    mode_cmd.tx_length = 8;
    mode_cmd.rx_length = 8;
#endif

    if(jmc_state.ccw != state.ccw)
        rpm_programmed = 0.0f;

    jmc_state.on = state.on;
    jmc_state.ccw = state.ccw;

    // if (!state.on)
    //     // DIGITAL_OUT(SPINDLE_ENABLE_PIN, settings.spindle.invert.on);
    //     spindle_off();
    // else {
    //     if(hal.driver_cap.spindle_dir)
    //         // spindle_dir(state.ccw);
    //     spindle_on();
    //     // DIGITAL_OUT(SPINDLE_ENABLE_PIN, !settings.spindle.invert.on);
    //     // #ifdef SPINDLE_SYNC_ENABLE
    //     // spindleDataReset();
    //     // #endif
    // }
    if(modbus_send(&mode_cmd, true))
    spindleSetRPM(rpm, true);
}

// Returns spindle state in a spindle_state_t variable
static spindle_state_t spindleGetState (void)
{
    modbus_message_t mode_cmd;

    mode_cmd.xx = (void *)JMC_GetStatus;
    mode_cmd.adu[0] = JMC_ADDRESS;

#if SPINDLE_HUANYANG == 2

    // Get current RPM

    mode_cmd.adu[1] = ModBus_ReadHoldingRegisters;
    mode_cmd.adu[2] = 0x70;
    mode_cmd.adu[3] = 0x0C;
    mode_cmd.adu[4] = 0x00;
    mode_cmd.adu[5] = 0x02;
    mode_cmd.tx_length = 8;
    mode_cmd.rx_length = 8;

#else

    mode_cmd.adu[1] = ModBus_ReadHoldingRegisters;
    mode_cmd.adu[2] = regs_status >> 8;
    mode_cmd.adu[3] = regs_status & 0xFF;
    mode_cmd.adu[4] = 0x00;
    mode_cmd.adu[5] = 0x02;
    mode_cmd.tx_length = 8;
    mode_cmd.rx_length = 8;

#endif
    // hal.stream.write("[MSG:JMC SPINDLE get State]" ASCII_EOL);
    modbus_send(&mode_cmd, false);

    return jmc_state; // return previous state as we do not want to wait for the response
}

static spindle_data_t *spindleGetData (spindle_data_request_t request)
{
    return &spindle_data;
}


static void rx_packet (modbus_message_t *msg)
{
    if(!(msg->adu[0] & 0x80)) {

        switch((jmc_response_t)msg->xx) {

            case JMC_GetRPM:
                spindle_data.rpm = (float)((msg->adu[4] << 8) | msg->adu[5]);
                jmc_state.at_speed = settings.spindle.at_speed_tolerance <= 0.0f || (spindle_data.rpm >= rpm_low_limit && spindle_data.rpm <= rpm_high_limit);
                break;
            case JMC_GetLoad:
                // rpm_max = (msg->adu[4] << 8) | msg->adu[5];
                break;
            default:
                break;
        }
    }
}

static void rx_exception (uint8_t code)
{       
    hal.stream.write("[MSG:JMC SPINDLE RX Exception Alarm]" ASCII_EOL);
    // system_raise_alarm(Alarm_Spindle);
}

static void onReportOptions (bool newopt)
{
    on_report_options(newopt);

    if(!newopt) {
        hal.stream.write("[PLUGIN:JMC SPINDLE v0.01]" ASCII_EOL);
    }
}

static void jmc_spindle_settings_changed (settings_t *settings)
{
    static bool init_ok = false;
    static driver_cap_t driver_cap;
    static spindle_ptrs_t spindle_org;

    if(init_ok && settings_changed)
        settings_changed(settings);

    if(!init_ok && hal.driver_cap.dual_spindle) {
        driver_cap = hal.driver_cap;
        memcpy(&spindle_org, &hal.spindle, sizeof(spindle_ptrs_t));
    }

    if(hal.driver_cap.dual_spindle && settings->mode == Mode_Laser) {

        if(hal.spindle.set_state == spindleSetState) {
            hal.driver_cap = driver_cap;
            memcpy(&spindle_org, &hal.spindle, sizeof(spindle_ptrs_t));
        }

    } else {

        if(settings->spindle.ppr == 0)
            hal.spindle.get_data = spindleGetData;

        if(hal.spindle.set_state != spindleSetState) {

            hal.spindle.set_state = spindleSetState;
            hal.spindle.get_state = spindleGetState;
            hal.spindle.update_rpm = spindleUpdateRPM;
            hal.spindle.reset_data = NULL;

            hal.driver_cap.variable_spindle = On;
            hal.driver_cap.spindle_at_speed = On;
            hal.driver_cap.spindle_dir = On;
        }

#if SPINDLE_HUANYANG == 2
        if(!init_ok) {

            modbus_message_t cmd;

            cmd.xx = JMC_GetMaxRPM;
            cmd.adu[0] = JMC_ADDRESS;
            cmd.adu[1] = ModBus_ReadHoldingRegisters;
            cmd.adu[2] = 0xB0;
            cmd.adu[3] = 0x05;
            cmd.adu[4] = 0x02;
            cmd.adu[5] = 0x00;
            cmd.tx_length = 8;
            cmd.rx_length = 8;

            modbus_send(&cmd, true);
        }
#endif

#if SPINDLE_JMC == 1
        if(!init_ok) {
            hal.stream.write("[MSG:JMC SPINDLE Settings changed()]" ASCII_EOL);
            modbus_message_t cmd;

            cmd.xx = (void *)JMC_GetVbus;
            cmd.adu[0] = JMC_ADDRESS;
            cmd.adu[1] = ModBus_ReadHoldingRegisters;
            cmd.adu[2] = regs_vbus >> 8;
            cmd.adu[3] = regs_vbus & 0xFF;
            cmd.adu[4] = 0x00;
            cmd.adu[5] = 0x02;
            cmd.tx_length = 8;
            cmd.rx_length = 8;

            modbus_send(&cmd, false);
        }
#endif
    }
    hal.stream.write("[MSG:JMC SPINDLE Init_OK]" ASCII_EOL);
    init_ok = true;
}

void jmc_spindle_init (modbus_stream_t *stream)
{
    // hal.stream.write("[MSG:JMC SPINDLE Init()]" ASCII_EOL);
    settings_changed = hal.settings_changed;
    hal.settings_changed = jmc_spindle_settings_changed;

    on_report_options = grbl.on_report_options;
    grbl.on_report_options = onReportOptions;

    stream->on_rx_packet = rx_packet;
    stream->on_rx_exception = rx_exception;

    if(!hal.driver_cap.dual_spindle)
        jmc_spindle_settings_changed(&settings);
}
#endif
