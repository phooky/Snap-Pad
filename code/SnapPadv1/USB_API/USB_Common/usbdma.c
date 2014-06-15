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

//
//! \cond
//

/* 
 * ======== dma.c ========
 */
#include <string.h>

#include "driverlib.h"

#include "../USB_Common/device.h"
#include "../USB_Common/types.h"    //Basic Type declarations
#include "../USB_Common/defMSP430USB.h"
#include <descriptors.h>
#include <string.h>

#ifdef __REGISTER_MODEL__
/* for IAR */
#   if __REGISTER_MODEL__ == __REGISTER_MODEL_REG20__
#       define __DMA_ACCESS_REG__ (void __data20 *)
#   else
#       define __DMA_ACCESS_REG__ (unsigned short)
#   endif
#else
/* for CCS */
#   define __DMA_ACCESS_REG__ (__SFR_FARPTR)(unsigned long)
#endif

//function pointers
VOID *(*USB_TX_memcpy)(VOID * dest, const VOID * source, size_t count);
VOID *(*USB_RX_memcpy)(VOID * dest, const VOID * source, size_t count);

VOID * memcpyDMA0 (VOID * dest, const VOID * source, size_t count);
VOID * memcpyDMA1 (VOID * dest, const VOID * source, size_t count);
VOID * memcpyDMA2 (VOID * dest, const VOID * source, size_t count);

//NOTE: this functin works only with data in the area <64k (small memory model)
VOID * memcpyV (VOID * dest, const VOID * source, size_t count)
{
    WORD i;
    volatile BYTE bTmp;

    for (i = 0; i < count; i++)
    {
        bTmp = *((BYTE*)source + i);
        *((BYTE*)dest  + i) = bTmp;
    }
    return (dest);
}

VOID * memcpyDMA (VOID * dest, const VOID *  source, size_t count)
{
    if (count == 0){                                        //do nothing if zero bytes to transfer
        return (dest);
    }

    //DMA4 workaround - disable DMA transfers during read-modify-write CPU 
    //operations
    DMA_disableTransferDuringReadModifyWrite(DMA_BASE);
    DMA_setSrcAddress(DMA_BASE, USB_DMA_CHAN, (uint32_t)source, DMA_DIRECTION_INCREMENT);
    DMA_setDstAddress(DMA_BASE, USB_DMA_CHAN, (uint32_t)dest, DMA_DIRECTION_INCREMENT);
    //DMA4 workaround - re-enable DMA transfers during read-modify-write CPU 
    //operations
    DMA_enableTransferDuringReadModifyWrite(DMA_BASE);
    DMA_setTransferSize(DMA_BASE, USB_DMA_CHAN, count);
    DMA_enableTransfers(DMA_BASE, USB_DMA_CHAN);
    DMA_startTransfer(DMA_BASE, USB_DMA_CHAN);

    while (DMA_getInterruptStatus(DMA_BASE, USB_DMA_CHAN) == DMA_INT_INACTIVE);

    DMA_disableTransfers(DMA_BASE, USB_DMA_CHAN);

    return (dest);
}

//this function inits the DMA
VOID USB_initMemcpy (VOID)
{
    USB_TX_memcpy = memcpyV;
    USB_RX_memcpy = memcpyV;

    if (USB_DMA_CHAN != 0xFF) {
    	DMA_init (DMA_BASE, USB_DMA_CHAN, DMA_TRANSFER_BLOCK, 0,
    		DMA_TRIGGERSOURCE_0, DMA_SIZE_SRCBYTE_DSTBYTE, DMA_TRIGGER_HIGH);
        USB_TX_memcpy = memcpyDMA;
        USB_RX_memcpy = memcpyDMA;
    }
}

//
//! \endcond
//

/*----------------------------------------------------------------------------+
 | End of source file                                                          |
 +----------------------------------------------------------------------------*/
/*------------------------ Nothing Below This Line --------------------------*/
//Released_Version_4_00_02
