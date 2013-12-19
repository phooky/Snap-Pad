/* --COPYRIGHT--,BSD
 * Copyright (c) 2013, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
/** @file usbEventHandling.c
 *  @brief Contains required event Handler fucntions
 */
/* 
 * ======== usbEventHandling.c ========
 * Event-handling placeholder functions.
 * All functios are called in interrupt context.
 */
 
//
//! \cond
//

#include "USB_API/USB_Common/device.h"
#include "USB_API/USB_Common/types.h"
#include "USB_API/USB_Common/defMSP430USB.h"
#include "USB_config/descriptors.h"
#include "USB_API/USB_Common/usb.h"

#ifdef _CDC_
#include "USB_API/USB_CDC_API/UsbCdc.h"
#endif

#ifdef _HID_
#include "USB_API/USB_HID_API/UsbHid.h"
#endif

#ifdef _MSC_
#include "USB_API/USB_MSC_API/UsbMsc.h"
#endif

#ifdef _PHDC_
#include "USB_API/USB_PHDC_API/UsbPHDC.h"
#endif

//
//! \endcond
//

//******************************************************************************
//
//! USB PLL has Failed
//!
//! This event signals that the output of the USB PLL has failed. This event may
//! have occurred because XT2, the source of the PLL’s reference clock, has
//! failed or is unreliable. If this event occurs, the USB connection will 
//! likely be lost. It is best to handle it by calling USB_disconnect() and
//! attempting a re-connection.
//! 
//! Since this event is associated with a change in state, it's a good
//! practice to return TRUE so the main loop can adapt.
//
//******************************************************************************
BYTE USB_handleClockEvent ()
{
    //Something happened to the PLL.  This might also show up in the system as an oscillator fault on XT2.
    //The USB connection is probably lost.  Software should ensure any faults on XT2 are resolved,
    //then can attempt to call USB_enable()/USB_connect() again.
    USB_disconnect();
    USB_disable();

    return ( TRUE) ;                    //Since this event is associated with a change in state, it's a good practice to return TRUE
                                        //so the main loop can adapt.
}

//*****************************************************************************
//
//! Valid Voltage Applied to VBUS
//!
//! If this function gets executed, it indicates that a valid voltage has been
//! applied to the VBUS pin; that is, the voltage on VBUS has transitioned from
//! low to high.
//! 
//! This usually means the device has been attached to an active USB host. It is
//! recommended to attempt a USB connection from this handler, as described in 
//! Sec. 6.3 of \e "Programmer’s Guide: MSP430 USB API Stack for CDC/PHDC/HID/MSC."
//! events.
//! 
//! Returns TRUE to wake the main loop (if LPM was entered), so that it can
//! take into account the change in state.
//
//*****************************************************************************
BYTE USB_handleVbusOnEvent ()
{
    //The standard user experience when a USB device gets physically attached to a host is for the host to
    //enumerate the device.  Typically this happens as follows:
    //1) the device senses 5V VBUS from the host, which tells it a host is present; (usually; but could also be a powered hub w/o a
    //host!  See state ST_NOENUM_SUSPENDED.)
    //2) the device asserts the PUR signal, which tells the host the devicde is present;
    //3) the host issues a number of USB device requests, including asking for the device's USB descriptors;
    //4) the host decides if it has the appropriate driver for what it sees in the descriptors; and if so, loads it.  Enumeration is
    //now complete.
    //So -- USB_handleVbusOnEvent occurs if a VBUS-on event has been detected.  We respond by doing the following.
    //However, keep in mind that USB_enable() might take a few milliseconds while the crystal starts up, and that most events handle
    //in
    //the context of the USB interrupt handler.  If this interrupt latency is unacceptable, it might be better to set a flag for
    //main() to handle it.
    if (USB_enable() == kUSB_succeed){  //Start the module;
        USB_reset();                    //Reset the internal API
        USB_connect();                  //Assert PUR, to tell the host we're here
    }                                   //Enumeration will now take place in the background
    return (TRUE);                      //Meanwhile, return TRUE to wake the main loop (if LPM was entered), so
}                                       //that it can take into account the change in state

