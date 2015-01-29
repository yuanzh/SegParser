################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../decoder/ClassifierDecoder.cpp \
../decoder/DependencyDecoder.cpp \
../decoder/DevelopmentThread.cpp \
../decoder/HillClimbingDecoder.cpp \
../decoder/SampleRankDecoder.cpp 

OBJS += \
./decoder/ClassifierDecoder.o \
./decoder/DependencyDecoder.o \
./decoder/DevelopmentThread.o \
./decoder/HillClimbingDecoder.o \
./decoder/SampleRankDecoder.o 

CPP_DEPS += \
./decoder/ClassifierDecoder.d \
./decoder/DependencyDecoder.d \
./decoder/DevelopmentThread.d \
./decoder/HillClimbingDecoder.d \
./decoder/SampleRankDecoder.d 


# Each subdirectory must supply rules for building sources it contributes
decoder/%.o: ../decoder/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D__GXX_EXPERIMENTAL_CXX0X__ -D__cplusplus=201103L -O3 -Wall -c -fmessage-length=0 -std=c++0x -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


