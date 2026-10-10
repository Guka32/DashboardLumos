################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../RTT/Syscalls/SEGGER_RTT_Syscalls_GCC.c \
../RTT/Syscalls/SEGGER_RTT_Syscalls_IAR.c \
../RTT/Syscalls/SEGGER_RTT_Syscalls_KEIL.c \
../RTT/Syscalls/SEGGER_RTT_Syscalls_SES.c 

OBJS += \
./RTT/Syscalls/SEGGER_RTT_Syscalls_GCC.o \
./RTT/Syscalls/SEGGER_RTT_Syscalls_IAR.o \
./RTT/Syscalls/SEGGER_RTT_Syscalls_KEIL.o \
./RTT/Syscalls/SEGGER_RTT_Syscalls_SES.o 

C_DEPS += \
./RTT/Syscalls/SEGGER_RTT_Syscalls_GCC.d \
./RTT/Syscalls/SEGGER_RTT_Syscalls_IAR.d \
./RTT/Syscalls/SEGGER_RTT_Syscalls_KEIL.d \
./RTT/Syscalls/SEGGER_RTT_Syscalls_SES.d 


# Each subdirectory must supply rules for building sources it contributes
RTT/Syscalls/%.o RTT/Syscalls/%.su RTT/Syscalls/%.cyclo: ../RTT/Syscalls/%.c RTT/Syscalls/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I../RTT -I../Core/Inc -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-RTT-2f-Syscalls

clean-RTT-2f-Syscalls:
	-$(RM) ./RTT/Syscalls/SEGGER_RTT_Syscalls_GCC.cyclo ./RTT/Syscalls/SEGGER_RTT_Syscalls_GCC.d ./RTT/Syscalls/SEGGER_RTT_Syscalls_GCC.o ./RTT/Syscalls/SEGGER_RTT_Syscalls_GCC.su ./RTT/Syscalls/SEGGER_RTT_Syscalls_IAR.cyclo ./RTT/Syscalls/SEGGER_RTT_Syscalls_IAR.d ./RTT/Syscalls/SEGGER_RTT_Syscalls_IAR.o ./RTT/Syscalls/SEGGER_RTT_Syscalls_IAR.su ./RTT/Syscalls/SEGGER_RTT_Syscalls_KEIL.cyclo ./RTT/Syscalls/SEGGER_RTT_Syscalls_KEIL.d ./RTT/Syscalls/SEGGER_RTT_Syscalls_KEIL.o ./RTT/Syscalls/SEGGER_RTT_Syscalls_KEIL.su ./RTT/Syscalls/SEGGER_RTT_Syscalls_SES.cyclo ./RTT/Syscalls/SEGGER_RTT_Syscalls_SES.d ./RTT/Syscalls/SEGGER_RTT_Syscalls_SES.o ./RTT/Syscalls/SEGGER_RTT_Syscalls_SES.su

.PHONY: clean-RTT-2f-Syscalls