//*****************************************************************************
//
//! Valid Voltage Removed from VBUS
//!
//! This event indicates that a valid voltage has just been removed from the 
//! VBUS pin. That is, the voltage on VBUS has transitioned from high to low.
//! 
//! This generally means the device has been removed from an active USB host. It
//! might also mean the device is still physically attached to the host, but the
//! host went into a standby mode; or it was attached to a powered hub but the
//! host upstream from that hub became inactive. The API automatically responds
//! to a VBUS-off event by powering down the USB module and PLL, which is the
//! equivalent of calling USB_disable(). It then calls this handling function,
//! if enabled.
//! 
//! Since this event is associated with a change in state, it's a good
//! practice to return TRUE so the main loop can adapt.
//
//*****************************************************************************
BYTE USB_handleVbusOffEvent ()
{
    //Typically there's no need to place code here -- the main loop simply shifts to ST_USB_DISCONNECTED.

    return (TRUE);                      //Since this event is associated with a change in state, it's a good practice to return TRUE
                                        //so the main loop can adapt.
}

//*****************************************************************************
//
//! USB Host Issued a Reset
//!
//! This event indicates that the USB host has issued a reset of this USB
//! device. The API handles this automatically, and no action is required by the
//! application to maintain USB operation. After handling the reset, the API
//! calls this handling function, if enabled. In most cases there is no
//! significant reason for the application to respond to bus resets.
//
//*****************************************************************************
BYTE USB_handleResetEvent ()
{
    return (TRUE);
}

//*****************************************************************************
//
//! USB Host Suspends USB Device
//!
//! This event indicates that the USB host has chosen to suspend this USB device
//! after a period of active operation. It’s important that a bus-powered,
//! suspended USB device limit its consumption of power from VBUS during this
//! time. The API automatically shuts down USB-related circuitry inside the
//! MSP430’s USB module. However, the application may need to shut down other
//! circuitry drawing from VBUS. This handler is a good place to do this.
//! 
//! See Sec.11.1.3 of \e "Programmer’s Guide:
//! MSP430 USB API Stack for CDC/PHDC/HID/MSC." for a complete discussion
//! about handling suspend.
//! 
//! Returns TRUE so that the main loop can adapt.
//
//*****************************************************************************
BYTE USB_handleSuspendEvent ()
{
    //If this device draws power from the host over VBUS, then this event is the signal for software to ensure that
    //no more than 2.5mA is drawn over VBUS.  Code can be placed here to do this, or it can be placed in the main loop
    //under ST_ENUM_SUSPENDED (but make sure this handler returns TRUE to wake the main loop, if LPM0 was entered).

    return (TRUE);  //Return TRUE so that the main loop can adapt.
}

//*****************************************************************************
//
//! USB Host has Resumed this USB Device
//!
//! This event indicates that the USB host has resumed this USB device from
//! suspend mode. If the device is bus-powered, it is no longer restricted in
//! the amount of power it can draw from VBUS. The API automatically re-enables
//! any circuitry in the MSP430’s USB module that was disabled during suspend.
//! The application can use this handler to re-enable other circuitry as well.
//! 
//! Since this event is associated with a change in state, it's a good
//! practice to return TRUE so the main loop can adapt.
//
//*****************************************************************************
BYTE USB_handleResumeEvent ()
{
    //If functionality was shut off during USB_handleSuspendEvent(), it can be re-enabled here.

    return (TRUE);  //Since this event is associated with a change in state, it's a good practice to return TRUE so the main loop
                    //can adapt.
}

//*****************************************************************************
//
//! Device has Become Enumerated
//! 
//! This event indicates that the device has just become enumerated. This
//! corresponds with a state change to ST_ENUM_ACTIVE.
//! 
//! Since this event is associated with a change in state, it's a good
//! practice to return TRUE so the main loop can adapt.
//
//*****************************************************************************
BYTE USB_handleEnumCompleteEvent ()
{
    //Typically there's no need to place code here -- the main loop shifts to ST_ENUM_ACTIVE.

    return (TRUE);  //Since this event is associated with a change in state, it's a good practice to return TRUE so the main loop
                    //can adapt.
}

