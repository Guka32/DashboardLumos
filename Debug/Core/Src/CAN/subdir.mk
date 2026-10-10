################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/CAN/can.c \
../Core/Src/CAN/can_bus.c \
../Core/Src/CAN/can_bus_parser.c \
../Core/Src/CAN/can_bus_task.c \
../Core/Src/CAN/can_bus_tx_tasks.c 

OBJS += \
./Core/Src/CAN/can.o \
./Core/Src/CAN/can_bus.o \
./Core/Src/CAN/can_bus_parser.o \
./Core/Src/CAN/can_bus_task.o \
./Core/Src/CAN/can_bus_tx_tasks.o 

C_DEPS += \
./Core/Src/CAN/can.d \
./Core/Src/CAN/can_bus.d \
./Core/Src/CAN/can_bus_parser.d \
./Core/Src/CAN/can_bus_task.d \
./Core/Src/CAN/can_bus_tx_tasks.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/CAN/%.o Core/Src/CAN/%.su Core/Src/CAN/%.cyclo: ../Core/Src/CAN/%.c Core/Src/CAN/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I../RTT -I../Core/Inc -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-CAN

clean-Core-2f-Src-2f-CAN:
	-$(RM) ./Core/Src/CAN/can.cyclo ./Core/Src/CAN/can.d ./Core/Src/CAN/can.o ./Core/Src/CAN/can.su ./Core/Src/CAN/can_bus.cyclo ./Core/Src/CAN/can_bus.d ./Core/Src/CAN/can_bus.o ./Core/Src/CAN/can_bus.su ./Core/Src/CAN/can_bus_parser.cyclo ./Core/Src/CAN/can_bus_parser.d ./Core/Src/CAN/can_bus_parser.o ./Core/Src/CAN/can_bus_parser.su ./Core/Src/CAN/can_bus_task.cyclo ./Core/Src/CAN/can_bus_task.d ./Core/Src/CAN/can_bus_task.o ./Core/Src/CAN/can_bus_task.su ./Core/Src/CAN/can_bus_tx_tasks.cyclo ./Core/Src/CAN/can_bus_tx_tasks.d ./Core/Src/CAN/can_bus_tx_tasks.o ./Core/Src/CAN/can_bus_tx_tasks.su

.PHONY: clean-Core-2f-Src-2f-CAN

