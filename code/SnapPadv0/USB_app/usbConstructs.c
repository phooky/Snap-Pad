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
/** @file usbConstructs.c
 *  @brief Contains example constructs for send/receive operations
 */
/* 
 * ======== usbConstructs.c ========
 */
//
//! \cond
//

#include "USB_API/USB_Common/device.h"
#include "USB_API/USB_Common/types.h"          // Basic Type declarations

#include "USB_config/descriptors.h"
#include "USB_API/USB_Common/usb.h"        // USB-specific functions

#ifdef _CDC_
    #include "USB_API/USB_CDC_API/UsbCdc.h"
#endif
#ifdef _HID_
    #include "USB_API/USB_HID_API/UsbHid.h"
#endif
#ifdef _PHDC_
    #include "USB_API/USB_PHDC_API/UsbPHDC.h"
#endif

#include <intrinsics.h>
#include "usbConstructs.h"


//
//! \endcond
//

/**************************************************************************************************
These are example, user-editable construct functions for calling the API.  

In cases where fast development is the priority, it's usually best to use these sending  
construct functions, rather than calling USBCDC_sendData() or USBHID_sendData() 
directly.  This is because they put boundaries on the "background execution" of sends,
simplfying the application.  

xxxsendDataWaitTilDone() essentially eliminates background processing altogether, always 
polling after the call to send and not allowing execution to resume until it's done.  This
allows simpler coding at the expense of wasted MCU cycles, and MCU execution being "locked" 
to the host (also called "synchronous" operation).  

xxxsendDataInBackground() takes advantage of background processing ("asynchronous" operation) 
by allowing sending to happen during application execution; while at the same time ensuring 
that the sending always definitely occurs.  It provides most of the simplicity of 
xxxsendDataWaitTilDone() while minimizing wasted cycles.  It's probably the best choice 
for most applications.    

A true, asynchronous implementation would be the most cycle-efficient, but is the most 
difficult to code; and can't be "contained" in an example function as these other approaches 
are.  Such an implementation might be advantageous in RTOS-based implementations or those 
requiring the highest levels of efficiency.  

These functions take into account all the pertinent return codes, toward ensuring fully 
robust operation.   The send functions implement a timeout feature, using a loose "number of 
retries" approach.  This was done in order to avoid using additional hardware resources.  A 
more sophisticated approach, which the developer might want to implement, would be to use a 
hardware timer.  

Please see the MSP430 CDC/HID/MSC USB API Programmer's Guide for a full description of these 
functions, how they work, and how to use them.  
**************************************************************************************************/



