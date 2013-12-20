################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
hal.obj: ../hal.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"/home/phooky/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmspx --abi=eabi -O3 --include_path="/home/phooky/ti/ccsv5/ccs_base/msp430/include" --include_path="/home/phooky/ti/ccsv5/tools/compiler/msp430_4.2.1/include" --include_path="/home/phooky/Repos/Snap-Pad/code/SnapPadv0/driverlib/MSP430F5xx_6xx" --include_path="/home/phooky/Repos/Snap-Pad/code/SnapPadv0" --include_path="/home/phooky/Repos/Snap-Pad/code/SnapPadv0/USB_config" -g --define=__MSP430F5508__ --diag_warning=225 --display_error_number --diag_wrap=off --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="hal.pp" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '

main.obj: ../main.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"/home/phooky/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmspx --abi=eabi -O3 --include_path="/home/phooky/ti/ccsv5/ccs_base/msp430/include" --include_path="/home/phooky/ti/ccsv5/tools/compiler/msp430_4.2.1/include" --include_path="/home/phooky/Repos/Snap-Pad/code/SnapPadv0/driverlib/MSP430F5xx_6xx" --include_path="/home/phooky/Repos/Snap-Pad/code/SnapPadv0" --include_path="/home/phooky/Repos/Snap-Pad/code/SnapPadv0/USB_config" -g --define=__MSP430F5508__ --diag_warning=225 --display_error_number --diag_wrap=off --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="main.pp" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '

nand.obj: ../nand.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"/home/phooky/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmspx --abi=eabi -O3 --include_path="/home/phooky/ti/ccsv5/ccs_base/msp430/include" --include_path="/home/phooky/ti/ccsv5/tools/compiler/msp430_4.2.1/include" --include_path="/home/phooky/Repos/Snap-Pad/code/SnapPadv0/driverlib/MSP430F5xx_6xx" --include_path="/home/phooky/Repos/Snap-Pad/code/SnapPadv0" --include_path="/home/phooky/Repos/Snap-Pad/code/SnapPadv0/USB_config" -g --define=__MSP430F5508__ --diag_warning=225 --display_error_number --diag_wrap=off --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="nand.pp" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '


