/**
 * display.h
 *
 * Header for the display server ingesting data from the ADC.
 *
 * TODO: Include the LCD display once it arrives
 */
#ifndef INCLUDE_DISPLAY_H
#define INCLUDE_DISPLAY_H

#include "defs.h"

typedef enum {
    INVALID_DISPLAY,
    TERMINAL_DISPLAY,
    LCD_DISPLAY,
} DisplayOption;

typedef enum {
    DISPLAY_OPEN,
    DISPLAY_CLOSED,
} DisplayStatus;

#define CHANNEL_COUNT_MAX 2
#define CHANNEL_NAME_MAX 20

typedef struct {
    _Bool active;
    char name[CHANNEL_NAME_MAX];
    double last_value;
} Channel;

typedef struct {
    Channel channels[CHANNEL_COUNT_MAX];
    usize nchannels;
    usize chars_wide;
    usize chars_tall;
    usize col;
    double scale;
    double binwidth;
} TerminalDisplay;

typedef struct {
} LcdDisplay;

union Display {
    TerminalDisplay terminal;
    LcdDisplay lcd;
};

typedef struct {
    DisplayOption variant;
    DisplayStatus status;
    union Display display;
} DisplayFile;

typedef usize ChannelHandle;

RC display_open(DisplayOption opt, DisplayFile **file);
RC display_close(DisplayFile *file);

RC display_add_channel(DisplayFile *file, const char *name, ChannelHandle *hdl);
RC display_remove_channel(DisplayFile *file, ChannelHandle hdl);

RC display_clear(DisplayFile *file);
RC display_redraw(DisplayFile *file);

RC display_set_y(DisplayFile *file, usize dim);
RC display_set_x(DisplayFile *file, usize dim);
RC display_set_scale(DisplayFile *file, double scale);

RC display_writev(DisplayFile *file, ChannelHandle hdl, double *values,
                  usize sz);
RC display_write(DisplayFile *file, ChannelHandle hdl, double value);

#endif // INCLUDE_DISPLAY_H
