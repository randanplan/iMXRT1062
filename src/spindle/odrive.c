// #include "driver.h"
#include "odrive.h"

#if SPINDLE_ODRIVE

#include <stdio.h>
#include "can_t4.h"
// #include "../oled/oled.h"
#include "../grbl/hal.h"
#include "../grbl/override.h"
#include "../grbl/protocol.h"
#include "../grbl/report.h"
#include "../grbl/nvs_buffer.h"
#include "../grbl/state_machine.h"
#include "../grbl/settings.h"
#include "grbl/spindle_sync.h"


//------------------------------------------Odrive Settings----------------------------------

typedef enum odrive_settings_type_t{  // Odrive Settings type
    Settings_Odrive_useRatio = Setting_SettingsMax,
    Settings_Odrive_invert_RPM,
    Settings_Odrive_can_node_id,
    Settings_Odrive_can_baudrate,
    Settings_Odrive_can_timeout,
    Settings_Odrive_encoder_cpr,
    Settings_Odrive_motor_current_limit,
    Settings_Odrive_controller_vel_limit,
    Settings_Odrive_controller_vel_gain,
    Settings_Odrive_controller_vel_integral_gain,
    Settings_Odrive_gear_motor,
    Settings_Odrive_gear_spindle,
	Settings_Odrive_ovr_feed,
	Settings_Odrive_ovr_curit,
    // Settings_Odrive_cooling_after_time,
    // Settings_Odrive_cooling_temp_0,
    // Settings_Odrive_cooling_temp_100,
    Settings_Odrive_Settings_Max
}odrive_settings_type_t;

typedef struct odrive_settings_t{ // Odrive Settings
    bool use_ratio;          // Use Gear Ratio
    bool invert_rpm;         // Motor direction invert
    uint8_t can_node_id;     // Node-ID 
    uint32_t can_baudrate;    // Baudrate CAN_BPS_500*K
    uint16_t can_timeout;      // ms CAN Frame Timeout
    uint16_t encoder_cpr;     // Odrive Encoder cpr
	float motor_current_limit;
	float controller_vel_limit;
	float controller_vel_gain;
	float controller_vel_integrator_gain;
    uint16_t gear_motor;          // Gear at Motor shaft
    uint16_t gear_spindle;          // Gear at Spindle shaft
    // uint16_t cooling_after_time;  // Time in seconds spindle cooling after spindle stop
    // uint16_t fan_temp_0;      // Temperatur 0% = value C°
    // uint16_t fan_temp_100;    // Temperatur 100% = value C°
    // uint16_t load_max;        // Max Load value
	bool ovr_feed;
	float ovr_cur;
} odrive_settings_t;

static nvs_address_t nvs_address;
static settings_changed_ptr settings_changed;
static odrive_settings_t odrive;

static const setting_group_detail_t odrive_groups [] = {
    { Group_Spindle, Group_All, "Spindle ODrive"},
};

static const setting_detail_t odrive_settings[] = {
    { Settings_Odrive_useRatio, Group_Spindle, "Odrive use Ratio", NULL, Format_Bool, NULL, NULL, NULL, Setting_NonCore, &odrive.use_ratio, NULL, NULL },
    { Settings_Odrive_invert_RPM, Group_Spindle, "Odrive invert setpoint", NULL, Format_Bool, NULL, NULL, NULL, Setting_NonCore, &odrive.invert_rpm, NULL, NULL },
    { Settings_Odrive_can_baudrate, Group_Spindle, "Odrive CAN Baudrate", "125-1000K", Format_Integer, "###0", "125", "1000", Setting_NonCore, &odrive.can_baudrate, NULL, NULL },
    { Settings_Odrive_can_node_id, Group_Spindle, "Odrive Axis Node ID", NULL, Format_Int16, "###0", "0", "65535", Setting_NonCore, &odrive.can_node_id, NULL, NULL },
    { Settings_Odrive_can_timeout, Group_Spindle, "Odrive CAN Timeout", "milliseconds", Format_Int8, "##0", "0", "250", Setting_NonCore, &odrive.can_timeout, NULL, NULL },
    { Settings_Odrive_encoder_cpr, Group_Spindle, "Odrive Encoder CPR", NULL, Format_Int16, "####0", NULL, "65535", Setting_NonCore, &odrive.encoder_cpr, NULL, NULL },
    { Settings_Odrive_motor_current_limit, Group_Spindle, "Odrive Motor cur_limit", NULL, Format_Decimal, "###0.00", NULL, NULL, Setting_NonCore, &odrive.motor_current_limit, NULL, NULL },
    { Settings_Odrive_controller_vel_limit, Group_Spindle, "Odrive Controller vel_limit", "turn/s", Format_Decimal, "#0.0", "12.0", "60.0", Setting_NonCore, &odrive.controller_vel_limit, NULL, NULL },
    { Settings_Odrive_controller_vel_gain, Group_Spindle, "Odrive Controller vel_gain", "Nm/(turn/s)", Format_Decimal, "##0.00", NULL, NULL, Setting_NonCore, &odrive.controller_vel_gain, NULL, NULL },
    { Settings_Odrive_controller_vel_integral_gain, Group_Spindle, "Odrive vel_integrator_gain", "Nm/(turn/s)*s", Format_Decimal, "##0.00", NULL, NULL, Setting_NonCore, &odrive.controller_vel_integrator_gain, NULL, NULL },
    { Settings_Odrive_gear_motor, Group_Spindle, "Odrive Gear on Motor", "T", Format_Int8, "#0", "0", "255", Setting_NonCore, &odrive.gear_motor, NULL, NULL },
    { Settings_Odrive_gear_spindle, Group_Spindle, "Odrive Gear on Spindle", "T", Format_Int8, "#0", "0", "255", Setting_NonCore, &odrive.gear_spindle, NULL, NULL },
    { Settings_Odrive_ovr_feed, Group_Spindle, "Odrive override FEED on err", NULL, Format_Bool, NULL, NULL, NULL, Setting_NonCore, &odrive.ovr_feed, NULL, NULL },
    { Settings_Odrive_ovr_curit, Group_Spindle, "Odrive override CURRENT on err", "0=Off", Format_Decimal, "#0.00", "0.0", "5.0", Setting_NonCore, &odrive.ovr_cur, NULL, NULL },
};

static void odrive_settings_save (void)
{
    hal.nvs.memcpy_to_nvs(nvs_address, (uint8_t *)&odrive, sizeof(odrive_settings_t), true);
}