#ifdef _HID_
//*****************************************************************************
//
//! Completely Sends the Data in dataBuf
//! 
//! \param *dataBuf is the address of the data buffer.
//! \param size is the size of the data.
//! \param intfnum intfNum is which HID interface is being used.
//! \param ulTimeout is the (32-bit) number of polls to USBHID_intfStatus().
//!
//! Sends the data in \b dataBuf, of size \b size, using the post-call polling method.
//! It does so over interface \b intfNum. The function doesn’t return until the
//! send has completed. Because of this, the user buffer can be edited
//! immediately after the function returns, without consequence.  The
//! function assumes that size is non-zero.  It assumes no previous send
//! operation is underway. 
//! 
//! The 32-bit number \b ulTimeout selects how many times USBHID_intfStatus() will
//! be polled while waiting for the operation to complete. If the value is zero,
//! then no timeout is employed; it will wait indefinitely. When choosing a
//! number, it is advised to consider MCLK speed, as a faster CPU will cycle
//! through the calls more quickly.  The function provides the simplest coding,
//! at the expense of wasted cycles and potentially allowing MCU execution to
//! become "locked" to the host, a disadvantage if the host (or bus) is slow.
//! 
//! The function also checks all valid return codes, and returns non-zero if an
//! error occurred.  In many applications, the return value can simply be
//! evaluated as zero or non-zero, where nonzero means the call failed for
//! reasons of host or bus non-availability. Therefore, it may desirable for the
//! application to break from execution. Other applications may wish to handle
//! return values 1 and 2 in different ways.
//! 
//! It’s recommended not to call this function from within an event handler.
//! This is because if an interface currently has an open send operation, the
//! operation will never complete during the event handler; rather, only after
//! the ISR that spawned the event returns. Thus the USBHID_intfStatus() polling
//! would loop indefinitely (or timeout). It’s better to set a flag from within
//! the event handler, and use this flag to trigger the calling of this function
//! from within main().
//! 
//! \return \b 0 if the call succeeded; all data has been sent.
//! \return \b 1 if the call timed out, either because the host is unavailable
//! 	or a COM port with an active application on the host wasn't opened.
//! \return \b 2 if the bus is unavailable.
//
//*****************************************************************************
BYTE hidSendDataWaitTilDone (BYTE* dataBuf,
    WORD size,
    BYTE intfNum,
    ULONG ulTimeout)
{
    ULONG sendCounter = 0;
    WORD bytesSent, bytesReceived;

    switch (USBHID_sendData(dataBuf,size,intfNum)){
        case kUSBHID_sendStarted:
            break;
        case kUSBHID_busNotAvailable:
            return ( 2) ;
        case kUSBHID_intfBusyError:
            return ( 3) ;
        case kUSBHID_generalError:
            return ( 4) ;
        default:;
    }

    /* If execution reaches this point, then the operation successfully started.  Now wait til it's finished. */
    while (1){
        BYTE ret = USBHID_intfStatus(intfNum,&bytesSent,&bytesReceived);
        if (ret & kUSBHID_busNotAvailable){                 /* This may happen at any time */
            return ( 2) ;
        }
        if (ret & kUSBHID_waitingForSend){
            if (ulTimeout && (sendCounter++ >= ulTimeout)){ /* Incr counter & try again */
                return ( 1) ;                               /* Timed out */
            }
        } else {
            return ( 0) ;                                   /* If neither busNotAvailable nor waitingForSend, it succeeded */
        }
    }
}



//*****************************************************************************
//
//! Completely Sends the Data in dataBuf
//! 
//! \param *dataBuf is the address of the data buffer.
//! \param size is the size of the data.
//! \param intfnum intfNum is which HID interface is being used.
//! \param ulTimeout is the (32-bit) number of polls to USBHID_intfStatus().
//!
//! Sends the data in \b dataBuf, of size \b size, using the pre-call polling
//! method. It does so over interface \b intfNum. The send operation may still
//! be active after the function returns, and \b dataBuf should not be edited
//! until it can be verified that the operation has completed. The function
//! assumes that size is non-zero.  This call assumes a previous send operation
//! might be underway.
//! 
//! The 32-bit number \b ulTimeout selects how many times USBHID_intfStatus()
//! will be polled while waiting for the previous operation to complete. If the
//! value is zero, then no timeout is employed; it will wait indefinitely. When
//! choosing a number, it is advised to consider MCLK speed, as a faster CPU
//! will cycle through the calls more quickly.  The function provides simple
//! coding while also taking advantage of the efficiencies of background
//! processing.  If a previous send operation is underway, this function does
//! waste cycles polling, like xxxsendDataWaitTilDone(); however it's less
//! likely to do so since much of the sending presumably took place in the 
//! background since the last call to xxxsendDataInBackground().
//! 
//! The function also checks all valid return codes, and returns non-zero if an
//! error occurred.  In many applications, the return value can simply be
//! evaluated as zero or non-zero, where nonzero means the call failed for
//! reasons of host or bus non-availability. Therefore, it may desirable for the
//! application to break from execution. Other applications may wish to handle
//! return values 1 and 2 in different ways.
//! 
//! It’s recommended not to call this function from within an event handler.
//! This is because if an interface currently has an open send operation, the
//! operation will never complete during the event handler; rather, only after
//! the ISR that spawned the event returns. Thus the USBHID_intfStatus() polling
//! would loop indefinitely (or timeout). It’s better to set a flag from within
//! the event handler, and use this flag to trigger the calling of this function
//! from within main().
//! 
//! \return \b 0 if the call succeeded; all data has been sent.
//! \return \b 1 if the call timed out, either because the host is unavailable
//! 	or a COM port with an active application on the host wasn't opened.
//! \return \b 2 if the bus is unavailable.
//
//*****************************************************************************
BYTE hidSendDataInBackground (BYTE* dataBuf,
    WORD size,
    BYTE intfNum,
    ULONG ulTimeout)
{
    ULONG sendCounter = 0;
    WORD bytesSent, bytesReceived;

    while (USBHID_intfStatus(intfNum,&bytesSent,
               &bytesReceived) & kUSBHID_waitingForSend){
        if (ulTimeout && ((sendCounter++) > ulTimeout)){    /* A send operation is underway; incr counter & try again */
            return ( 1) ;                                   /* Timed out */
        }
    }

    /* The interface is now clear.  Call sendData(). */
    switch (USBHID_sendData(dataBuf,size,intfNum)){
        case kUSBHID_sendStarted:
            return ( 0) ;
        case kUSBHID_busNotAvailable:
            return ( 2) ;
        default:
            return ( 4) ;
    }
}
                                  

  
                         
