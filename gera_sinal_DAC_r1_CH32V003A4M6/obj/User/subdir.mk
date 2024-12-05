################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/DAC_MCP4725.c \
../User/buffer_fifo.c \
../User/ch32v00x_it.c \
../User/main.c \
../User/system_ch32v00x.c \
../User/table_sin.c \
../User/usart_com.c 

OBJS += \
./User/DAC_MCP4725.o \
./User/buffer_fifo.o \
./User/ch32v00x_it.o \
./User/main.o \
./User/system_ch32v00x.o \
./User/table_sin.o \
./User/usart_com.o 

C_DEPS += \
./User/DAC_MCP4725.d \
./User/buffer_fifo.d \
./User/ch32v00x_it.d \
./User/main.d \
./User/system_ch32v00x.d \
./User/table_sin.d \
./User/usart_com.d 


# Each subdirectory must supply rules for building sources it contributes
User/%.o: ../User/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GNU RISC-V Cross C Compiler'
	riscv-none-embed-gcc -march=rv32ec -mabi=ilp32e -msmall-data-limit=0 -msave-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized  -g -I"D:\RISC-V\mrs_workspace\gera_sinal_DAC_r1_CH32V003A4M6\Debug" -I"D:\RISC-V\mrs_workspace\gera_sinal_DAC_r1_CH32V003A4M6\Core" -I"D:\RISC-V\mrs_workspace\gera_sinal_DAC_r1_CH32V003A4M6\User" -I"D:\RISC-V\mrs_workspace\gera_sinal_DAC_r1_CH32V003A4M6\Peripheral\inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