static void odrive_settings_restore (void)
{
    odrive.use_ratio = DEFAULT_ODRIVE_SPINDLE_USE_RATIO;
	odrive.invert_rpm = DEFAULT_ODRIVE_SPINDLE_INVERT_RPM;
	odrive.can_baudrate = (uint32_t)(CAN_BAUD_500K / 1000);
	odrive.can_node_id = DEFAULT_ODRIVE_SPINDLE_NODE_ID;
	odrive.can_timeout = DEFAULT_ODRIVE_CAN_TIMEOUT;
	odrive.encoder_cpr = DEFAULT_ODRIVE_SPINDLE_PPR;
	odrive.motor_current_limit = 60.0f;
	odrive.controller_vel_limit = 120.0f;
	odrive.controller_vel_gain = 0.0f;
	odrive.controller_vel_integrator_gain = 0.0f;
	odrive.gear_motor = DEFAULT_ODRIVE_GEAR_1;
	odrive.gear_spindle = DEFAULT_ODRIVE_GEAR_2;
	odrive.ovr_feed = true;
	odrive.ovr_cur = 0.0f;

    hal.nvs.memcpy_to_nvs(nvs_address, (uint8_t *)&odrive, sizeof(odrive_settings_t), true);
}

static void odrive_settings_load (void)
{
    if(hal.nvs.memcpy_from_nvs((uint8_t *)&odrive, nvs_address, sizeof(odrive_settings_t), true) != NVS_TransferResult_OK)
        odrive_settings_restore();
}

static setting_details_t details = {
    .groups = odrive_groups,
    .n_groups = sizeof(odrive_groups) / sizeof(setting_group_detail_t),
    .settings = odrive_settings,
    .n_settings = sizeof(odrive_settings) / sizeof(setting_detail_t),
    .save = odrive_settings_save,
    .load = odrive_settings_load,
    .restore = odrive_settings_restore
};

static setting_details_t *onReportSettings (void)
{
    return &details;
}

//--------------------------------------Odrive defines---------------------------------------

#define report_feedback report_feedback_message((message_code_t)(-1))
#define set_msg_id(cmd,node_id) ((node_id << 5) + cmd)
#define get_node_id(msgID) ((msgID >> 5) & 0x03F)				// Upper 6 bits
#define get_cmd_id(msgID) (msgID & 0x01F)						// Bottom 5 bits

typedef enum Odrive_axis_state{ //Odrive Axisstate
    AXIS_STATE_UNDEFINED = 0,                   //<! will fall through to idle
    AXIS_STATE_IDLE = 1,                        //<! disable PWM and do nothing
    AXIS_STATE_STARTUP_SEQUENCE = 2,            //<! the actual sequence is defined by the config.startup_... flags
    AXIS_STATE_FULL_CALIBRATION_SEQUENCE = 3,   //<! run all calibration procedures, then idle
    AXIS_STATE_MOTOR_CALIBRATION = 4,           //<! run motor calibration
    AXIS_STATE_SENSORLESS_CONTROL = 5,          //<! run sensorless control
    AXIS_STATE_ENCODER_INDEX_SEARCH = 6,        //<! run encoder index search
    AXIS_STATE_ENCODER_OFFSET_CALIBRATION = 7,  //<! run encoder offset calibration
    AXIS_STATE_CLOSED_LOOP_CONTROL = 8,         //<! run closed loop control
    AXIS_STATE_LOCKIN_SPIN = 9,                 //<! run lockin spin
    AXIS_STATE_ENCODER_DIR_FIND = 10,
}Odrive_axis_state;

typedef enum Odrive_can_msg{
        MSG_CO_NMT_CTRL = 0x000,  // CANOpen NMT Message REC
        MSG_ODRIVE_HEARTBEAT,
        MSG_ODRIVE_ESTOP,
        MSG_GET_MOTOR_ERROR,  // Errors
        MSG_GET_ENCODER_ERROR,
        MSG_GET_SENSORLESS_ERROR,
        MSG_SET_AXIS_NODE_ID,
        MSG_SET_AXIS_REQUESTED_STATE,
        MSG_SET_AXIS_STARTUP_CONFIG,
        MSG_GET_ENCODER_ESTIMATES,
        MSG_GET_ENCODER_COUNT,
        MSG_SET_CONTROLLER_MODES,
        MSG_SET_INPUT_POS,
        MSG_SET_INPUT_VEL,
        MSG_SET_INPUT_TORQUE,
        MSG_SET_LIMITS,
        MSG_START_ANTICOGGING,
        MSG_SET_TRAJ_VEL_LIMIT,
        MSG_SET_TRAJ_ACCEL_LIMITS,
        MSG_SET_TRAJ_INERTIA,
        MSG_GET_IQ,
        MSG_GET_SENSORLESS_ESTIMATES,
        MSG_RESET_ODRIVE,
        MSG_GET_VBUS_VOLTAGE,
        MSG_CLEAR_ERRORS,
        MSG_GET_TEMPERATURE,
		MSG_SET_VEL_GAINS,
		MSG_RESET_COUNT,
        MSG_CO_HEARTBEAT_CMD = 0x700  // CANOpen NMT Heartbeat  SEND
} Odrive_can_msg;

typedef enum odrive_mcode_t{
    Odrive_Set_Current_Limit = 200, //200
    Odrive_Reboot,// = UserMCode_Generic1, //101
    Odrive_Report_Stats,// = UserMCode_Generic2, //102
    // UserMCode_Generic3 = 103,
    // UserMCode_Generic4 = 104,
} odrive_mcode_t;

typedef struct axis_parameters_t{
    uint8_t axis_state;
    uint8_t axis_error;
    bool isAlive;
    int32_t count_pos;
    int32_t count_cpr;
    float pos_estimate;
    float vel_estimate;
    float vbus;
    float ibus;
    float temp_fet;
    float temp_motor;
	float lim_current;
	float lim_vel;
    uint32_t last_heartbeat;
    uint8_t last_state;
}axis_parameters_t;

typedef struct encoder_frame_timer_t{
    volatile uint32_t last_index_us;   // Timer value at last encoder index pulse
    volatile uint32_t last_pulse_us;   // Timer value at last encoder pulse
    volatile uint32_t pulse_length_us; // Last timer tics between spindle encoder pulse interrupts.
	volatile uint32_t frame_time_us;
} encoder_frame_timer_t;

typedef enum fan_state{ //Fan_state
	FAN_IDLE,
	FAN_START,
	FAN_ON,
    FAN_WAIT_STOP,
	FAN_STOP
}fan_state;

