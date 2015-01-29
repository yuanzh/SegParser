################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../DependencyInstance.cpp \
../DependencyPipe.cpp \
../FeatureEncoder.cpp \
../FeatureExtractor.cpp \
../Options.cpp \
../Parameters.cpp \
../SegParser.cpp 

OBJS += \
./DependencyInstance.o \
./DependencyPipe.o \
./FeatureEncoder.o \
./FeatureExtractor.o \
./Options.o \
./Parameters.o \
./SegParser.o 

CPP_DEPS += \
./DependencyInstance.d \
./DependencyPipe.d \
./FeatureEncoder.d \
./FeatureExtractor.d \
./Options.d \
./Parameters.d \
./SegParser.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D__GXX_EXPERIMENTAL_CXX0X__ -D__cplusplus=201103L -O3 -Wall -c -fmessage-length=0 -std=c++0x -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


