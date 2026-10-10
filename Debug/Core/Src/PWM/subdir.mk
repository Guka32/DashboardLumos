################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/PWM/pwm_out.c 

OBJS += \
./Core/Src/PWM/pwm_out.o 

C_DEPS += \
./Core/Src/PWM/pwm_out.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/PWM/%.o Core/Src/PWM/%.su Core/Src/PWM/%.cyclo: ../Core/Src/PWM/%.c Core/Src/PWM/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I../RTT -I../Core/Inc -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-PWM

clean-Core-2f-Src-2f-PWM:
	-$(RM) ./Core/Src/PWM/pwm_out.cyclo ./Core/Src/PWM/pwm_out.d ./Core/Src/PWM/pwm_out.o ./Core/Src/PWM/pwm_out.su

.PHONY: clean-Core-2f-Src-2f-PWM