typedef struct fan_t{ //Fan
    uint16_t value;
	uint16_t last_value;
    uint8_t state;
    uint8_t last_state;
    uint16_t last_temperatur;
}fan_t;

typedef union flag_t{
    struct {
        uint8_t timeout          :1,
                got_error        :1,
                got_rpm_diff     :1,
                ovr_active    	 :1,
                wait_off      	 :1,
                wait_accel    	 :1,
                motor_error      :1,
                temperatur_error :1;
    };
} flag_t;

typedef enum rpm_diff_state_t{
	DIFF_IDLE,
	DIFF_OVR_ON,
	DIFF_STATE_HOLD,
	DIFF_RESTORE,
}rpm_diff_state_t;

typedef struct rpm_diff_t{
	rpm_diff_state_t state;
	uint32_t t_start;
	uint32_t t_last;
	uint_fast8_t f_start;
	float c_start;
	float max_current;
	float max_rpm_diff;
}rpm_diff_t;

typedef struct periodic_frame_t {
    volatile bool active;
    uint8_t node_id;
	uint8_t cmd_id;
    uint16_t interval;
	uint32_t last;
    struct periodic_frame_t *next;
}periodic_frame_t; 

static periodic_frame_t frames_periodic[] = {
        {Off, -1, MSG_GET_VBUS_VOLTAGE, 	100, 0 , NULL},
        // {Off, NULL, MSG_GET_ENCODER_ESTIMATES,	50, 0, NULL},
        {Off, -1, MSG_GET_ENCODER_COUNT,  1, 0, NULL},
        {Off, -1, MSG_GET_TEMPERATURE, 	500, 0, NULL}
};
static periodic_frame_t *encoder_count_frame = NULL;
static periodic_frame_t *next_frame = NULL;

static axis_parameters_t sp_p ={0};
// static fan_t sp_fan = {0};
static flag_t flag = {0};
static rpm_diff_t diff = {0};

static float rpm_diff = 0.0f, rpm_tol;
static spindle_state_t sp_state = {0};
static spindle_data_t sp_data = {0};
static CANListener odrv_listener = {0};

static spindle_encoder_t spindle_encoder;
static spindle_sync_t spindle_tracker;
static spindle_encoder_counter_t encoder;
static encoder_frame_timer_t encoder_timer;

static on_execute_realtime_ptr on_execute_realtime;
static on_realtime_report_ptr on_realtime_report;
static on_unknown_feedback_message_ptr on_unknown_feedback_message;
static on_program_completed_ptr on_program_completed;
spindle_data_t *spindleGetData (spindle_data_request_t request);
static on_report_options_ptr on_report_options;
static user_mcode_ptrs_t user_mcode;
static char msg_debug[80] = {0};
static char msg_feedback[80] = {0};
static char msg_warning[80] = {0};
static void spindle_rpm_diff(sys_state_t state);
//-------------------------------Odrive CAN Connection Commands------------------------

void report_FrameData(CAN_message_t *frame){
	char msgString[80];
	sprintf(msgString, "[Frame ID:%02lu DLC:%01u Data: %02u %02u %02u %02u %02u %02u %02u %02u]",
						frame->id,frame->len,
						frame->buf[0],frame->buf[1],frame->buf[2],frame->buf[3],
						frame->buf[4],frame->buf[5],frame->buf[6],frame->buf[7]
						);
	report_message(msgString,-1);
}

void odrive_set_state(uint8_t axis_state, bool block){
	if (!sp_p.isAlive) return;
    CAN_message_t out = {0};
	out.id = set_msg_id(MSG_SET_AXIS_REQUESTED_STATE,odrive.can_node_id);
	out.len = 4;
	out.low = (uint32_t)axis_state;
	out.high = 0;
	canbus_write_blocking(&out, block);
}

void odrive_get_parameter(uint8_t cmd, bool block){
	if (!sp_p.isAlive)
		return;
    CAN_message_t out = {0};
	out.flags.remote = 1;
	out.len = 0;
	out.id = set_msg_id(cmd,odrive.can_node_id);
	// if (block)
		canbus_write_blocking(&out,block);
	// else
		// canbus_write(&out);
}

void odrive_set_input_vel(float vel, bool block){
	if (!sp_p.isAlive){
		sprintf(msg_warning,"INPUT VEL fail! id:%d vel:%.2f odrvAlive:%d can:%d",odrive.can_node_id,vel,sp_p.isAlive,(uint8_t)canbus_connected());
		report_message(msg_warning,Message_Warning);
		memset(&msg_warning,0,sizeof(msg_warning));
		return;
	}
	if (msg_debug[0] == '\0')
		sprintf(msg_debug,"spindle_update rpm=%.0f vel=%.2f", sp_data.rpm_programmed, vel);
		
	CAN_message_t out = {0};
	out.id = set_msg_id(MSG_SET_INPUT_VEL,odrive.can_node_id);
	out.len = 4;
	uint32_t u_vel = 0;
	memcpy(&u_vel,&vel,sizeof(vel));
	out.low = u_vel;
  	out.high = 0;
	canbus_write_blocking(&out, block);
}

bool odrive_set_limits(float vel, float cur, bool block){
	if (!sp_p.isAlive){
		sprintf(msg_feedback,"SET LIMITS fail! id:%d odrvAlive:%d can:%d",odrive.can_node_id,sp_p.isAlive,(uint8_t)canbus_connected());
		report_feedback;
		return 0;
	}    
	CAN_message_t out = {0};
	out.id = set_msg_id(MSG_SET_LIMITS,odrive.can_node_id);
	out.len = 8;
	uint32_t u_vel = 0,u_cur = 0;
	memcpy(&u_vel,&vel,sizeof(vel));
	memcpy(&u_cur,&cur,sizeof(cur));
	out.low = u_vel;
  	out.high = u_cur;
	return canbus_write_blocking(&out,block);
}

void odrive_clear_errors(bool block){
    CAN_message_t out = {0};
	out.len = 0;
	out.flags.remote = 1;
	out.id = set_msg_id(MSG_CLEAR_ERRORS,odrive.can_node_id);
	canbus_write_blocking(&out, block);
}

//-------------------------------CAN Callbacks-----------------------------------------

