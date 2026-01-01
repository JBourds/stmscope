#include "display.h"
#include <stdio.h>
#include <string.h>

#define CLEAR_SCREEN "\033c\n"

#define RESET "\033[0m"
#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"

#define BOLDBLACK "\033[1m\033[30m"
#define BOLDRED "\033[1m\033[31m"
#define BOLDGREEN "\033[1m\033[32m"
#define BOLDYELLOW "\033[1m\033[33m"
#define BOLDBLUE "\033[1m\033[34m"
#define BOLDMAGENTA "\033[1m\033[35m"
#define BOLDCYAN "\033[1m\033[36m"
#define BOLDWHITE "\033[1m\033[37m"

const char *COLORS[CHANNEL_COUNT_MAX] = {
    BOLDGREEN,
    BOLDYELLOW,
};

// shared functions
static RC add_channel(Channel *channels, usize *nchannels, const char *name,
                      ChannelHandle *hdl);
static RC remove_channel(Channel *channels, usize *nchannels,
                         ChannelHandle hdl);

// terminal function declarations
static RC terminal_open(TerminalDisplay *term, DisplayFile **file);
static RC terminal_close(TerminalDisplay *term);
static RC terminal_clear(TerminalDisplay *term);
static RC terminal_redraw(TerminalDisplay *term);
static RC terminal_add_channel(TerminalDisplay *term, const char *name,
                               ChannelHandle *hdl);
static RC terminal_remove_channel(TerminalDisplay *term, ChannelHandle hdl);
static RC terminal_set_scale(TerminalDisplay *file, double scale);
static RC terminal_set_y(TerminalDisplay *term, usize chars_tall);
static RC terminal_set_x(TerminalDisplay *term, usize chars_wide);
static RC terminal_writev(TerminalDisplay *term, ChannelHandle hdl,
                          double *values, usize sz);
static RC terminal_write(TerminalDisplay *term, ChannelHandle hdl,
                         double value);

// lcd function declarations
static RC lcd_open(LcdDisplay *lcd, DisplayFile **file);
static RC lcd_close(LcdDisplay *lcd);
static RC lcd_clear(LcdDisplay *lcd);
static RC lcd_redraw(LcdDisplay *lcd);
static RC lcd_add_channel(LcdDisplay *lcd, const char *name,
                          ChannelHandle *hdl);
static RC lcd_remove_channel(LcdDisplay *lcd, ChannelHandle hdl);
static RC lcd_set_scale(LcdDisplay *file, double scale);
static RC lcd_set_y(LcdDisplay *lcd, usize pixels_tall);
static RC lcd_set_x(LcdDisplay *lcd, usize pixels_wide);
static RC lcd_writev(LcdDisplay *lcd, ChannelHandle hdl, double *values,
                     usize sz);
static RC lcd_write(LcdDisplay *lcd, ChannelHandle hdl, double value);

// singletons
static DisplayFile TERMINAL = {
    .variant = TERMINAL_DISPLAY,
    .status = DISPLAY_CLOSED,
};

static DisplayFile LCD = {
    .variant = LCD_DISPLAY,
    .status = DISPLAY_CLOSED,
};

RC display_open(DisplayOption opt, DisplayFile **file) {
    switch (opt) {
    case TERMINAL_DISPLAY:
        return terminal_open(&TERMINAL.display.terminal, file);
    case LCD_DISPLAY:
        return lcd_open(&TERMINAL.display.lcd, file);
    default:
        return RC_INVALID_OPT;
    }
}

RC display_close(DisplayFile *file) {
    switch (file->variant) {
    case TERMINAL_DISPLAY:
        if (TERMINAL.status == DISPLAY_CLOSED) {
            return RC_NOT_OPEN;
        }
        terminal_close(&file->display.terminal);
        TERMINAL.status = DISPLAY_CLOSED;
        break;
    case LCD_DISPLAY:
        if (LCD.status == DISPLAY_CLOSED) {
            return RC_NOT_OPEN;
        }
        lcd_close(&file->display.lcd);
        LCD.status = DISPLAY_CLOSED;
        break;
    default:
        return RC_CLOSE_FAILED;
    }
    return RC_OK;
}

RC display_add_channel(DisplayFile *file, const char *name,
                       ChannelHandle *hdl) {
    switch (file->variant) {
    case INVALID_DISPLAY:
        return RC_INVALID_OPT;
    case TERMINAL_DISPLAY:
        return terminal_add_channel(&file->display.terminal, name, hdl);
    case LCD_DISPLAY:
        return lcd_add_channel(&file->display.lcd, name, hdl);
    }
}

