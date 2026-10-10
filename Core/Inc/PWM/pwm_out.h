/*
 * PWM outputs for the F446RE boat controller.
 */
#ifndef INC_PWM_PWM_OUT_H_
#define INC_PWM_PWM_OUT_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>

typedef enum {
    PWM_CH_ESC_LEFT = 0,       // PC6, TIM3_CH1
    PWM_CH_ESC_RIGHT = 1,      // PC7, TIM3_CH2
    PWM_CH_SERVO_TURRET_X = 2, // PB0, TIM3_CH3
    PWM_CH_SERVO_TURRET_Y = 3, // PB1, TIM3_CH4
    PWM_CH_SERVO_UNSTICK = 4,  // PB6, TIM4_CH1
    PWM_CH_RESERVED_5 = 5,     // PB7, TIM4_CH2
    PWM_CH_RESERVED_6 = 6,     // PB8, TIM4_CH3
    PWM_CH_RESERVED_7 = 7      // PB9, TIM4_CH4
} pwm_output_channel_t;

void pwm_init(void);

// Standard 50 Hz signal, normalized input [-1, +1] maps to 1100-1900 us.
void pwm_set(uint8_t ch, float output);
void pwm_set_raw(uint8_t ch, uint16_t output);
uint16_t pwm_get_raw(uint8_t ch);

// IBT-2 direction/duty PWM on TIM9 CH1/CH2; speed in [-1, +1].
void pwm_set_ibt2_motor(float speed);
void pwm_set_ball_enable(uint8_t enable);

// Existing pump enable output.
void pwm_set_pump_enable(uint8_t enable);

#endif