void cb_gotFrame(CAN_message_t *frame, int mb){
  if (get_node_id(frame->id) != odrive.can_node_id) 
  	return;
  uint8_t cmd = get_cmd_id(frame->id);
  
  switch (cmd){
  	case MSG_ODRIVE_HEARTBEAT:{
		uint8_t lastState = sp_p.axis_state;
		memcpy(&sp_p.axis_error, &frame->buf[0], sizeof(uint8_t));
    	memcpy(&sp_p.axis_state, &frame->buf[4], sizeof(uint8_t));
		if (sp_p.axis_state != lastState)
			sp_p.last_state = lastState;
		sp_state.on = sp_p.axis_state == AXIS_STATE_CLOSED_LOOP_CONTROL ? On : Off;
		flag.got_error = sp_p.axis_error == 0 ? Off : On;
		sp_p.last_heartbeat = hal.get_elapsed_ticks();
		if (!sp_p.isAlive){
			sp_p.isAlive = On;
			for(uint8_t idx = 0; idx < (sizeof(frames_periodic) / sizeof(periodic_frame_t)); idx++){
				frames_periodic[idx].active = On;
				frames_periodic[idx].last = 0UL;
			}
			hal.spindle.get_data = spindleGetData;
			report_message("CAN connected",Message_Info);
			odrive_set_limits(odrive.controller_vel_limit,odrive.motor_current_limit,false);
			// if (sp_p.lim_current == 0.0f){
			// 	odrive_get_parameter(MSG_SET_LIMITS,true);	
			// }
		}
		// sprintf(feedback_msg,"Frame HEARTBEAT can:%d alive:%d state:%d lastState:%d last:%d",canbus_connected(),sp_p.isAlive,sp_p.axis_state,sp_p.last_state,sp_p.last_heartbeat);
		// report_feedback;
  	    break;}
  	case MSG_GET_VBUS_VOLTAGE:{
  	  	memcpy(&sp_p.vbus, &frame->low, sizeof(float));
		memcpy(&sp_p.ibus, &frame->high, sizeof(float));
  	  	// sprintf(feedback_msg,"Frame VBUS_VOLTAGE vbus:%0.2fV ibus:%0.2fA",sp_p.vbus,sp_p.ibus);
		// report_feedback;
  	  	break;}
  	case MSG_GET_TEMPERATURE:{
		memcpy(&sp_p.temp_fet, &frame->low, sizeof(float));
		memcpy(&sp_p.temp_motor, &frame->high, sizeof(float));
  	  	// sprintf(feedback_msg,"Frame TEMPERATURE fet:%0.2f°C motor:%0.2f°C",sp_p.temp_fet,sp_p.temp_motor);
		// report_feedback;
  	  	break;}
  	case MSG_SET_LIMITS:{
		memcpy(&sp_p.lim_vel, &frame->low, sizeof(float));
		memcpy(&sp_p.lim_current, &frame->high, sizeof(float));
  	  	// if (msg_feedback[0] == '\0'){
		// 	sprintf(msg_feedback,"LIMITS lim_vel:%.0f lim_cur:%.0f",sp_p.lim_vel,sp_p.lim_current);
		// 	report_feedback;
		// }
  	  	break;}
  	case MSG_GET_ENCODER_COUNT:{
		uint32_t now_us = micros();
		// spindle_encoder.timer.last_pulse = micros();
		encoder_timer.frame_time_us = now_us - encoder_timer.last_pulse_us;
		encoder_timer.last_pulse_us = now_us;
		spindle_encoder.counter.last_index = sp_p.count_pos / odrive.encoder_cpr;
		spindle_encoder.counter.last_count = sp_p.count_cpr;
  	  	memcpy(&sp_p.count_pos, &frame->low, sizeof(sp_p.count_pos));
  	  	memcpy(&sp_p.count_cpr, &frame->high, sizeof(sp_p.count_cpr));
		sp_data.angular_position = (360.0f/odrive.encoder_cpr) * sp_p.count_cpr;
		spindle_encoder.counter.index_count = sp_p.count_pos / odrive.encoder_cpr;
		spindle_encoder.counter.pulse_count = sp_p.count_pos;
		sp_data.angular_position = (float)encoder.index_count +
                    ((float)(sp_data.pulse_count) * (1.0f / odrive.encoder_cpr));
                            //  (pulse_length == 0 ? 0.0f : (float)rpm_timer_delta / (float)pulse_length)) *
  	  	static uint32_t next_print;
		if (next_print <= hal.get_elapsed_ticks()){
			next_print = hal.get_elapsed_ticks() + 500;
			sprintf(msg_feedback,"cpr:%04ld apos:%.3f index:%lu time:%lu", 
				sp_p.count_cpr, sp_data.angular_position, sp_data.index_count, encoder_timer.frame_time_us);
			report_feedback;
			// encoder_timer.frame_time_us = -1;
		}
  	  	break;}
  	case MSG_GET_ENCODER_ESTIMATES:{
  	    memcpy(&sp_p.pos_estimate, &frame->low, sizeof(sp_p.pos_estimate));
    	memcpy(&sp_p.vel_estimate, &frame->high, sizeof(sp_p.vel_estimate));
		sp_data.rpm = ((sp_p.vel_estimate * (sp_p.vel_estimate < 0.0f ? -60.0f : 60.0f)) /  
				odrive.gear_spindle) * odrive.gear_motor;
		bool atSpeed = settings.spindle.at_speed_tolerance <= 0.0f ? On : 
				(sp_data.rpm >= sp_data.rpm_low_limit && sp_data.rpm <= sp_data.rpm_high_limit);
		if(flag.wait_accel){
			flag.wait_accel = !atSpeed;
		}
		sp_state.at_speed = !flag.wait_accel && !flag.wait_off ? atSpeed : Off;

		rpm_diff = (!atSpeed && !flag.wait_accel && !flag.wait_off && sp_data.rpm_programmed > 0.0f) || rpm_diff > 0.0f ?  
					sp_data.rpm_programmed - min(sp_data.rpm,sp_data.rpm_programmed) : 0.0f;

		if (!flag.got_rpm_diff && state_get() == STATE_CYCLE)
			protocol_enqueue_rt_command(spindle_rpm_diff);
  	    // sprintf(feedback_msg,"Frame ENC EST pos:%.2f vel:%.4f",sp_p.pos_estimate,sp_p.vel_estimate);
		// report_feedback;
  	    break;}
  	default:
  	  	break;
  	}
}

//-------------------------------------------------------------------------------------

