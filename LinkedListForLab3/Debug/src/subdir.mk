################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/LinkedListForLab3\ (Fatass-PC's\ conflicted\ copy\ 2013-10-14).c \
../src/LinkedListForLab3.c 

OBJS += \
./src/LinkedListForLab3\ (Fatass-PC's\ conflicted\ copy\ 2013-10-14).o \
./src/LinkedListForLab3.o 

C_DEPS += \
./src/LinkedListForLab3\ (Fatass-PC's\ conflicted\ copy\ 2013-10-14).d \
./src/LinkedListForLab3.d 


# Each subdirectory must supply rules for building sources it contributes
src/LinkedListForLab3\ (Fatass-PC's\ conflicted\ copy\ 2013-10-14).o: ../src/LinkedListForLab3\ (Fatass-PC's\ conflicted\ copy\ 2013-10-14).c
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C Compiler'
	gcc -O0 -g -g -Wall -c -fmessage-length=0 -MMD -MP -MF"src/LinkedListForLab3 (Fatass-PC's conflicted copy 2013-10-14).d" -MT"src/LinkedListForLab3\ (Fatass-PC's\ conflicted\ copy\ 2013-10-14).d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C Compiler'
	gcc -O0 -g -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