//*****************************************************************************
//
//! Opens a Receive Operation
//! 
//! \param *dataBuf is the address of the data buffer.
//! \param size is the size of the data.
//! \param intfnum intfNum is which HID interface is being used.
//!
//! Opens a brief receive operation for any data that has already been received
//! into the USB buffer over interface \b intfNum. This call only retrieves data
//! that is already waiting in the USB buffer -- that is, data that has already
//! been received by the MCU.  It assumes a previous, open receive operation
//! (began by a direct call to USBxxx_receiveData()) is NOT underway on this
//! interface; and no receive operation remains open after this call returns.
//! It doesn't check for kUSBxxx_busNotAvailable, because it doesn't matter if
//! it's not.  The data in the USB buffer is copied into \b dataBuf, and the
//! function returns the number of bytes received.
//! 
//! \b size is the maximum that is allowed to be received before exiting; i.e.,
//! it is the size allotted to \b dataBuf.  If \b size bytes are received, the
//! function ends, returning \b size. In this case, it’s possible that more
//! bytes are still in the USB buffer; it might be a good idea to open another
//! receive operation to retrieve them. For this reason, operation is simplified
//! by using large \b size values, since it helps ensure all the data is
//! retrieved at one time.
//! 
//! This function is usually called when a USBHID_handleDataReceived() event
//! flags the application that data has been received into the USB buffer.
//! 
//! \return The number of bytes received into \b dataBuf.
//
//*****************************************************************************
WORD hidReceiveDataInBuffer (BYTE* dataBuf, WORD size, BYTE intfNum)
{
    WORD bytesInBuf;
	WORD rxCount = 0;
    BYTE* currentPos = dataBuf;

    while (bytesInBuf = USBHID_bytesInUSBBuffer(intfNum)){
        if ((WORD)(currentPos - dataBuf + bytesInBuf) <= size){
            rxCount = bytesInBuf;
			USBHID_receiveData(currentPos,rxCount,intfNum);
        	currentPos += rxCount;
        } else {
            rxCount = size - (currentPos - dataBuf);
			USBHID_receiveData(currentPos,rxCount,intfNum);
        	currentPos += rxCount;
			return (currentPos - dataBuf);
        }
    }
	
	return (currentPos - dataBuf);
}

#endif

/*********************************************************************************************
Please see the MSP430 USB CDC API Programmer's Guide Sec. 9 for a full description of these 
functions, how they work, and how to use them.  
**********************************************************************************************/