RC display_remove_channel(DisplayFile *file, ChannelHandle hdl) {
    switch (file->variant) {
    case INVALID_DISPLAY:
        return RC_INVALID_OPT;
    case TERMINAL_DISPLAY:
        return terminal_remove_channel(&file->display.terminal, hdl);
    case LCD_DISPLAY:
        return lcd_remove_channel(&file->display.lcd, hdl);
    }
}

RC display_set_scale(DisplayFile *file, double scale) {
    switch (file->variant) {
    case INVALID_DISPLAY:
        return RC_INVALID_OPT;
    case TERMINAL_DISPLAY:
        return terminal_set_scale(&file->display.terminal, scale);
    case LCD_DISPLAY:
        return lcd_set_scale(&file->display.lcd, scale);
    }
}

RC display_clear(DisplayFile *file) {
    switch (file->variant) {
    case INVALID_DISPLAY:
        return RC_INVALID_OPT;
    case TERMINAL_DISPLAY:
        return terminal_clear(&file->display.terminal);
    case LCD_DISPLAY:
        return lcd_clear(&file->display.lcd);
    }
}

RC display_redraw(DisplayFile *file) {
    switch (file->variant) {
    case INVALID_DISPLAY:
        return RC_INVALID_OPT;
    case TERMINAL_DISPLAY:
        return terminal_redraw(&file->display.terminal);
    case LCD_DISPLAY:
        return lcd_redraw(&file->display.lcd);
    }
}

RC display_set_y(DisplayFile *file, usize dim) {
    switch (file->variant) {
    case INVALID_DISPLAY:
        return RC_INVALID_OPT;
    case TERMINAL_DISPLAY:
        return terminal_set_y(&file->display.terminal, dim);
    case LCD_DISPLAY:
        return lcd_set_y(&file->display.lcd, dim);
    }
}

RC display_set_x(DisplayFile *file, usize dim) {
    switch (file->variant) {
    case INVALID_DISPLAY:
        return RC_INVALID_OPT;
    case TERMINAL_DISPLAY:
        return terminal_set_x(&file->display.terminal, dim);
    case LCD_DISPLAY:
        return lcd_set_x(&file->display.lcd, dim);
    }
}

RC display_writev(DisplayFile *file, ChannelHandle hdl, double *values,
                  usize sz) {
    switch (file->variant) {
    case INVALID_DISPLAY:
        return RC_INVALID_OPT;
    case TERMINAL_DISPLAY:
        return terminal_writev(&file->display.terminal, hdl, values, sz);
    case LCD_DISPLAY:
        return lcd_writev(&file->display.lcd, hdl, values, sz);
    }
}

RC display_write(DisplayFile *file, ChannelHandle hdl, double value) {
    switch (file->variant) {
    case INVALID_DISPLAY:
        return RC_INVALID_OPT;
    case TERMINAL_DISPLAY:
        return terminal_write(&file->display.terminal, hdl, value);
    case LCD_DISPLAY:
        return lcd_write(&file->display.lcd, hdl, value);
    }
}

// common helper functions
RC add_channel(Channel *channels, usize *nchannels, const char *name,
               ChannelHandle *hdl) {
    _Bool found_opening = 0;
    usize ch, i;
    char *buf;

    // probe for open positions before incrementing channel count
    for (ch = 0; ch < *nchannels; ++ch) {
        if (!channels[ch].active) {
            found_opening = 1;
            break;
        }
    }

    // if there is no opening then add a new channel
    if (!found_opening) {
        if (*nchannels + 1 > CHANNEL_COUNT_MAX) {
            return RC_CHANNEL_COUNT;
        }
        ch = *nchannels;
    }

    // copy name into place
    buf = channels[ch].name;
    for (i = 0; i < CHANNEL_NAME_MAX; ++i) {
        char c = name[i];
        buf[i] = c;
        if (c == '\0') {
            break;
        }
    }
    if (buf[i] != '\0') {
        return RC_BUF_LENGTH;
    }
    *hdl = ch;
    channels[ch].active = 1;
    // if everything succeeded and we added a new channel, then increment val
    if (!found_opening) {
        ++(*nchannels);
    }
    return RC_OK;
}