#ifdef USE_TIMER_FOR_RESUME
//*****************************************************************************
//
//! USB_resume requires a "wait" for XT2 crystal stabilization
//! 
//! When this function gets executed, it indicates that a USB_resume is in 
//! progress and the USB stack requires the application to use a timer to wait 
//! until the XT2 crystal has stabilized. See crystal specific datasheet for 
//! delay times. When the crystal has stabilized the application needs to call 
//! the function USB_enable_PLL() to allow resume to continue.
//
//*****************************************************************************
void USB_handleCrystalStartedEvent(void)
{

}
//*****************************************************************************
//
//! USB_resume requires a "wait" for USB PLL stabilization
//! 
//! When this function gets executed, it indicates that a USB_resume is in 
//! progress and the USB stack requires the application to use a timer to wait 
//! until the USB PLL has stabilized. See crystal specific datasheet for 
//! delay times. When the PLL has stabilized the application needs to call 
//! the function USB_enable_final() to allow resume to continue.
//
//*****************************************************************************
void USB_handlePLLStartedEvent(void)
{

}
#endif

//*****************************************************************************
//
//! Indicates Data has been Received for CDC Interface
//!
//! \param intfNum is which HID interface is being used.
//! 
//! This event indicates that data has been received for CDC interface intfNum
//! with no receive operation underway. Effectively, the API doesn’t know what
//! to do with this data and is asking for instructions. The application can
//! respond by either initiating a receive operation or rejecting the data.
//! Until one of these is performed, USB data reception cannot continue; any
//! packets received from the USB host will be NAK’ed.
//! 
//! Therefore, this event should be handled quickly. A receive operation cannot
//! be started directly out of this event, since USBCDC_receiveData() cannot be
//! called from the event handlers. However, the handler can set a flag for
//! main() to begin the receive operation. After this function exits, a call to
//! USBCDC_intfStatus() for this CDC interface will return kUSBDataWaiting.
//! 
//! If the application is written so that a receive operation is always begun
//! prior to data arriving from the host, this event will never occur. The
//! software designer generally has a choice of whether to use this event as
//! part of code flow (initiating receive operations after data is received), or
//! to always keep a receive operation open in case data arrives. (See Sec. 11
//! of \e "Programmer’s Guide: MSP430 USB API Stack for CDC/PHDC/HID/MSC" for
//! more discussion.)
//! 
//! Return TRUE to wake up after data was received.
//
//*****************************************************************************
#ifdef _CDC_

BYTE USBCDC_handleDataReceived (BYTE intfNum)
{
    //TO DO: You can place your code here

    return (FALSE); //return TRUE to wake up after data was received
}

//*****************************************************************************
//
//! Send Operation on CDC Interface has Completed
//!
//! \param intfNum is which HID interface is being used.
//! 
//! 
//! This event indicates that a send operation on CDC interface intfNum has just
//! been completed. 
//! 
//! In applications sending a series of data blocks, the designer may wish
//! to use this event to trigger another send operation. This cannot be done
//! directly out of this event, since USBCDC_sendData() cannot be called 
//! from the event handlers. However, the handler can set a flag for main()
//! to begin the operation.
//! 
//! Returns FALSE to go asleep after interrupt (in the case the CPU slept before
//! interrupt).
//
//*****************************************************************************
BYTE USBCDC_handleSendCompleted (BYTE intfNum)
{
    //TO DO: You can place your code here

    return (FALSE); //return FALSE to go asleep after interrupt (in the case the CPU slept before interrupt)
}

//*****************************************************************************
//
//! Receive Operation on CDC Interface has Completed
//!
//! \param intfNum is which HID interface is being used.
//! 
//! This event indicates that a receive operation on CDC interface intfNum has
//! just been completed, and the data is therefore available in the user buffer
//! assigned when the call was made to USBCDC_receiveData(). If this event
//! occurs, it means that the entire buffer is full, according to the size value
//! that was requested.
//! 
//! The designer may wish to use this event to trigger another receive
//! operation. This cannot be done directly out of this event, since
//! USBCDC_receiveData() cannot be called from the event handlers. However, the
//! handler can set a flag for main() to begin the operation.
//! 
//! Returns FALSE to go asleep after interrupt (in the case the CPU slept before
//! interrupt).
//
//*****************************************************************************
BYTE USBCDC_handleReceiveCompleted (BYTE intfNum)
{
    //TO DO: You can place your code here

    return (FALSE); //return FALSE to go asleep after interrupt (in the case the CPU slept before interrupt)
}

