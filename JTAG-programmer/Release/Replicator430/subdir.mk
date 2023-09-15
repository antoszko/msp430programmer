################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (11.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Replicator430/Devices430.c \
../Replicator430/FlashErase.c \
../Replicator430/FlashWrite.c \
../Replicator430/JTAGfunc430.c \
../Replicator430/LowLevelFunc430.c 

OBJS += \
./Replicator430/Devices430.o \
./Replicator430/FlashErase.o \
./Replicator430/FlashWrite.o \
./Replicator430/JTAGfunc430.o \
./Replicator430/LowLevelFunc430.o 

C_DEPS += \
./Replicator430/Devices430.d \
./Replicator430/FlashErase.d \
./Replicator430/FlashWrite.d \
./Replicator430/JTAGfunc430.d \
./Replicator430/LowLevelFunc430.d 


# Each subdirectory must supply rules for building sources it contributes
Replicator430/%.o Replicator430/%.su Replicator430/%.cyclo: ../Replicator430/%.c Replicator430/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32L432xx -DUSE_FULL_LL_DRIVER -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I"/hdd/STM32CubeIDE/workspace_1.11.2/JTAG-programmer/Replicator430" -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Replicator430

clean-Replicator430:
	-$(RM) ./Replicator430/Devices430.cyclo ./Replicator430/Devices430.d ./Replicator430/Devices430.o ./Replicator430/Devices430.su ./Replicator430/FlashErase.cyclo ./Replicator430/FlashErase.d ./Replicator430/FlashErase.o ./Replicator430/FlashErase.su ./Replicator430/FlashWrite.cyclo ./Replicator430/FlashWrite.d ./Replicator430/FlashWrite.o ./Replicator430/FlashWrite.su ./Replicator430/JTAGfunc430.cyclo ./Replicator430/JTAGfunc430.d ./Replicator430/JTAGfunc430.o ./Replicator430/JTAGfunc430.su ./Replicator430/LowLevelFunc430.cyclo ./Replicator430/LowLevelFunc430.d ./Replicator430/LowLevelFunc430.o ./Replicator430/LowLevelFunc430.su

.PHONY: clean-Replicator430

