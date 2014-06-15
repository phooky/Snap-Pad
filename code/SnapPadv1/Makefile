
TARGET	= snap_pad
MCU	= msp430f5508

JLH_SOURCES = \
	USB/usbEventHandling.c \
	USB/USB_config/descriptors.c \
	USB/USB_config/UsbIsr.c \
	USB/USB_API/USB_HID_API/UsbHidReq.c \
	USB/USB_API/USB_HID_API/UsbHid.c \
	USB/USB_API/USB_MSC_API/UsbMscStateMachine.c \
	USB/USB_API/USB_MSC_API/UsbMscReq.c \
	USB/USB_API/USB_MSC_API/UsbMscScsi.c \
	USB/USB_API/USB_PHDC_API/UsbPHDC.c \
	USB/USB_API/USB_Common/usb.c \
	USB/USB_API/USB_Common/dma.c \
	USB/USB_API/USB_CDC_API/UsbCdc.c \
	USB/usbConstructs.c \
	F5xx_F6xx_Core_Lib/HAL_UCS.h \
	F5xx_F6xx_Core_Lib/HAL_TLV.c \
	F5xx_F6xx_Core_Lib/HAL_PMM.c \
	F5xx_F6xx_Core_Lib/HAL_UCS.c

TI_SOURCES = \
	USB_API/USB_HID_API/UsbHidReq.c \
	USB_API/USB_HID_API/UsbHid.c \
	USB_API/USB_MSC_API/UsbMscStateMachine.c \
	USB_API/USB_MSC_API/UsbMscReq.c \
	USB_API/USB_MSC_API/UsbMscScsi.c \
	USB_API/USB_PHDC_API/UsbPHDC.c \
	USB_API/USB_Common/usbdma.c \
	USB_API/USB_Common/usb.c \
	USB_API/USB_CDC_API/UsbCdc.c \
	USB_app/usbEventHandling.c \
	USB_app/usbConstructs.c \
	USB_config/descriptors.c \
	USB_config/UsbIsr.c \


SOURCES	= \
	base64.c \
	buffers.c \
	hwrng.c \
	leds.c \
	main.c \
	nand.c \
	onetimepad.c \
	print.c \
	timer.c \
	uarts.c \
	ecc.c \
	hal.c \
	$(JLH_SOURCES)

INCLUDES = -Idriverlib/MSP430F5xx_6xx -IF5xx_F6xx_Core_Lib -IUSB -IUSB/USB_config -I.

CFLAGS   = -mmcu=$(MCU) -std=c99 -Os -Wall -Wunused $(INCLUDES)   
ASFLAGS  = -mmcu=$(MCU) -x assembler-with-cpp -Wa,-gstabs
LDFLAGS  = -mmcu=$(MCU) -std=c99 -Os -Wl,-Map=$(TARGET).map

CC       = msp430-gcc
LD       = msp430-ld
AR       = msp430-ar
AS       = msp430-gcc
GASP     = msp430-gasp
NM       = msp430-nm
OBJCOPY  = msp430-objcopy
RANLIB   = msp430-ranlib
STRIP    = msp430-strip
SIZE     = msp430-size
READELF  = msp430-readelf
CP       = cp -p
RM       = rm -f
MV       = mv

OBJ 	= $(SOURCES:.c=.o)

all: $(TARGET).elf $(TARGET).hex

$(TARGET).elf: $(OBJ)
	echo "Linking $@"
	$(CC) $(OBJ) $(LDFLAGS) $(LIBS) -o $@
	echo ">>>> Size of Firmware <<<<"
	$(SIZE) $(TARGET).elf

%.hex: %.elf
	$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

# rule for making assembler source listing, to see the code
%.lst: %.c
	$(CC) -c $(ASFLAGS) -Wa,-anlhd $< > $@

.SILENT:
.PHONY:	clean
clean:
	-$(RM) $(OBJ)
	-$(RM) $(TARGET).*
	-$(RM) $(SOURCES:.c=.lst)