//*****************************************************************************
//
//! New Line Coding Parameters have been Received from the Host
//!
//! \param intfNum is which CDC interface is being used.
//! \param lBaudrate had COMport baud rate values such as 9600, 19200 etc 
//!
//! This event indicates that a SetLineCoding request has been received from the
//! host and new values for baud rate are available.
//! 
//! The application can use the new baud rate value to re-configure the Uart
//! in the case of a usb to uart bridge application. See C7 Example for
//! details.
//! 
//! Returns FALSE to go asleep after interrupt (in the case the CPU slept before
//! interrupt).
//
//*****************************************************************************
BYTE USBCDC_handleSetLineCoding (BYTE intfNum, ULONG lBaudrate)
{
    //TO DO: You can place your code here

    return (FALSE); //return FALSE to go asleep after interrupt (in the case the CPU slept before interrupt)
}

//*****************************************************************************
//
//! New Line State has been Received from the Host
//!
//! \param intfNum is which CDC interface is being used.
//! \param lineState BIT0 is DTR_PRESENT(1) or DTR_NOT_PRESENT(0)
//!                  BIT1 is RTS_PRESETNT(1) or RTS_NOT_PRESENT(0)
//!
//! This event indicates that a SetControlLineState request has been received
//! from the host and new values for RTS are available.
//! 
//! The application can use the new RTS value to flow off the uart. See C7
//! Example for details.
//! 
//! Returns FALSE to go asleep after interrupt (in the case the CPU slept before
//! interrupt).
//
//*****************************************************************************
BYTE USBCDC_handleSetControlLineState (BYTE intfNum, BYTE lineState)
{
	return FALSE;
}

#endif //_CDC_

#ifdef _HID_

//*****************************************************************************
//
//! Data has been Received for HID Interface
//!
//! \param intfNum is which HID interface is being used.
//! 
//! This event applies to HID-Datapipe only, as opposed to HID-Traditional.
//! It indicates that data has been received for HID interface intfNum with no
//! receive operation underway. Effectively, the API doesn’t know what to do
//! with this data and is asking for instructions. The application can respond
//! by either initiating a receive operation or rejecting the data. Until one of
//! these is performed, USB data reception cannot continue; any packets received
//! from the USB host will be NAK’ed.
//! 
//! Therefore, this event should be handled quickly. A receive operation cannot
//! be started directly out of this event, since USBHID_receiveData() cannot be
//! called from the event handlers. However, the handler can set a flag for
//! main() to begin the receive operation. After this function exits, a call to
//! USBHID_intfStatus() for this HID interface will return kUSBDataWaiting.
//! 
//! If the application is written so that a receive operation is always begun
//! prior to data arriving from the host, this event will never occur. The
//! software designer generally has a choice of whether to use this event as
//! part of code flow (initiating receive operations after data is received), or
//! to always keep a receive operation open in case data arrives. (See Sec. 11
//! of \e "Programmer’s Guide: MSP430 USB API Stack for CDC/PHDC/HID/MSC" more
//! discussion.)
//! 
//! Returns FALSE to go asleep after interrupt (in the case the CPU slept before
//! interrupt).
//
//*****************************************************************************
BYTE USBHID_handleDataReceived (BYTE intfNum)
{
    //TO DO: You can place your code here

    return (FALSE); //return FALSE to go asleep after interrupt (in the case the CPU slept before interrupt)
}

//*****************************************************************************
//
//! Send Operation on Data Interface has been Completed
//!
//! \param intfNum is which HID interface is being used.
//! 
//! This event applies to HID-Datapipe only, as opposed to HID-Traditional. It
//! indicates that a send operation on data interface intfNum has just been
//! completed.
//! 
//! In applications sending a series of large blocks of data, the designer may
//! wish to use this event to trigger another send operation. This cannot be
//! done directly out of this event, since USBHID_sendData() cannot be called
//! from the event handlers. However, the handler can set a flag for main() to
//! begin the operation.
//! 
//! Returns FALSE to go asleep after interrupt (in the case the CPU slept before
//! interrupt).
//
//*****************************************************************************
BYTE USBHID_handleSendCompleted (BYTE intfNum)
{
    //TO DO: You can place your code here

    return (FALSE); //return FALSE to go asleep after interrupt (in the case the CPU slept before interrupt)
}

