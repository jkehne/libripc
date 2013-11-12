#ifndef BASE64_H_
#define BASE64_H_

#include <stdint.h>

void base64(char *destination, const uint8_t *source, size_t lengthDest, size_t lengthSource);

#endif // BASE64_H_
