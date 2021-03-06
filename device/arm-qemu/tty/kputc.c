/**
 * @file kputc.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */
#include <xinu.h>
#include <uart.h>

/**
 * @ingroup uarthardware
 *
 * Synchronously write a character to a UART.  This blocks until the character
 * has been written to the hardware.  The interrupt handler is not used.
 *
 * @param c
 *      The character to write.
 * @param devptr
 *      Pointer to the device table entry for a UART.
 *
 * @return
 *      The character written to the UART as an <code>unsigned char</code> cast
 *      to an <code>int</code>.
 */
syscall kputc(uchar c, struct dentry *devptr)
{
    struct	ttycblk	*typtr;
    volatile struct uart_csreg *regptr;
    uint uart_im;

    /* Get pointers to the UART and to its registers.  */
    typtr = &ttytab[devptr->dvminor];
    regptr = devptr->dvcsr;

    /* Save the UART's interrupt state and disable the UART's interrupts.  Note:
     * we do not need to disable global interrupts here; only UART interrupts
     * must be disabled, to prevent race conditions with the UART interrupt
     * handler.  */
    uart_im = regptr->imsc;
    regptr->imsc = 0;

    /* Wait until UART is ready for another character  */
    while ((regptr->fr & PL011_FR_TXFF))
    {
        /* Do nothing */
    }

    /* Put the character to the UART by writing it to the UART's data register.
     * */
    regptr->dr = c;

    /* Tally one character sent.  */
    typtr->cout++;

    /* Restore UART interrupts and return the put character.  */
    regptr->imsc = uart_im;
    return c;
}
