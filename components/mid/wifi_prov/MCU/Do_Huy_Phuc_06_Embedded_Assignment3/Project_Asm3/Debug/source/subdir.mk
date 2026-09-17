################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/Project_Asm3.c 

C_DEPS += \
./source/Project_Asm3.d 

OBJS += \
./source/Project_Asm3.o 


# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.c source/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DCPU_MKL46Z256VLL4_cm0plus -DCPU_MKL46Z256VLL4 -DFSL_RTOS_BM -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\Users\phucc\Documents\MCUXpressoIDE_11.8.0_1165\workspace\Project_Asm3\board" -I"C:\Users\phucc\Documents\MCUXpressoIDE_11.8.0_1165\workspace\Project_Asm3\source" -I"C:\Users\phucc\Documents\MCUXpressoIDE_11.8.0_1165\workspace\Project_Asm3" -I"C:\Users\phucc\Documents\MCUXpressoIDE_11.8.0_1165\workspace\Project_Asm3\drivers" -I"C:\Users\phucc\Documents\MCUXpressoIDE_11.8.0_1165\workspace\Project_Asm3\CMSIS" -I"C:\Users\phucc\Documents\MCUXpressoIDE_11.8.0_1165\workspace\Project_Asm3\utilities" -I"C:\Users\phucc\Documents\MCUXpressoIDE_11.8.0_1165\workspace\Project_Asm3\startup" -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m0plus -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-source

clean-source:
	-$(RM) ./source/Project_Asm3.d ./source/Project_Asm3.o

.PHONY: clean-source