//*****************************************************************************
//
//! Receive Operation has been Completed
//!
//! \param intfNum is which HID interface is being used.
//! 
//! This event applies to HID-Datapipe only, as opposed to HID-Traditional. It
//! indicates that a receive operation on HID interface intfNum has just been
//! completed, and the data is therefore available in the user buffer assigned
//! when the call was made to USBHID_receiveData(). If this event occurs, it
//! means that the entire buffer is full, according to the size value that was
//! requested.
//! 
//! The designer may wish to use this event to trigger another receive
//! operation. This cannot be done directly out of this event, since
//! USBHID_receiveData() cannot be called from the event handlers. However, the
//! handler can set a flag for main() to begin the operation. 
//! 
//! Returns FALSE to go asleep after interrupt (in the case the CPU slept before
//! interrupt).
//
//*****************************************************************************
BYTE USBHID_handleReceiveCompleted (BYTE intfNum)
{
    //TO DO: You can place your code here

    return (FALSE); //return FALSE to go asleep after interrupt (in the case the CPU slept before interrupt)
}

//*****************************************************************************
//
//! Set_Protocol Request Received from the Host
//!
//! \param protocol indicates HID_BOOT_PROTOCOL or HID_REPORT_PROTOCOL 
//! \param intfNum is which HID interface is being used.
//! 
//! This event applies to HID Traditional only. It indicates that the host has
//! requested a change in the HID protocol – from Boot to Standard or Standard
//! to Boot. An application that maintains separate reports for boot and
//! standard protocols can switch to the appropriate report upon receiving this
//! request. The protocol field is either HID_BOOT_PROTOCOL or
//! HID_REPORT_PROTOCOL.
//! 
//! Returns FALSE to go asleep after interrupt (in the case the CPU slept before
//! interrupt).
//
//*****************************************************************************
BYTE USBHID_handleBootProtocol (BYTE protocol, BYTE intfnum)
{
    return (FALSE);
}

//*****************************************************************************
//
//! Set_Report request Received from the Host
//!
//! \param reportType is either USB_REQ_HID_INPUT, USB_REQ_HID_OUTPUT or
//!                             USB_REQ_HID_FEATURE
//! \param reportId is values defined by report descriptor
//! \param dataLength is length of report
//! \param intfNum is which HID interface is being used.
//! 
//! This event indicates that a Set_Report request was received from the
//! host. The application needs to supply a buffer to retrieve the report data
//! that will be sent as part of this request. This handler is passed the
//! reportType, reportId, the length of data phase as well as the interface
//! number.
//
//*****************************************************************************
BYTE *USBHID_handleEP0SetReport (BYTE reportType, BYTE reportId,
    WORD dataLength,
    BYTE intfnum)
{
	switch (reportType) {
		case USB_REQ_HID_INPUT:
			//Return pointer to input Report Buffer
			return (0);
		case USB_REQ_HID_OUTPUT:
			//Return pointer to output Report Buffer
			return (0);

		case USB_REQ_HID_FEATURE:
			//Return pointer to feature Report Buffer
			return (0);

		default:
			return (0);
	}
}

//*****************************************************************************
//
//! Data as Part of Set_Report Request was Received from the Host
//!
//! \param intfNum is which HID interface is being used.
//! 
//! This event indicates that data as part of Set_Report request was received
//! from the host. If the application supplied a buffer as part of
//! USBHID_handleEP0SetReport, then this buffer will contain the Set Report data.
//! 
//! Returns TRUE to wake up after data was received.
//
//*****************************************************************************
BYTE USBHID_handleEP0SetReportDataAvailable (BYTE intfnum)
{
	//Process received data based on currentReportType
    return (TRUE);
}

