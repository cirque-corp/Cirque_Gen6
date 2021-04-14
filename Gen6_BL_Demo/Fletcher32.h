/*--------------------------------------------------------------------------*\

    Filename       : Fletcher32.h
    Description    : This function provides Fletcher32 checksum.

  ------------------
    Date           : 01/09/2012
    Copyright      : (C) 2011 by Cirque Corporation
    Author         : John Lagerquist

    This Software is copyrighted and owned by Cirque Corporation and is being
    licensed, not sold.  Licensee acknowledges that the Software contains
    copyrighted and proprietary material and Licensee may not decompile,
    reverse engineer or otherwise reduce the object code form of the
    Software.  Licensee may not modify, sell, rent, lease, loan, distribute,
    or create derivative works based upon the Software in whole or in part.
    Licensee may not remove any copyright or other proprietary notices from
    the Software.  Licensee may not copy or reproduce the Software.  Cirque
    Corporation does not grant any express or implied right to Licensee
    under any patents, copyrights, trademarks, trade secret or any other
    intellectual property or proprietary right.

\*--------------------------------------------------------------------------*/

#ifndef __FLETCHER32_H_INCL__
#define __FLETCHER32_H_INCL__

#include <stdint.h>

/** 
 * Calculate a Fletcher 32 checksum on the given memory range.  
 * 
 * @param ptr - a pointer to the memory region
 * @param n_bytes - the number of bytes
 * 
 * @return uint32_t - the calculated checksum
 */
extern uint32_t Fletcher32(void const *ptr, uint32_t n_bytes);

/**
 * Continue modifying a checksum value with more data.
 * This can be helpful if not all data is accessible at once, but must be accessed
 * in segments.
 * 
 * @param ptr - a pointer to the memory region
 * @param n_bytes - the number of bytes
 * @param previousChecksum The previous checksum value from either Fletcher32 or Fletcher32_Continue
 * 
 * @return uint32_t - the calculated checksum
 */
uint32_t Fletcher32_Continue(void const *ptr, uint32_t n_bytes, uint32_t previousChecksum);

#endif // __FLETCHER32_H_INCL__