#ifdef _CDC_
//*****************************************************************************
//
//! Completely Sends the Data in dataBuf
//! 
//! \param *dataBuf is the address of the data buffer.
//! \param size is the size of the data.
//! \param intfnum intfNum is which interface is being used.
//! \param ulTimeout is the (32-bit) number of polls to USBCDC_intfStatus().
//!
//! Sends the data in \b dataBuf, of size \b size, using the post-call polling method.
//! It does so over interface \b intfNum. The function doesn’t return until the
//! send has completed. Because of this, the user buffer can be edited
//! immediately after the function returns, without consequence.  The
//! function assumes that size is non-zero.  It assumes no previous send
//! operation is underway. 
//! 
//! The 32-bit number \b ulTimeout selects how many times USBCDC_intfStatus() will
//! be polled while waiting for the operation to complete. If the value is zero,
//! then no timeout is employed; it will wait indefinitely. When choosing a
//! number, it is advised to consider MCLK speed, as a faster CPU will cycle
//! through the calls more quickly.  The function provides the simplest coding,
//! at the expense of wasted cycles and potentially allowing MCU execution to
//! become "locked" to the host, a disadvantage if the host (or bus) is slow.
//! 
//! The function also checks all valid return codes, and returns non-zero if an
//! error occurred.  In many applications, the return value can simply be
//! evaluated as zero or non-zero, where nonzero means the call failed for
//! reasons of host or bus non-availability. Therefore, it may desirable for the
//! application to break from execution. Other applications may wish to handle
//! return values 1 and 2 in different ways.
//! 
//! It’s recommended not to call this function from within an event handler.
//! This is because if an interface currently has an open send operation, the
//! operation will never complete during the event handler; rather, only after
//! the ISR that spawned the event returns. Thus the USBCDC_intfStatus() polling
//! would loop indefinitely (or timeout). It’s better to set a flag from within
//! the event handler, and use this flag to trigger the calling of this function
//! from within main().
//! 
//! \return \b 0 if the call succeeded; all data has been sent.
//! \return \b 1 if the call timed out, either because the host is unavailable
//! 	or a COM port with an active application on the host wasn't opened.
//! \return \b 2 if the bus is unavailable.
//
//*****************************************************************************
BYTE cdcSendDataWaitTilDone (BYTE* dataBuf,
    WORD size,
    BYTE intfNum,
    ULONG ulTimeout)
{
    ULONG sendCounter = 0;
    WORD bytesSent, bytesReceived;

    switch (USBCDC_sendData(dataBuf,size,intfNum))
    {
        case kUSBCDC_sendStarted:
            break;
        case kUSBCDC_busNotAvailable:
            return ( 2) ;
        case kUSBCDC_intfBusyError:
            return ( 3) ;
        case kUSBCDC_generalError:
            return ( 4) ;
        default:;
    }

    /* If execution reaches this point, then the operation successfully started.  Now wait til it's finished. */
    while (1){
        BYTE ret = USBCDC_intfStatus(intfNum,&bytesSent,&bytesReceived);
        if (ret & kUSBCDC_busNotAvailable){                 /* This may happen at any time */
            return ( 2) ;
        }
        if (ret & kUSBCDC_waitingForSend){
            if (ulTimeout && (sendCounter++ >= ulTimeout)){ /* Incr counter & try again */
                return ( 1) ;                               /* Timed out */
            }
        } else {
            return ( 0) ;                                   /* If neither busNotAvailable nor waitingForSend, it succeeded */
        }
    }
}




//*****************************************************************************
//
//! Completely Sends the Data in dataBuf
//! 
//! \param *dataBuf is the address of the data buffer.
//! \param size is the size of the data.
//! \param intfnum intfNum is which interface is being used.
//! \param ulTimeout is the (32-bit) number of polls to USBCDC_intfStatus().
//!
//! Sends the data in \b dataBuf, of size \b size, using the pre-call polling
//! method. It does so over interface \b intfNum. The send operation may still
//! be active after the function returns, and \b dataBuf should not be edited
//! until it can be verified that the operation has completed. The function
//! assumes that size is non-zero.  This call assumes a previous send operation
//! might be underway.
//! 
//! The 32-bit number \b ulTimeout selects how many times USBCDC_intfStatus()
//! will be polled while waiting for the previous operation to complete. If the
//! value is zero, then no timeout is employed; it will wait indefinitely. When
//! choosing a number, it is advised to consider MCLK speed, as a faster CPU
//! will cycle through the calls more quickly.  The function provides simple
//! coding while also taking advantage of the efficiencies of background
//! processing.  If a previous send operation is underway, this function does
//! waste cycles polling, like xxxsendDataWaitTilDone(); however it's less
//! likely to do so since much of the sending presumably took place in the 
//! background since the last call to xxxsendDataInBackground().
//! 
//! The function also checks all valid return codes, and returns non-zero if an
//! error occurred.  In many applications, the return value can simply be
//! evaluated as zero or non-zero, where nonzero means the call failed for
//! reasons of host or bus non-availability. Therefore, it may desirable for the
//! application to break from execution. Other applications may wish to handle
//! return values 1 and 2 in different ways.
//! 
//! It’s recommended not to call this function from within an event handler.
//! This is because if an interface currently has an open send operation, the
//! operation will never complete during the event handler; rather, only after
//! the ISR that spawned the event returns. Thus the USBCDC_intfStatus() polling
//! would loop indefinitely (or timeout). It’s better to set a flag from within
//! the event handler, and use this flag to trigger the calling of this function
//! from within main().
//! 
//! \return \b 0 if the call succeeded; all data has been sent.
//! \return \b 1 if the call timed out, either because the host is unavailable
//! 	or a COM port with an active application on the host wasn't opened.
//! \return \b 2 if the bus is unavailable.
//
//*****************************************************************************
BYTE cdcSendDataInBackground (BYTE* dataBuf,
    WORD size,
    BYTE intfNum,
    ULONG ulTimeout)
{
    ULONG sendCounter = 0;
    WORD bytesSent, bytesReceived;

    while (USBCDC_intfStatus(intfNum,&bytesSent,
               &bytesReceived) & kUSBCDC_waitingForSend){
        if (ulTimeout && ((sendCounter++) > ulTimeout)){    /* A send operation is underway; incr counter & try again */
            return ( 1) ;                                   /* Timed out                */
        }
    }

    /* The interface is now clear.  Call sendData().   */
    switch (USBCDC_sendData(dataBuf,size,intfNum)){
        case kUSBCDC_sendStarted:
            return ( 0) ;
        case kUSBCDC_busNotAvailable:
            return ( 2) ;
        default:
            return ( 4) ;
    }
}
                                 

                         
                         
