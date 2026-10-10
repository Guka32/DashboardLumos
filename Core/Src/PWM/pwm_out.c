#include "PWM/pwm_out.h"
#include "main.h"
#include <math.h>

extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim9;

static uint8_t initialized = 0;
static TIM_HandleTypeDef *const standard_pwm_timers[] = {&htim3, &htim4};
static const uint32_t timer_channels[] = {
    TIM_CHANNEL_1, TIM_CHANNEL_2, TIM_CHANNEL_3, TIM_CHANNEL_4
};

static float pwm_clamp(float x, float min_value, float max_value)
{
    if (x < min_value) return min_value;
    if (x > max_value) return max_value;
    return x;
}

void pwm_init(void)
{
    if (initialized) return;

    // 50 Hz outputs start at the neutral 1.5 ms pulse.
    for (uint8_t ch = 0; ch < 8; ++ch) {
        pwm_set(ch, 0.0f);
    }
    pwm_set_ibt2_motor(0.0f);
    pwm_set_ball_enable(0);

    for (uint8_t timer = 0; timer < 2; ++timer) {
        for (uint8_t channel = 0; channel < 4; ++channel) {
            if (HAL_TIM_PWM_Start(standard_pwm_timers[timer],
                                  timer_channels[channel]) != HAL_OK) {
                Error_Handler();
            }
        }
    }

    if (HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_1) != HAL_OK ||
        HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_2) != HAL_OK) {
        Error_Handler();
    }

    initialized = 1;
}

void pwm_set_raw(uint8_t ch, uint16_t output)
{
    if (ch >= 8) return;

    TIM_HandleTypeDef *timer = (ch < 4) ? &htim3 : &htim4;
    uint32_t channel = timer_channels[ch % 4];
    __HAL_TIM_SET_COMPARE(timer, channel, output);
}

void pwm_set(uint8_t ch, float output)
{
    if (ch >= 8) return;

    output = pwm_clamp(output, -1.0f, 1.0f);
    float pulse_us = 1500.0f + (output * 400.0f);
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD((ch < 4) ? &htim3 : &htim4);

    if (pulse_us < 0.0f) pulse_us = 0.0f;
    if (pulse_us > (float)arr) pulse_us = (float)arr;

    pwm_set_raw(ch, (uint16_t)lroundf(pulse_us));
}

uint16_t pwm_get_raw(uint8_t ch)
{
    if (ch >= 8) return 0;

    TIM_HandleTypeDef *timer = (ch < 4) ? &htim3 : &htim4;
    uint32_t channel = timer_channels[ch % 4];
    return (uint16_t)__HAL_TIM_GET_COMPARE(timer, channel);
}

void pwm_set_ibt2_motor(float speed)
{
    speed = pwm_clamp(speed, -1.0f, 1.0f);
    uint32_t period_counts = __HAL_TIM_GET_AUTORELOAD(&htim9) + 1U;
    uint16_t duty = (uint16_t)lroundf(fabsf(speed) * (float)period_counts);

    // Drive one BTS7960 input at a time to select direction.
    __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, (speed > 0.0f) ? duty : 0U);
    __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_2, (speed < 0.0f) ? duty : 0U);
}

void pwm_set_ball_enable(uint8_t enable)
{
    HAL_GPIO_WritePin(GPIOC, IBT2_R_EN_Pin | IBT2_L_EN_Pin,
                      enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void pwm_set_pump_enable(uint8_t enable)
{
    HAL_GPIO_WritePin(PUMP_EN_GPIO_Port, PUMP_EN_Pin,
                      enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
