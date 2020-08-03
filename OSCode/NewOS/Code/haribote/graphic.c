/* 关于绘图部分的处理 */

#include "bootpack.h"

void init_palette()
{
    static unsigned char table_rgb[16 * 3] = {
        0x00, 0x00, 0x00, /*  0:黑 */
        0xff, 0x00, 0x00, /*  1:梁红 */
        0x00, 0xff, 0x00, /*  2:亮绿 */
        0xff, 0xff, 0x00, /*  3:亮黄 */
        0x00, 0x00, 0xff, /*  4:亮蓝 */
        0xff, 0x00, 0xff, /*  5:亮紫 */
        0x00, 0xff, 0xff, /*  6:浅亮蓝 */
        0xff, 0xff, 0xff, /*  7:白 */
        0xc6, 0xc6, 0xc6, /*  8:亮灰 */
        0x84, 0x00, 0x00, /*  9:暗红 */
        0x00, 0x84, 0x00, /* 10:暗绿 */
        0x84, 0x84, 0x00, /* 11:暗黄 */
        0x00, 0x00, 0x84, /* 12:暗青 */
        0x84, 0x00, 0x84, /* 13:暗紫 */
        0x00, 0x84, 0x84, /* 14:浅暗蓝 */
        0x84, 0x84, 0x84  /* 15:暗灰 */
    };
    unsigned char table2[216 * 3]; // 216 = 6 * 6 * 6
    int r, g, b;

    set_palette(0, 15, table_rgb);
    /* r,g,b每种颜色赋予6个色阶，定义216种颜色 */
    for (b = 0; b < 6; b++)
    {
        for (g = 0; g < 6; g++)
        {
            for (r = 0; r < 6; r++)
            {
                /* 色号 = 16 + 1 * r / 51 + 6 * g / 51 + 36 * b / 51 */
                table2[(r + g * 6 + b * 36) * 3 + 0] = r * 51;
                table2[(r + g * 6 + b * 36) * 3 + 1] = g * 51;
                table2[(r + g * 6 + b * 36) * 3 + 2] = b * 51;
            }
        }
    }
    set_palette(16, 231, table2);
}

void set_palette(int start, int end, unsigned char* rgb)
{
    int i, eflags;
    eflags = io_load_eflags(); /* 记录中断许可标志的值 */
    io_cli();                  /* 将中断许可标志置为0,禁止中断 */
    io_out8(0x03c8, start);
    for (i = start; i <= end; i++)
    {
        io_out8(0x03c9, rgb[0] / 4);
        io_out8(0x03c9, rgb[1] / 4);
        io_out8(0x03c9, rgb[2] / 4);
        rgb += 3;
    }
    io_store_eflags(eflags); /* 复原中断许可标志 */
}

void boxfill8(unsigned char* vram, int xsize, unsigned char color,
    int x0, int y0, int x1, int y1)
{
    int x, y;
    for (y = y0; y <= y1; y++)
    {
        for (x = x0; x <= x1; x++)
            vram[y * xsize + x] = color;
    }
}

void init_screen(char* vram, int xsize, int ysize)
{
    /* 根据 0xa0000 + x + y * 320 计算坐标*/
    boxfill8(vram, xsize, COL8_000084, 0, 0, xsize, ysize);           // 背景
}

void putfont8(char* vram, int xsize, int x, int y, char color, char* font)
{
    int i;
    char* p, d /* data */;
    for (i = 0; i < 16; i++)
    {
        p = vram + (y + i) * xsize + x;
        d = font[i];
        if ((d & 0x80) != 0)
        {
            p[0] = color;
        }
        if ((d & 0x40) != 0)
        {
            p[1] = color;
        }
        if ((d & 0x20) != 0)
        {
            p[2] = color;
        }
        if ((d & 0x10) != 0)
        {
            p[3] = color;
        }
        if ((d & 0x08) != 0)
        {
            p[4] = color;
        }
        if ((d & 0x04) != 0)
        {
            p[5] = color;
        }
        if ((d & 0x02) != 0)
        {
            p[6] = color;
        }
        if ((d & 0x01) != 0)
        {
            p[7] = color;
        }
    }
}

void putfont8_ascii(char* vram, int xsize, int x, int y, char color, unsigned char* string)
{
    extern char font[4096];
    for (; *string != '\0'; string++)
    {
        putfont8(vram, xsize, x, y, color, font + *string * 16);
        x += 8; //一个字符的宽
    }
}

void putblock8_8(char* vram, int vxsize, int pxsize,
    int pysize, int px0, int py0, char* buf, int bxsize)
{
    int x, y;
    for (y = 0; y < pysize; y++)
    {
        for (x = 0; x < pxsize; x++)
        {
            vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
        }
    }
}