//*****************************************************************************
//
//! Opens a Receive Operation
//! 
//! \param *dataBuf is the address of the data buffer.
//! \param size is the size of the data.
//! \param intfnum intfNum is which CDC interface is being used.
//!
//! Opens a brief receive operation for any data that has already been received
//! into the USB buffer over interface \b intfNum. This call only retrieves data
//! that is already waiting in the USB buffer -- that is, data that has already
//! been received by the MCU.  It assumes a previous, open receive operation
//! (began by a direct call to USBxxx_receiveData()) is NOT underway on this
//! interface; and no receive operation remains open after this call returns.
//! It doesn't check for kUSBxxx_busNotAvailable, because it doesn't matter if
//! it's not.  The data in the USB buffer is copied into \b dataBuf, and the
//! function returns the number of bytes received.
//! 
//! \b size is the maximum that is allowed to be received before exiting; i.e.,
//! it is the size allotted to \b dataBuf.  If \b size bytes are received, the
//! function ends, returning \b size. In this case, it’s possible that more
//! bytes are still in the USB buffer; it might be a good idea to open another
//! receive operation to retrieve them. For this reason, operation is simplified
//! by using large \b size values, since it helps ensure all the data is
//! retrieved at one time.
//! 
//! This function is usually called when a USBCDC_handleDataReceived() event
//! flags the application that data has been received into the USB buffer.
//! 
//! \return The number of bytes received into \b dataBuf.
//
//*****************************************************************************
WORD cdcReceiveDataInBuffer (BYTE* dataBuf, WORD size, BYTE intfNum)
{
    WORD bytesInBuf;
	WORD rxCount = 0;
    BYTE* currentPos = dataBuf;

    while (bytesInBuf = USBCDC_bytesInUSBBuffer(intfNum)){
        if ((WORD)(currentPos - dataBuf + bytesInBuf) <= size){
            rxCount = bytesInBuf;
			USBCDC_receiveData(currentPos,rxCount,intfNum);
        	currentPos += rxCount;
        } else {
            rxCount = size - (currentPos - dataBuf);
			USBCDC_receiveData(currentPos,rxCount,intfNum);
        	currentPos += rxCount;
			return (currentPos - dataBuf);
        }
    }
	
	return (currentPos - dataBuf);
}

#endif

#ifdef _PHDC_
/* This construct implements post-call polling to ensure the sending completes before the function
 * returns.  It provides the simplest coding, at the expense of wasted cycles and potentially
 * allowing MCU execution to become "locked" to the host, a disadvantage if the host (or bus) is
 * slow.  The function also checks all valid return codes, and returns non-zero if an error occurred.
 * It assumes no previous send operation is underway; also assumes size is non-zero.  */
