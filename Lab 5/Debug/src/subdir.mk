################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/asm8.c \
../src/objmem.c \
../src/opcodes.c \
../src/symtab.c \
../src/token.c 

OBJS += \
./src/asm8.o \
./src/objmem.o \
./src/opcodes.o \
./src/symtab.o \
./src/token.o 

C_DEPS += \
./src/asm8.d \
./src/objmem.d \
./src/opcodes.d \
./src/symtab.d \
./src/token.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


