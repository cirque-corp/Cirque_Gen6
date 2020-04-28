#ifndef __FLETCHER16_H_INCL__
#define __FLETCHER16_H_INCL__

#include <stdint.h>

/**
 * @fn uint16_t Fletcher16( uint8_t const* data, uint32_t bytes );
 *
 * @brief Calculate the Fletcher 16 checksum of the provided data.
 *
 * @param data  The pointer to the data to checksum.
 * @param bytes The number of bytes to checksum.
 *
 * @return An uint16_t.
 */
uint16_t Fletcher16(uint8_t const* data, uint32_t bytes);

/** Calculate the Fletcher 16 checksum of the provided data, supplying
 *  the checksum result of a previous segment of data.

 *  @param data  The pointer to the data to checksum.
 *  @param bytes The number of bytes to checksum.
 *  @param previousChecksum The checksum of a previous segment of data, as returned by @ref Fletcher16 or this function.
 *
 *  @return The 16-bit checksum as would be calculated if all data were contiguous
 */
uint16_t Fletcher16_Continue(uint8_t const* data, uint32_t bytes, uint16_t previousChecksum);

#endif // __FLETCHER16_H_INCL__