void spindle_rpm_diff(sys_state_t state){
	uint32_t ms = hal.get_elapsed_ticks();
	static uint16_t org_enc_cound_period = {0};
	static uint32_t last_feed = {0};
	static uint32_t last_cur = {0};
	bool got_diff = rpm_diff > 0.0f && !flag.wait_off;
	bool check = 0;
	bool timeout = 0;
	bool ovr_cur_active = odrive.ovr_cur > 0.0f;
	bool ovr_feed_active = odrive.ovr_feed;
	UNUSED(state);
	
	if (got_diff || diff.state != DIFF_IDLE){

		switch (diff.state)
		{
		case DIFF_IDLE:
			if (ovr_cur_active && sp_p.lim_current == 0.0f){
				if (sp_p.lim_current == 0.0f){
					odrive_get_parameter(MSG_SET_LIMITS,true);
				}
				return;
			}
			diff.f_start = ovr_feed_active ? (uint_fast8_t)sys.override.feed_rate : 0;
			diff.c_start = ovr_cur_active && !(sp_p.lim_current == 0.0f) ? sp_p.lim_current : 0.0f;
			diff.t_start = diff.t_last = ms - 20;
			org_enc_cound_period = encoder_count_frame->interval;
			encoder_count_frame->interval = 15;
			encoder_count_frame->last = ms - 10;
			// sprintf(msg_feedback,"new ovr state=%u start=%u act=%u diff=%.f c_on=%u start_c=%.1f act_c=%.1f", 
			// 	(uint8_t)diff.state,diff.f_start,sys.override.feed_rate,rpm_diff,ovr_cur_active,diff.c_start,sp_p.lim_current);
			// report_feedback;
			diff.state = DIFF_OVR_ON;
			
		case DIFF_OVR_ON:{
			if (ms >= (diff.t_last + 150)){
				diff.state = DIFF_RESTORE;
				return;
			}

			check = ms >= diff.t_last + 15;
			timeout = ms >= diff.t_start + 800;
			if (!check || !got_diff)
				return;
			
			if (timeout){
				diff.state = DIFF_STATE_HOLD;
				sprintf(msg_feedback,"Still RPM diff=%3.0f Stoping FEED",rpm_diff);
				report_feedback;
				protocol_enqueue_realtime_command(CMD_FEED_HOLD);
				system_set_exec_state_flag(EXEC_FEED_HOLD);
				break;
			}

			if(diff.f_start){
				enqueue_feed_override(CMD_OVERRIDE_FEED_FINE_MINUS);
			}

			if (diff.c_start){
				float add_val = odrive.ovr_cur * sp_data.rpm_programmed < 1200.0 ? 1.5f : 1.25f;
				odrive_set_limits(sp_p.lim_vel, (sp_p.lim_current + add_val) > 60.0f ? 60.0f : sp_p.lim_current + add_val, true);
				// sprintf(msg_feedback,"plus ovr=%u f_s=%u f=%u d=%.f c_s=%.1f c=%.1f c_n=%.1f", 
				// 	(uint8_t)diff.state,diff.f_start,sys.override.feed_rate,rpm_diff,diff.c_start,sp_p.lim_current,sp_p.lim_current + add_val);
				// report_feedback;
			}

			// sprintf(msg_feedback,"ovr state=%u start=%u act=%u diff=%.f atS=%u cur=%.1f", 
			// 	(uint8_t)diff.state,diff.f_start,sys.override.feed_rate,rpm_diff,sp_state.at_speed,sp_p.lim_current);
			// report_feedback;
			diff.t_last = ms;

			break;}
		case DIFF_STATE_HOLD:{
			timeout = ms >= diff.t_last + 500;
			if(timeout && got_diff){
				sprintf(msg_feedback,"Still RPM diff - stoping Spindle...");
				report_feedback;
				hal.spindle.set_state((spindle_state_t){0}, 0.0f);
				system_set_exec_state_flag(EXEC_STATUS_REPORT);
				diff.state = DIFF_RESTORE;
				diff.t_last = ms;
				return;
			}
			else if(!got_diff){
				diff.state = DIFF_RESTORE;
				diff.t_last = ms;
				return;
			}
			break;}
		case DIFF_RESTORE:{
			check = ms >= (diff.t_last + 25);
			if (got_diff){
				diff.t_start = diff.t_last = ms ;
				diff.state = DIFF_OVR_ON;
				return;
			}

			if (!check)
				return;
				
			// diff.t_last = ms;
			// float current_now = sp_p.lim_current;
			bool restore_feed =  (uint_fast8_t)sys.override.feed_rate != diff.f_start;
			bool restore_current = ovr_cur_active && (sp_p.lim_current != diff.c_start);

			if (!restore_current && !restore_feed){
				encoder_count_frame->interval = org_enc_cound_period;
				org_enc_cound_period = 0;
				encoder_count_frame->last = ms;
				diff = (rpm_diff_t){0};
				// diff.state = DIFF_IDLE;
				// sprintf(msg_feedback,"end ovr state=%u start=%u act=%u diff=%.f start_c=%.1f act_c=%.1f", 
				// 	(uint8_t)diff.state,diff.f_start,sys.override.feed_rate,rpm_diff,diff.c_start,sp_p.lim_current);
				// report_feedback;
				break;
			}

			if (restore_feed && ms >= (last_feed + 20)){
				enqueue_feed_override(CMD_OVERRIDE_FEED_FINE_PLUS);
				last_feed = ms;
			}

			if (restore_current && ms >= (last_cur + 50)){
				last_cur = ms;
				float add_val = sp_p.lim_current - (odrive.ovr_cur * (sp_data.rpm_programmed > 1200.0 ? 1.5f : 1.25f));
				float new_val = add_val < diff.c_start ? diff.c_start : add_val;
				odrive_set_limits(sp_p.lim_vel,new_val,false);
				// sprintf(msg_feedback,"minus ovr=%u f_s=%u f_n=%u d=%.f r_f=%u r_c=%u c_s=%.1f c=%.1f c_n=%.1f", 
				// 	(uint8_t)diff.state,diff.f_start,sys.override.feed_rate,rpm_diff,restore_feed,restore_current,diff.c_start,sp_p.lim_current,new_val);
				// report_feedback;
			}

			// else {
			// 	sprintf(msg_feedback,"ovr state=%u start=%u act=%u diff=%.f atS=%u start=%.1f cur=%.1f", 
			// 		(uint8_t)diff.state,diff.f_start,sys.override.feed_rate,rpm_diff,sp_state.at_speed,diff.c_start,sp_p.lim_current);
			// 	report_feedback;
			// }
			diff.t_last = ms;
			break;}
		}
	}

	flag.got_rpm_diff = diff.state == DIFF_IDLE ? Off : On;
}

static void spindle_update(void){
	float out_vel = 0.0f;
	bool invert = (!sp_data.state_programmed.ccw && odrive.invert_rpm) || (sp_data.state_programmed.ccw && !odrive.invert_rpm);
	out_vel = ((((sp_data.rpm_programmed) / 60.0f)/odrive.gear_motor) * odrive.gear_spindle);
	out_vel = invert ? -1.0f * out_vel : 1.0f * out_vel;
	odrive_set_input_vel(out_vel,true);
}

