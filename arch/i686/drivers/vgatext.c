#include <stdint.h>
#include <stdio.h>
#include "gfx.h"
#include "io.h"
#include "panic.h"
#include "vgatext.h"

static int term_x, term_y;
static uint8_t term_col;
/* VGA buffer starts at 0xB8000 on color or 0xB0000 on monochrome */
static uint16_t *term_buf;

static uint16_t video_detect_hardware(void)
{
    const uint16_t *ptr = (const uint16_t*)0x410;
    return *ptr;
}

void vgatext_init(void)
{
    uint16_t vid_type = video_detect_hardware() & 0x30;
    if(vid_type == 0x20)
    {
        /* color */
        term_buf = (uint16_t*)0xB8000;
    }
    else if(vid_type == 0x30)
    {
        term_buf = (uint16_t*)0xB0000;
    }
    else
    {
        /* none */
        panic("VGATEXT init failed!");
    }
    vgatext_set_color(VGA_MAKE_COLOR(VGA_LIGHT_GRAY, VGA_BLACK));
    vgatext_clear();
    set_putchar(vgatext_putchar);
    set_puts(vgatext_puts);
}

static void move_cursor(uint16_t cursor_idx)
{
    outb(0x3D4, 14);
    outb(0x3D5, cursor_idx >> 8); // high byte
    outb(0x3D4, 15);
    outb(0x3D5, cursor_idx); // low byte
}

static void update_cursor(void)
{
    move_cursor(term_y * VGA_WIDTH + term_x);
}

void vgatext_clear(void)
{
    term_x = 0;
    term_y = 0;
    for(int y = 0; y < VGA_HEIGHT; ++y)
    {
        for(int x = 0; x < VGA_WIDTH; ++x)
        {
            term_buf[y * VGA_WIDTH + x] = VGA_MAKE_ENTRY(' ', term_col);
        }
    }
}

void vgatext_set_color(uint8_t color)
{
    term_col = color;
}

uint8_t vgatext_get_color(void)
{
    return term_col;
}

void vgatext_putchar_at(int ch, uint8_t col, int x, int y)
{
    term_buf[y * VGA_WIDTH + x] = VGA_MAKE_ENTRY((char)ch, col);
}

void vgatext_putchar(int ch)
{
    if(ch != '\n' && ch != '\b')
    {
        vgatext_putchar_at(ch, term_col, term_x, term_y);
        if(++term_x == VGA_WIDTH)
        {
            term_x = 0;
            if(++term_y == VGA_HEIGHT)
            {
                vgatext_clear();
                term_y = 0;
            }
        }
    }
    else if(ch == '\n')
    {
        term_x = 0;
        if(++term_y == VGA_HEIGHT)
        {
            vgatext_clear();
            term_y = 0;
        }
    }
    else if(ch == '\b')
    {
        int temp_x = term_x - 1;
        if(temp_x >= 0)
            term_x = temp_x;
        vgatext_putchar_at(' ', term_col, term_x, term_y);
    }

    update_cursor();
}

void vgatext_puts(const char *str)
{
    while(*str)
    {
        vgatext_putchar(*str++);
    }
}