BYTE phdcSendDataWaitTilDone (BYTE* dataBuf,
    WORD size,
    BYTE intfNum,
    ULONG ulTimeout)
{
    ULONG sendCounter = 0;
    WORD bytesSent, bytesReceived;

    switch (USBPHDC_sendData(dataBuf,size,intfNum))
    {
        case kUSBPHDC_sendStarted:
            break;
        case kUSBPHDC_busNotAvailable:
            return ( 2) ;
        case kUSBPHDC_intfBusyError:
            return ( 3) ;
        case kUSBPHDC_generalError:
            return ( 4) ;
        default:;
    }

    /* If execution reaches this point, then the operation successfully started.  Now wait til it's finished. */
    while (1){
        BYTE ret = USBPHDC_intfStatus(intfNum,&bytesSent,&bytesReceived);
        if (ret & kUSBPHDC_busNotAvailable){                 /* This may happen at any time */
            return ( 2) ;
        }
        if (ret & kUSBPHDC_waitingForSend){
            if (ulTimeout && (sendCounter++ >= ulTimeout)){ /* Incr counter & try again */
                return ( 1) ;                               /* Timed out */
            }
        } else {
            return ( 0) ;                                   /* If neither busNotAvailable nor waitingForSend, it succeeded */
        }
    }
}

/* This construct implements pre-call polling to ensure the sending completes before the function
 * returns.  It provides simple coding while also taking advantage of the efficiencies of background
 * processing.  If a previous send operation is underway, this function does waste cycles polling,
 * like xxxsendDataWaitTilDone(); however it's less likely to do so since much of the sending
 * presumably took place in the background since the last call to xxxsendDataInBackground().
 * The function also checks all valid return codes, and returns non-zero if an error occurred.
 * It assumes no previous send operation is underway; also assumes size is non-zero.
 * This call assumes a previous send operation might be underway; also assumes size is non-zero.
 * Returns zero if send completed; non-zero if it failed, with 1 = timeout and 2 = bus is gone. */
BYTE phdcSendDataInBackground (BYTE* dataBuf,
    WORD size,
    BYTE intfNum,
    ULONG ulTimeout)
{
    ULONG sendCounter = 0;
    WORD bytesSent, bytesReceived;

    while (USBPHDC_intfStatus(intfNum,&bytesSent,
               &bytesReceived) & kUSBPHDC_waitingForSend){
        if (ulTimeout && ((sendCounter++) > ulTimeout)){    /* A send operation is underway; incr counter & try again */
            return ( 1) ;                                   /* Timed out                */
        }
    }

    /* The interface is now clear.  Call sendData().   */
    switch (USBPHDC_sendData(dataBuf,size,intfNum)){
        case kUSBPHDC_sendStarted:
            return ( 0) ;
        case kUSBPHDC_busNotAvailable:
            return ( 2) ;
        default:
            return ( 4) ;
    }
}

/* This call only retrieves data that is already waiting in the USB buffer -- that is, data that has
 * already been received by the MCU.  It assumes a previous, open receive operation (began by a direct
 * call to USBxxx_receiveData()) is NOT underway on this interface; and no receive operation remains
 * open after this call returns.  It doesn't check for kUSBxxx_busNotAvailable, because it doesn't
 * matter if it's not.  size is the maximum that is allowed to be received before exiting; i.e., it
 * is the size allotted to dataBuf.  Returns the number of bytes received. */
WORD phdcReceiveDataInBuffer (BYTE* dataBuf, WORD size, BYTE intfNum)
{
    WORD bytesInBuf;
	WORD rxCount = 0;
    BYTE* currentPos = dataBuf;

    while (bytesInBuf = USBPHDC_bytesInUSBBuffer(intfNum)){
        if ((WORD)(currentPos - dataBuf + bytesInBuf) <= size){
            rxCount = bytesInBuf;
			USBPHDC_receiveData(currentPos,rxCount,intfNum);
        	currentPos += rxCount;
        } else {
            rxCount = size - (currentPos - dataBuf);
			USBPHDC_receiveData(currentPos,rxCount,intfNum);
        	currentPos += rxCount;
			return (currentPos - dataBuf);
        }
    }
	
	return (currentPos - dataBuf);
}

#endif
//Released_Version_4_00_02