static void spindle_on(sys_state_t state){
	if (!sp_p.isAlive) return;
	
	if (odrive.motor_current_limit > 0.0f && sp_p.lim_current != odrive.motor_current_limit){
		sp_p.lim_current = 0.0f;
		odrive_set_limits(sp_p.lim_vel, odrive.motor_current_limit,true);
	}
	
	if (sp_p.axis_state == AXIS_STATE_IDLE){
		odrive_set_state(AXIS_STATE_CLOSED_LOOP_CONTROL,true);
	}
	// else {
	// 	static uint32_t _timeout = 0;
	// 	_timeout = !_timeout ? hal.get_elapsed_ticks() : _timeout;

	// }
}

static void spindle_off(sys_state_t state){
	if(!sp_p.isAlive) return;
	
	if(!flag.wait_off)
		flag.wait_off = On;
	
	bool running = sp_data.rpm > 50.0f;
	if (!running){
		flag.wait_off = Off;
		if (sp_p.axis_state != AXIS_STATE_STARTUP_SEQUENCE && sp_p.axis_state != AXIS_STATE_IDLE)
			odrive_set_state(AXIS_STATE_IDLE,true);
	}
	else
		protocol_enqueue_rt_command(spindle_off);
}

static void spindleUpdateRPM(float rpm)
{
	// sp_state.at_speed = Off;
	flag.wait_accel = rpm > sp_data.rpm && sp_p.isAlive ? On : Off;
    sp_data.rpm_programmed = rpm;
	// flag.wait_off = rpm == 0.0f && sp_p.isAlive ? On : Off;
	if(settings.spindle.at_speed_tolerance > 0.0f) {
		rpm_tol = rpm > 0.0f && rpm <= 250.0f ? (250.0f / rpm) * settings.spindle.at_speed_tolerance : settings.spindle.at_speed_tolerance;
        sp_data.rpm_low_limit = rpm == 0.0f ? 0.0f : rpm / (1.0f + rpm_tol);
        sp_data.rpm_high_limit = rpm == 0.0f ? 0.0f : rpm * (1.0f + rpm_tol);
	}
  	spindle_update();
}

static void spindleSetState(spindle_state_t state, float rpm)
{
	sp_data.state_programmed = state;
	sp_data.rpm_programmed = rpm;
	sp_state.ccw = state.ccw;
	// sp_state.at_speed = Off;
	if (!sp_p.isAlive){
		sp_data.rpm = rpm;
		sp_state.on = state.on;
	}
	else {
  		if (!state.on || (state.on && rpm == 0.0f)) {
			spindleUpdateRPM(rpm);
    		spindle_off(state_get());
  		} else {
    		spindle_on(state_get());
    		spindleUpdateRPM(rpm);
  		}
	}
}

static spindle_state_t spindleGetState (void)
{
  spindle_state_t state = {0};
  if (sp_p.isAlive){
	  return sp_state;
	// state.on = sp_state.on;
	// state.ccw = sp_state.ccw;
	// state.at_speed = sp_state.at_speed;
  }
  else{
	state.on = sp_data.state_programmed.on;
	state.ccw = sp_data.state_programmed.ccw;
	state.at_speed = On;
  }
  return state;
}

spindle_data_t *spindleGetData (spindle_data_request_t request)
{
    // bool stopped;
    uint32_t frame_length_us;
    // spindle_encoder_counter_t encoder;

    // __disable_irq();

    // memcpy(&encoder, &spindle_encoder.counter, sizeof(spindle_encoder_counter_t));

    // pulse_length = spindle_encoder.timer.pulse_length / spindle_encoder.tics_per_irq;
    // rpm_timer_delta = GPT1_CNT - spindle_encoder.timer.last_pulse;

    // __enable_irq();

    // // If no (4) spindle pulses during last 250 ms assume RPM is 0
    // if((stopped = ((pulse_length == 0) || (rpm_timer_delta > spindle_encoder.maximum_tt)))) {
    //     sp_data.rpm = 0.0f;
    //     rpm_timer_delta = (GPT2_CNT - spindle_encoder.counter.last_count) * pulse_length;
    // }

    switch(request) {

        case SpindleData_Counters:
            // sp_data.pulse_count = GPT2_CNT;
            // sp_data.index_count = spindle_encoder.index_count;
            // sp_data.error_count = spindle_encoder.error_count;
            break;

        case SpindleData_RPM:
            // if(!stopped)
                // sp_data.rpm = spindle_encoder.rpm_factor / (float)pulse_length;
            break;

        case SpindleData_AngularPosition:
			odrive_get_parameter(set_msg_id(MSG_GET_ENCODER_COUNT,odrive.can_node_id),true);
			// encoder.last_count = micros();
            // int32_t d = encoder.last_count - encoder.last_index;
            // sp_data.angular_position = (float)encoder.index_count +
            //         ((float)(d) +
            //                  (pulse_length == 0 ? 0.0f : (float)rpm_timer_delta / (float)pulse_length)) *
            //                     spindle_encoder.pulse_distance;
            break;
    }

    return &sp_data;
}

static void spindleDataReset (void)
{
    spindle_encoder.timer.last_pulse =
    spindle_encoder.timer.last_index = 0;

    spindle_encoder.timer.pulse_length =
    spindle_encoder.counter.last_count =
    spindle_encoder.counter.last_index =
    spindle_encoder.counter.pulse_count =
    spindle_encoder.counter.index_count =
    spindle_encoder.error_count = 0;
	
	encoder_timer.frame_time_us =
	encoder_timer.last_index_us =
	encoder_timer.last_pulse_us =
	encoder_timer.pulse_length_us = 0;
}

//-------------------------------------Report--Feedback--Realtime---------------------

void sp_feedback_message(stream_write_ptr stream_write){
	if(on_unknown_feedback_message)
        on_unknown_feedback_message(stream_write);
	
	if (msg_feedback[0] != '\0'){
		stream_write(msg_feedback);
		memset(msg_feedback,0,sizeof(msg_feedback));
	}
}

