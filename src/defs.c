#include "defs.h"

const char *rcstr(RC rc) {
    switch (rc) {
    case RC_OK:
        return "Ok";
    case RC_ALREADY_OPEN:
        return "Resource already open";
    case RC_OPEN_FAILED:
        return "Unable to open resource";
    case RC_START_FAILED:
        return "Unable to start resource";
    case RC_NOT_OPEN:
        return "Resource has not been opened";
    case RC_CLOSE_FAILED:
        return "Resource could not be closed";
    case RC_INVALID_OPT:
        return "Invalid option";
    case RC_CHANNEL_COUNT:
        return "Too many channels";
    case RC_BUF_LENGTH:
        return "Buffer is too small";
    default:
        return "?";
    }
}