//*****************************************************************************
//
//! Get_Report Request was Received from the Host
//!
//! \param reportType is either USB_REQ_HID_INPUT, USB_REQ_HID_OUTPUT or
//!                             USB_REQ_HID_FEATURE
//! \param reportId is values defined by report descriptor
//! \param requestedLength is length of report
//! \param intfNum is which HID interface is being used.
//!
//! This event indicates that a Get_Report request was received from the host.
//! The application can supply a buffer of data that will be sent to the host.
//! This handler is passed the reportType, reportId, the requested length as
//! well as the interface number.
//! 
//! Returns TRUE to wake up after data was received.
//
//*****************************************************************************
BYTE *USBHID_handleEP0GetReport (BYTE reportType, BYTE reportId,
    WORD requestedLength,
    BYTE intfnum)
{
	//report data should be ready in buffers for Get Report.
	switch (reportType) {
		case USB_REQ_HID_INPUT:
			//Return pointer to input Report Buffer
			return (0);
		case USB_REQ_HID_OUTPUT:
			//Return pointer to OUTput Report Buffer
			return (0);
		case USB_REQ_HID_FEATURE:
			//Return pointer to FEATURE Report Buffer
			return (0);
		default:
			return (0);
	}
}

#endif //_HID_

#ifdef _MSC_
//*****************************************************************************
//
//! API Requests a Buffer
//!
//! This event occurs when the API requests a buffer. Immediately prior to this,
//! the API sets the operation field of the USBMSC_RWBuf_Info structure
//! corresponding with the request, and also clears the low-power-mode bits of
//! the MCU’s status register to ensure the CPU remains awake to process the
//! buffer after the event occurs. 
//! 
//! NOTE: This means the return value of this event has no effect; the CPU will 															<-- BECAUSE OF THIS...
//! remain awake even if this function returns FALSE.
//
//*****************************************************************************
BYTE USBMSC_handleBufferEvent (VOID)
{
    return (FALSE); //return FALSE to go asleep after interrupt (in the case the CPU slept before interrupt)								<-- ...LOSE COMMENT???
}

#endif //_MSC_

#ifdef _PHDC_

//*****************************************************************************
//
//! Data Received
//! 
//! \param intfNum is which interface is being used.
//!
//! This event indicates that data has been received for interface \b intfNum,
//! but no data receive operation is underway.
//! 
//! Returns TRUE to keep CPU awake, or return FALSE to go asleep after interrupt
//! 	(in the case the CPU slept before interrupt).
//
//*****************************************************************************
BYTE USBPHDC_handleDataReceived (BYTE intfNum)
{
    //TO DO: You can place your code here

    return (FALSE);                              //return FALSE to go asleep after interrupt (in the case the CPU slept before
                                                //interrupt)
}

//*****************************************************************************
//
//! Send Completed
//! 
//! \param intfNum is which interface is being used.
//!
//! This event indicates that a send operation on interface \b intfNum has just
//! been completed.
//! 
//! Returns TRUE to keep CPU awake, or return FALSE to go asleep after interrupt
//! 	(in the case the CPU slept before interrupt).
//
//*****************************************************************************
BYTE USBPHDC_handleSendCompleted (BYTE intfNum)
{
    //TO DO: You can place your code here

    return (FALSE);                             //return FALSE to go asleep after interrupt (in the case the CPU slept before
                                                //interrupt)
}

//*****************************************************************************
//
//! Receive Completed
//! 
//! \param intfNum is which interface is being used.
//!
//! This event indicates that a receive operation on interface \b intfNum has
//! just been completed.
//! 
//! Returns TRUE to keep CPU awake, or return FALSE to go asleep after interrupt
//! 	(in the case the CPU slept before interrupt).
//
//*****************************************************************************
BYTE USBPHDC_handleReceiveCompleted (BYTE intfNum)
{
    //TO DO: You can place your code here

    return (FALSE);                             //return FALSE to go asleep after interrupt (in the case the CPU slept before
                                                //interrupt)
}

#endif //_PHDC_

/*----------------------------------------------------------------------------+
 | End of source file                                                          |
 +----------------------------------------------------------------------------*/
/*------------------------ Nothing Below This Line --------------------------*/
//Released_Version_4_00_02
