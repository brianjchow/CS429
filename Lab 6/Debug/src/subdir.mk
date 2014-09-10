################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/cds.c \
../src/main.c \
../src/read_cds.c \
../src/simulate.c \
../src/utils.c 

OBJS += \
./src/cds.o \
./src/main.o \
./src/read_cds.o \
./src/simulate.o \
./src/utils.o 

C_DEPS += \
./src/cds.d \
./src/main.d \
./src/read_cds.d \
./src/simulate.d \
./src/utils.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


