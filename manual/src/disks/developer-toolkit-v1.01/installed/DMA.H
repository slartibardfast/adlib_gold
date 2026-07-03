/******************************************************************************
				 Public defines

NOTE:   All register definitions are included here to provide an easy 
        escape in case of trouble, and for the sake of documentation.
        The registers should NEVER be accessed directly outside of 
        DMA.C.

        If something is missing in the DMA.c module, feel free to add a new
        function to the toolbox, but PLEASE, don't do any direct
        hardware tampering.

        The definitions will eventually be transferred to the private part of
        DMA.C

******************************************************************************/

/* Dma controller registers                     */
#define dmaBaseAddressRegister          0x00
#define dmaWordCountRegister            0x01


#define dmaStatusRegister               0x08

#define dmaCommandRegister              0x08
#define dmaRequestRegister              0x09
#define dmaSingleMaskRegister           0x0A
#define dmaModeRegister                 0x0B
#define dmaBytePointerRegister          0x0C
#define dmaMasterClearRegister          0x0D
#define dmaClearMaskRegister            0x0E
#define dmaAllMaskRegister              0x0F

#define dmaPageRegister0				 0x87	/* PC AT only */
#define dmaPageRegister1                0x83
#define dmaPageRegister2                0x81
#define dmaPageRegister3                0x82

/* Command register bits                                                */
/* Those should never be modified, since the PC takes care of defining  */
/* the operation mode of the DMA Controller for us.                     */

#define dmaCommandMemoryToMemoryEnable  0x01    
#define dmaCommandAddressHoldEnable     0x02
#define dmaCommandControllerEnable      0x04
#define dmaCommandNormalTiming          0x08
#define dmaCommandRotatingPriority      0x10
#define dmaCommandExtendedWrite         0x20
#define dmaCommandDREQSenseLow          0x40
#define dmaCommandDACKSenseHigh         0x80

/* Request register bits                                                */
/* These are used to generate a DMA request under software control.     */
/* the channel number is loaded in bit 0-1 and bit 2 is set to trigger  */
/* the DMA request.                                                     */

#define dmaRequestChannel               0x03
#define dmaRequestSet                   0x04


/* SingleMask register bits                                             */
/* Writing to this register allows individual DMA Channels to be masked */
/* off or on.                                                           */
/* Channel number is loaded in bits 0-1 and bit 2 is set to 0 to be     */
/* unmasked, or to 1 for the channel to be masked.                      */
 
#define dmaSingleMaskChannel            0x03
#define dmaSingleMaskSet                0x04

/* Mode register bits                                                   */
/* Bits 0-1 define the channel to which the command applies.            */
/* Bits 2-3 define the type of cycle: 00 - Verify                       */
/*                                    01 - Write (device -> memory)     */
/*                                    10 - Read  (memory ->device)      */
/* Bit  4   set autoinitialize on. When autoinitialized, the current    */
/*          address register and count register will be reset to the    */
/*          values contained in the base addres and count registers at  */
/*          the end of the transfer.                                    */
/* Bit 5    is set to 1 to decrement the addresses during the transfer. */
/*          value of 0 increments the addresses.                        */
/*                                                                      */
/* Bits 6-7 select the type of DMA operation:                           */
/*              00-Demand transfer mode (not recommended)               */
/*              01-Single Byte transfer mode (should be used!)          */
/*              10-Block mode (never use on PC)                         */
/*              11-Cascade mode                                         */

#define dmaModeChannel          0x03

#define dmaModeCycleType        0x0A
#define dmaModeCycleVerify      0x00
#define dmaModeCycleWrite       0x04
#define dmaModeCycleRead        0x08

#define dmaModeAutoInit         0x10
#define dmaModeDecrement        0x20

#define dmaModeTransfer         0xA0
#define dmaModeDemandTransfer   0x00
#define dmaModeSingleTransfer    0x40
#define dmaModeBlockTransfer    0x80
#define dmaModeCascadeTransfer  0xA0

/* allMaskRegister                                                      */
/* Is used to set/reset the masks to enable-diable the channels.        */

#define dmaAllMAskChannel0      0x01
#define dmaAllMAskChannel1      0x02
#define dmaAllMAskChannel2      0x04
#define dmaAllMAskChannel3      0x08

/* Read status register                                                 */
/*                                                                      */

#define dmaStatusTerminated0    0x01
#define dmaStatusTerminated1    0x02
#define dmaStatusTerminated2    0x04
#define dmaStatusTerminated3    0x08
#define dmaStatusRequest0       0x10
#define dmaStatusRequest1       0x20
#define dmaStatusRequest2       0x40
#define dmaStatusRequest3       0x80

