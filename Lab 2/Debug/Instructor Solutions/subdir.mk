################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Instructor\ Solutions/5bit.c \
../Instructor\ Solutions/5bit2.c \
../Instructor\ Solutions/5bitc1.c 

OBJS += \
./Instructor\ Solutions/5bit.o \
./Instructor\ Solutions/5bit2.o \
./Instructor\ Solutions/5bitc1.o 

C_DEPS += \
./Instructor\ Solutions/5bit.d \
./Instructor\ Solutions/5bit2.d \
./Instructor\ Solutions/5bitc1.d 


# Each subdirectory must supply rules for building sources it contributes
Instructor\ Solutions/5bit.o: ../Instructor\ Solutions/5bit.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C Compiler'
	gcc -O0 -g -g -Wall -c -fmessage-length=0 -MMD -MP -MF"Instructor Solutions/5bit.d" -MT"Instructor\ Solutions/5bit.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Instructor\ Solutions/5bit2.o: ../Instructor\ Solutions/5bit2.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C Compiler'
	gcc -O0 -g -g -Wall -c -fmessage-length=0 -MMD -MP -MF"Instructor Solutions/5bit2.d" -MT"Instructor\ Solutions/5bit2.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Instructor\ Solutions/5bitc1.o: ../Instructor\ Solutions/5bitc1.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C Compiler'
	gcc -O0 -g -g -Wall -c -fmessage-length=0 -MMD -MP -MF"Instructor Solutions/5bitc1.d" -MT"Instructor\ Solutions/5bitc1.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


