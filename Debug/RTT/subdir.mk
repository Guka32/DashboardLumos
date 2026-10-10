################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../RTT/SEGGER_RTT.c \
../RTT/SEGGER_RTT_printf.c \
../RTT/SEGGER_SYSVIEW.c \
../RTT/SEGGER_SYSVIEW_Config_FreeRTOS.c \
../RTT/SEGGER_SYSVIEW_FreeRTOS.c 

S_UPPER_SRCS += \
../RTT/SEGGER_RTT_ASM_ARMv7M.S 

OBJS += \
./RTT/SEGGER_RTT.o \
./RTT/SEGGER_RTT_ASM_ARMv7M.o \
./RTT/SEGGER_RTT_printf.o \
./RTT/SEGGER_SYSVIEW.o \
./RTT/SEGGER_SYSVIEW_Config_FreeRTOS.o \
./RTT/SEGGER_SYSVIEW_FreeRTOS.o 

S_UPPER_DEPS += \
./RTT/SEGGER_RTT_ASM_ARMv7M.d 

C_DEPS += \
./RTT/SEGGER_RTT.d \
./RTT/SEGGER_RTT_printf.d \
./RTT/SEGGER_SYSVIEW.d \
./RTT/SEGGER_SYSVIEW_Config_FreeRTOS.d \
./RTT/SEGGER_SYSVIEW_FreeRTOS.d 


# Each subdirectory must supply rules for building sources it contributes
RTT/%.o RTT/%.su RTT/%.cyclo: ../RTT/%.c RTT/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I../RTT -I../Core/Inc -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
RTT/%.o: ../RTT/%.S RTT/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m4 -g3 -DDEBUG -c -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"

clean: clean-RTT

clean-RTT:
	-$(RM) ./RTT/SEGGER_RTT.cyclo ./RTT/SEGGER_RTT.d ./RTT/SEGGER_RTT.o ./RTT/SEGGER_RTT.su ./RTT/SEGGER_RTT_ASM_ARMv7M.d ./RTT/SEGGER_RTT_ASM_ARMv7M.o ./RTT/SEGGER_RTT_printf.cyclo ./RTT/SEGGER_RTT_printf.d ./RTT/SEGGER_RTT_printf.o ./RTT/SEGGER_RTT_printf.su ./RTT/SEGGER_SYSVIEW.cyclo ./RTT/SEGGER_SYSVIEW.d ./RTT/SEGGER_SYSVIEW.o ./RTT/SEGGER_SYSVIEW.su ./RTT/SEGGER_SYSVIEW_Config_FreeRTOS.cyclo ./RTT/SEGGER_SYSVIEW_Config_FreeRTOS.d ./RTT/SEGGER_SYSVIEW_Config_FreeRTOS.o ./RTT/SEGGER_SYSVIEW_Config_FreeRTOS.su ./RTT/SEGGER_SYSVIEW_FreeRTOS.cyclo ./RTT/SEGGER_SYSVIEW_FreeRTOS.d ./RTT/SEGGER_SYSVIEW_FreeRTOS.o ./RTT/SEGGER_SYSVIEW_FreeRTOS.su

.PHONY: clean-RTT

