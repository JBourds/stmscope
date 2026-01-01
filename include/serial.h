#ifndef INCLUDE_SERIAL_H
#define INCLUDE_SERIAL_H
#include "defs.h"

int _write(int file, char *ptr, int len);

RC serial_init(void);

#endif // INCLUDE_SERIAL_H