RC remove_channel(Channel *channels, usize *nchannels, ChannelHandle hdl) {
    if (hdl >= *nchannels) {
        return RC_INVALID_OPT;
    }
    channels[hdl].active = 0;
    memset(channels[hdl].name, 0, CHANNEL_NAME_MAX);
    if (hdl == *nchannels - 1) {
        (*nchannels)--;
    }
    return RC_OK;
}

// terminal implementations
RC terminal_open(TerminalDisplay *term, DisplayFile **file) {
    if (TERMINAL.status == DISPLAY_OPEN) {
        return RC_ALREADY_OPEN;
    }
    TERMINAL.status = DISPLAY_OPEN;
    *file = &TERMINAL;
    return RC_OK;
}
RC terminal_close(TerminalDisplay *term) { return RC_OK; }
RC terminal_clear(TerminalDisplay *term) {
    printf("\033c\n");
    return RC_OK;
}
RC terminal_redraw(TerminalDisplay *term) {
    RC rc;
    rc = terminal_clear(term);
    if (rc != RC_OK) {
        return rc;
    }
    // header
    for (usize i = 0; i < term->nchannels; ++i) {
        Channel *ch = &term->channels[i];
        if (ch->active) {
            printf("%s (%.5lf) %-25s", COLORS[i], ch->last_value, ch->name);
        }
    }
    printf(RESET "\n");

    // draw axis with labels
    usize x_axis = (term->chars_tall - 1) / 2;
    for (usize i = 1; i < term->chars_tall; ++i) {
        if (i == 1) {
            printf("(%.5lfV)\n", term->scale);
        } else if (i == x_axis) {
            for (usize j = 0; j < term->chars_wide; ++j) {
                printf("*");
            }
            printf("\n");
        } else if (i == term->chars_tall - 1) {
            printf("(-%.5lfV)\n", term->scale);
        } else {
            printf("*\n");
        }
    }

    return RC_OK;
}
RC terminal_add_channel(TerminalDisplay *term, const char *name,
                        ChannelHandle *hdl) {
    return add_channel(term->channels, &term->nchannels, name, hdl);
}
RC terminal_remove_channel(TerminalDisplay *term, ChannelHandle hdl) {
    return remove_channel(term->channels, &term->nchannels, hdl);
}
RC terminal_set_scale(TerminalDisplay *term, double scale) {
    term->scale = scale;
    const usize reserved_cols = 1;
    // given a range of [-scale, scale] and one row reserved for the header,
    // determine how much of the signal each bin falls into
    term->binwidth = (2.0 * scale) / (term->chars_tall - reserved_cols);
    return RC_OK;
}
RC terminal_set_y(TerminalDisplay *term, usize chars_tall) {
    term->chars_tall = chars_tall;
    return RC_OK;
}
RC terminal_set_x(TerminalDisplay *term, usize chars_wide) {
    term->chars_wide = chars_wide;
    return RC_OK;
}
RC terminal_writev(TerminalDisplay *term, ChannelHandle hdl, double *values,
                   usize sz) {
    return RC_OK;
}
RC terminal_write(TerminalDisplay *term, ChannelHandle hdl, double value) {
    return RC_OK;
}

// lcd implementations
RC lcd_open(LcdDisplay *lcd, DisplayFile **file) {
    if (LCD.status == DISPLAY_OPEN) {
        return RC_ALREADY_OPEN;
    }
    LCD.status = DISPLAY_OPEN;
    *file = &LCD;
    return RC_OK;
}
RC lcd_close(LcdDisplay *lcd) { return RC_OK; }
RC lcd_clear(LcdDisplay *lcd) { return RC_OK; }
RC lcd_redraw(LcdDisplay *lcd) { return RC_OK; }
RC lcd_add_channel(LcdDisplay *lcd, const char *name, ChannelHandle *hdl) {
    return RC_OK;
}
RC lcd_remove_channel(LcdDisplay *lcd, ChannelHandle hdl) { return RC_OK; }
RC lcd_set_scale(LcdDisplay *lcd, double scale) { return RC_OK; }
RC lcd_set_y(LcdDisplay *lcd, usize pixels_tall) { return RC_OK; }
RC lcd_set_x(LcdDisplay *lcd, usize pixels_wide) { return RC_OK; }
RC lcd_writev(LcdDisplay *lcd, ChannelHandle hdl, double *values, usize sz) {
    return RC_OK;
}
RC lcd_write(LcdDisplay *lcd, ChannelHandle hdl, double value) { return RC_OK; }