void sp_rt_report(stream_write_ptr stream_write, report_tracking_flags_t report){
	// static report_tracking_flags_t last_report = {0};
	static char rt_report_msg[30] = {0};
	static uint8_t sp_counter = REPORT_WCO_REFRESH_IDLE_COUNT / 2;

	if (sp_counter > 0 && !sys.report.wco)
        sp_counter--;
    else
        sp_counter = REPORT_WCO_REFRESH_IDLE_COUNT / 2;
	
	if(on_realtime_report)
        on_realtime_report(stream_write, report);

	if (report.motor || !sp_counter){
		stream_write("|ODRV:");
    	sprintf(rt_report_msg,"%2.1f,%4.3f,%3.1f",sp_p.temp_motor > 0.0f ? sp_p.temp_motor : sp_p.temp_fet,
												sp_p.ibus, sp_p.ibus > 0.0f ? sp_p.ibus * sp_p.vbus : 0.0f);
		stream_write(rt_report_msg);
	}

	// memcpy(&last_report,&report,sizeof(report));
}

void sp_ms_event(){
	uint32_t ms = hal.get_elapsed_ticks();
	static uint32_t last_1ms = 0, last_50ms = 0;

	if (ms >= last_1ms + 1){
		last_1ms = ms;
		
	}
  
	}

	if (ms >= last_50ms + 50){
		last_50ms = ms;
		// if(diff.state != DIFF_IDLE && sp_p.isAlive) {
		// 	// if (flag.got_rpm_diff){
		// 	sprintf(msg_debug,"Override state=%u start=%u act=%u diff=%.f atS=%u cur=%.1f", 
		// 		(uint8_t)diff.state,diff.f_start,sys.override.feed_rate,rpm_diff,sp_state.at_speed,sp_p.lim_current);
		// 	report_message(msg_debug,Message_Plain);
		// 	memset(&msg_debug,0,sizeof(msg_debug));
		// 	// }
		// }
	}
}

void sp_execute_realtime(uint_fast16_t state){
	static uint32_t last_ms;
	on_execute_realtime(state);
    UNUSED(state);
    uint32_t ms = hal.get_elapsed_ticks();
	

	periodic_frame_t *sent_msg = next_frame;
	next_frame = sent_msg->next;
	if(sent_msg->active && (ms >= (sent_msg->last + sent_msg->interval))) {
		sent_msg->last = ms;
		odrive_get_parameter(sent_msg->cmd_id,false);
	}

    if (ms != last_ms){
		last_ms = ms;

		// if (flag.wait_off)
			// spindle_off();

		if (diff.state != DIFF_IDLE)
			spindle_rpm_diff(state);

		sp_ms_event();

	// if (sp_p.isAlive){
	// 	if (flag.got_rpm_diff){
	// 		spindle_rpm_diff();
	// 		// if (sys.suspend)
	// 			// protocol_execute_rt();
	// 			// protocol_execute_realtime();
	// 			// protocol_enqueue_rt_command(on_execute_realtime);
	// 	}
	//
	// 	if ((ms - sp_p.last_heartbeat) > odrive.can_timeout){
	// 		sp_p.isAlive = false;
	// 		memset(&sp_p,0,sizeof(sp_p));
	// 		for(uint_fast8_t idx = 0; idx < (sizeof(frames_periodic) / sizeof(periodic_frame_t)); idx++)
	// 			frames_periodic[idx].active = Off;
	// 		hal.spindle.get_data = NULL;
	// 		report_message("CAN timeout",Message_Info);
	// 	}
	// 	periodic_frame_t *sent_msg = next_frame;
	// 	next_frame = sent_msg->next;
	// 	if(sent_msg->active && (ms > (sent_msg->last + sent_msg->interval))) {
	// 		sent_msg->last = ms;
	// 		odrive_get_parameter(sent_msg->cmd_id,false);
	// 	}
	//
	// 	// static uint32_t print_ticks = 250;
	//     // if(!(--print_ticks) || gotDiff) {
    // 	//     // sprintf(msg_debug,"Spindel Angel:%3.2f",sp_data.angular_position);
	// 	// 	// report_feedback;
	// 	// 	// float tol = settings.spindle.at_speed_tolerance;
	// 	// 	int diff_max = sp_data.rpm_programmed * rpm_tol;
	// 	// 	sprintf(msg_debug,"RPM diff=%3.1f max=%d tol=%.2f on=%u soll=%.0f ist=%.0f atS=%d waitS=%d flag=%d",
	// 	// 		rpm_diff, diff_max, rpm_tol, sp_state.on, sp_data.rpm_programmed, sp_data.rpm, sp_state.at_speed, flag.wait_accel, gotDiff);
	// 	// 	report_feedback;
    //     // 	print_ticks = gotDiff ? 10 : 250;
    // 	// }
	// }
	}
}

//----------------------------Init and Setup-----------------------------------------

static void onReportOptions (bool newopt)
{
    on_report_options(newopt);

    if(!newopt) {
        hal.stream.write("[PLUGIN:ODRIVE CAN v0.01]" ASCII_EOL);
    }
}

static void odrive_warning (uint_fast16_t state)
{
	if (msg_warning[0] != 0){
		report_message(msg_warning, Message_Plain);
		memset(&msg_warning,0,sizeof(msg_warning));
	}
	if(nvs_address == 0 && msg_warning[0] == 0)
    	report_message("ODrive failed to initialize! nvs_address=0", Message_Warning);
}

static void report_rpm_override_stats (sys_state_t state)
{	
	char msg_out[40];
    sprintf(msg_out,"Spindel Angel=%3.2f Pos=%04u",sp_data.angular_position, (uint16_t)(sp_p.count_cpr));
	hal.stream.write(msg_out);
	hal.stream.write(ASCII_EOL);
    // format output -> T:21.17 /0.0000 B:21.04 /0.0000 @:0 B@:0
}

static void odrive_on_program_completed (program_flow_t program_flow, bool check_mode)
{
    bool completed = program_flow == ProgramFlow_Paused; // || program_flow == ProgramFlow_CompletedM30;

    // if(frewind) {
    //     f_lseek(file.handle, 0);
    //     file.pos = file.line = 0;
    //     file.eol = false;
    //     hal.stream.read = await_cycle_start;
    //     if(grbl.on_state_change != trap_state_change_request) {
    //         state_change_requested = grbl.on_state_change;
    //         grbl.on_state_change = trap_state_change_request;
    //     }
    //     protocol_enqueue_rt_command(sdcard_restart_msg);
    // } else
    //     sdcard_end_job();

    // if(on_program_completed)
        on_program_completed(program_flow, check_mode);
}

static user_mcode_t userMCodeCheck (user_mcode_t mcode)
{
    return (uint32_t)mcode == Odrive_Set_Current_Limit || (uint32_t)mcode == Odrive_Reboot || (uint32_t)mcode == Odrive_Report_Stats
            ? mcode
            : (user_mcode.check ? user_mcode.check(mcode) : UserMCode_Ignore);
}

