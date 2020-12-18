#include <c_types.h>
#include <eagle_soc.h> 
#include <gpio.h>
#include <driver/hw_timer.h>
#include "debug.h"


#define PWM_VAWEFORM	0b11100000 
#define UPDATE_CHANNEL_STATE(n, ch, cycle) \
	GPIO_OUTPUT_SET( \
			GPIO_ID_PIN(n), \
			!duty && (PWM_VAWEFORM >> ((7 - (cycle - ch * 2)) % 8) & 1))


#define UPDATE_CHANNELS(cycle) \
	UPDATE_CHANNEL_STATE(PWM0_NUM, 0, cycle); \
	UPDATE_CHANNEL_STATE(PWM1_NUM, 1, cycle); \
	UPDATE_CHANNEL_STATE(PWM2_NUM, 2, cycle); \
	UPDATE_CHANNEL_STATE(PWM3_NUM, 3, cycle);


#define CLEAR_CHANNELS() \
	GPIO_OUTPUT_SET(GPIO_ID_PIN(PWM0_NUM), 0); \
	GPIO_OUTPUT_SET(GPIO_ID_PIN(PWM1_NUM), 0); \
	GPIO_OUTPUT_SET(GPIO_ID_PIN(PWM2_NUM), 0); \
	GPIO_OUTPUT_SET(GPIO_ID_PIN(PWM3_NUM), 0);


LOCAL bool duty;
LOCAL int32_t ticks;
LOCAL void (*stop_callback) (void);


void motor_stop() {
    ETS_FRC1_INTR_DISABLE();
    TM1_EDGE_INT_DISABLE();
    RTC_REG_WRITE(FRC1_CTRL_ADDRESS, 0);
	CLEAR_CHANNELS();
	if (stop_callback) {
		stop_callback();
	}
}


void pwm_timer_func() {
	if (ticks == 0) {
		motor_stop();
		return;
	}
	UPDATE_CHANNELS(ticks % 8);
	duty ^= 1;
	if (duty) { 
		hw_timer_arm(PWM_DUTY);
		return;
	} 
	
	ticks += ticks > 0? -1: 1;
	hw_timer_arm(PWM_PERIOD - PWM_DUTY);
}

void motor_rotate(int32_t t) {
	if (ticks != 0) {
		ticks += t;
		return;
	}
	duty = false;
	uint32_t flags = DIVDED_BY_16 | FRC1_ENABLE_TIMER | TM_EDGE_INT;
    RTC_REG_WRITE(FRC1_CTRL_ADDRESS, flags);
    ETS_FRC_TIMER1_NMI_INTR_ATTACH(pwm_timer_func);
	ticks = t;
    TM1_EDGE_INT_ENABLE();
    ETS_FRC1_INTR_ENABLE();
	hw_timer_arm(PWM_PERIOD);
}


void motor_set_stop_callback(void (*done)(void)) {
	stop_callback = done;
}


void motor_init() {
	ticks = 0;

    ETS_FRC1_INTR_DISABLE();
    TM1_EDGE_INT_DISABLE();
    RTC_REG_WRITE(FRC1_CTRL_ADDRESS, 0);

	PIN_FUNC_SELECT(PWM0_MUX, PWM0_FUNC);
	PIN_PULLUP_DIS(PWM0_MUX);

	PIN_FUNC_SELECT(PWM1_MUX, PWM1_FUNC);
	PIN_PULLUP_DIS(PWM1_MUX);

	PIN_FUNC_SELECT(PWM2_MUX, PWM2_FUNC);
	PIN_PULLUP_DIS(PWM2_MUX);

	PIN_FUNC_SELECT(PWM3_MUX, PWM3_FUNC);
	PIN_PULLUP_DIS(PWM3_MUX);
	CLEAR_CHANNELS();
}
