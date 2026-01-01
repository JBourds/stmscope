#ifndef INCLUDE_DEFS_H
#define INCLUDE_DEFS_H
#include <stddef.h>
#include <stdint.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t usize;

typedef enum {
    RC_OK,
    RC_ALREADY_OPEN,
    RC_OPEN_FAILED,
    RC_START_FAILED,
    RC_NOT_OPEN,
    RC_CLOSE_FAILED,
    RC_INVALID_OPT,
    RC_CHANNEL_COUNT,
    RC_BUF_LENGTH,
} RC;

const char *rcstr(RC rc);

#endif // INCLUDE_DEFS_H