static status_code_t userMCodeValidate (parser_block_t *gc_block, parameter_words_t *value_words)
{
    status_code_t state = Status_GcodeValueWordMissing;

    switch((uint32_t)gc_block->user_mcode) {

        case Odrive_Set_Current_Limit:
            // if(!(*value_words).value){
				state = Status_OK;
				// (*value_words).value = Off;
			// }
            break;

        case Odrive_Reboot:
        case Odrive_Report_Stats:
            state = Status_OK;
            break;

        default:
            state = Status_Unhandled;
            break;
    }

    return state == Status_Unhandled && user_mcode.validate ? user_mcode.validate(gc_block, value_words) : state;
}

static void userMCodeExecute (uint_fast16_t state, parser_block_t *gc_block)
{
    bool handled = true;

    if (state != STATE_CHECK_MODE)
      switch((uint32_t)gc_block->user_mcode) {

        case Odrive_Set_Current_Limit:
            odrive_set_limits(odrive.controller_vel_limit,gc_block->values.f, true);
			sprintf(msg_warning,"ODrive updating motor current to %02.1f",gc_block->values.f);
			protocol_enqueue_rt_command(odrive_warning);
            break;

        case Odrive_Reboot: // Request rpm override stats report
            protocol_enqueue_rt_command(report_rpm_override_stats);
            break;

        case Odrive_Report_Stats:{
			CAN_message_t msg = {0};
			msg.id = set_msg_id(MSG_GET_ENCODER_COUNT,odrive.can_node_id);
			msg.len = 8;
			msg.flags.remote = On;
			int result = canbus_write_sync_msg(&msg,true);
			sprintf(msg_feedback,"adding sync msg = %u",result);
			report_feedback;
            break;}

        case 115:
            hal.stream.write("FIRMWARE_NAME:grblHAL ");
            hal.stream.write("FIRMWARE_URL:https%3A//github.com/grblHAL ");
            hal.stream.write("FIRMWARE_VERSION:" GRBL_VERSION " ");
            hal.stream.write("FIRMWARE_BUILD:" GRBL_VERSION_BUILD ASCII_EOL);
            break;

        default:
            handled = false;
            break;
    }

    if(!handled && user_mcode.execute)
        user_mcode.execute(state, gc_block);
}

static void odrive_settings_changed (settings_t *settings)
{
    static bool init_ok = false;

    if(init_ok && settings_changed)
        settings_changed(settings);
	else {
		odrive_settings_load();
        hal.spindle.set_state = spindleSetState;
    	hal.spindle.get_state = spindleGetState;
    	hal.spindle.update_rpm = spindleUpdateRPM;
		hal.spindle.get_data = spindleGetData;
    	// hal.spindle.reset_data = spindleDataReset;
		hal.driver_cap.variable_spindle = On;
    	hal.driver_cap.spindle_at_speed = On;
    	hal.driver_cap.spindle_dir = On;
		// if(spindle_encoder.ppr != settings->spindle.ppr) {

            hal.spindle.reset_data = spindleDataReset;
            hal.spindle.set_state((spindle_state_t){0}, 0.0f);

            pidf_init(&spindle_tracker.pid, &settings->position.pid);

            float timer_resolution = 1.0f / 1000000.0f; // 1 us resolution

            spindle_tracker.min_cycles_per_tick = (int32_t)ceilf(settings->steppers.pulse_microseconds * 2.0f + settings->steppers.pulse_delay_microseconds);
            spindle_encoder.ppr = settings->spindle.ppr;
            // spindle_encoder.tics_per_irq = max(1, spindle_encoder.ppr / 32);
            spindle_encoder.pulse_distance = 1.0f / spindle_encoder.ppr;
            // spindle_encoder.maximum_tt = (uint32_t)(2.0f / timer_resolution) / spindle_encoder.tics_per_irq;
            spindle_encoder.rpm_factor = 60.0f / ((timer_resolution * (float)spindle_encoder.ppr));
            spindleDataReset();
        // }
		canbus_begin(0,1000*odrive.can_baudrate);
	}

    init_ok = true;
}

void odrive_init()
{
	if ((nvs_address = nvs_alloc(sizeof(odrive_settings_t)))) {

		memcpy(&user_mcode, &hal.user_mcode, sizeof(user_mcode_ptrs_t));

    	hal.user_mcode.check = userMCodeCheck;
    	hal.user_mcode.validate = userMCodeValidate;
    	hal.user_mcode.execute = userMCodeExecute;
    	// driver_reset = hal.driver_reset;
        // hal.driver_reset = reset;

    	settings_changed = hal.settings_changed;
    	hal.settings_changed = odrive_settings_changed;

    	on_report_options = grbl.on_report_options;
    	grbl.on_report_options = onReportOptions;

		on_execute_realtime = grbl.on_execute_realtime;
    	grbl.on_execute_realtime = sp_execute_realtime;

		on_unknown_feedback_message = grbl.on_unknown_feedback_message;
		grbl.on_unknown_feedback_message = sp_feedback_message;

		on_realtime_report = grbl.on_realtime_report;
		grbl.on_realtime_report = sp_rt_report;
	
		details.on_get_settings = grbl.on_get_settings;
    	grbl.on_get_settings = onReportSettings;

		on_program_completed = grbl.on_program_completed;
        grbl.on_program_completed = odrive_on_program_completed;

		odrive_settings_changed(&settings);

		next_frame = &frames_periodic[0];

    	for(uint8_t idx = 0; idx < (sizeof(frames_periodic) / sizeof(periodic_frame_t)); idx++){
			frames_periodic[idx].next = idx == (sizeof(frames_periodic) / sizeof(periodic_frame_t)) - 1 ? &frames_periodic[0] : &frames_periodic[idx + 1];
			frames_periodic[idx].last = 0UL;
			frames_periodic[idx].node_id = odrive.can_node_id;
			frames_periodic[idx].active = false;
			if (frames_periodic[idx].cmd_id == MSG_GET_ENCODER_COUNT)
				encoder_count_frame = &frames_periodic[idx];
		}

		odrv_listener.generalCallbackActive = On;
		odrv_listener.callbacksActive = 0ULL;
    	odrv_listener.frameHandler = (void *)&cb_gotFrame;
    	canbus_attachObj(&odrv_listener);

	}

	if(nvs_address == 0)
        protocol_enqueue_rt_command(odrive_warning);

    // return nvs_address != 0;
}

#endif