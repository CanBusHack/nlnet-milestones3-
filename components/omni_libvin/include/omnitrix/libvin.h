#ifndef OMNITRIX_LIBVIN_H_
#define OMNITRIX_LIBVIN_H_

#include <stdbool.h>

/**
 * Starts a background task to read the VIN.
 */
void omni_libvin_main(void);

/**
 * Gets the currently read VIN from memory, if it exists.
 * `buf` must be a char array of length 17.
 * Returns false if the VIN has not been read, in which case `buf` is not modified.
 */
bool omni_libvin_get_vin(char buf[17]);

#endif
