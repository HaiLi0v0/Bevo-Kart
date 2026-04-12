################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
Scratch.o: ../Scratch.s $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs2040/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/bin/tiarmclang.exe" -c -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"C:/BevoKart/ScratchAsm" -I"C:/BevoKart/ScratchAsm/Debug" -I"C:/ti/mspm0_sdk_2_09_00_01/source/third_party/CMSIS/Core/Include" -I"C:/ti/mspm0_sdk_2_09_00_01/source" -gdwarf-3 -I"C:/BevoKart/ScratchAsm/Debug/syscfg" $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


