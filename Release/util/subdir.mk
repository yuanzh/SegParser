################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../util/Alphabet.cpp \
../util/Constant.cpp \
../util/FeatureAlphabet.cpp \
../util/FeatureVector.cpp \
../util/Logarithm.cpp \
../util/SerializationUtils.cpp \
../util/StringUtils.cpp 

OBJS += \
./util/Alphabet.o \
./util/Constant.o \
./util/FeatureAlphabet.o \
./util/FeatureVector.o \
./util/Logarithm.o \
./util/SerializationUtils.o \
./util/StringUtils.o 

CPP_DEPS += \
./util/Alphabet.d \
./util/Constant.d \
./util/FeatureAlphabet.d \
./util/FeatureVector.d \
./util/Logarithm.d \
./util/SerializationUtils.d \
./util/StringUtils.d 


# Each subdirectory must supply rules for building sources it contributes
util/%.o: ../util/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D__GXX_EXPERIMENTAL_CXX0X__ -I/scratch/yuanzh/workspace/Code/boost_1_55_0/ -O3 -Wall -c -fmessage-length=0 -std=c++0x -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


