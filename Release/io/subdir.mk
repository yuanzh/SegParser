################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../io/DependencyReader.cpp \
../io/DependencyWriter.cpp 

OBJS += \
./io/DependencyReader.o \
./io/DependencyWriter.o 

CPP_DEPS += \
./io/DependencyReader.d \
./io/DependencyWriter.d 


# Each subdirectory must supply rules for building sources it contributes
io/%.o: ../io/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D__GXX_EXPERIMENTAL_CXX0X__ -D__cplusplus=201103L -O3 -Wall -c -fmessage-length=0 -std=c++0x -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


