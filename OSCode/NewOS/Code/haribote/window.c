/* 窗口相关函数 */

#include "bootpack.h"

void make_window8(unsigned char* buf, int xsize, int ysize)
{
    boxfill8(buf, xsize, COL8_FFFFFF, 0, 0, xsize - 1, ysize - 1);
}

void putfonts8_ascii_sheet(SHEET* sheet, int x, int y, int string_color, int back_color, char* string, int len)
{
    boxfill8(sheet->buf, sheet->bxsize, back_color, x, y, x + len * 8 - 1, y + 15);
    putfont8_ascii(sheet->buf, sheet->bxsize, x, y, string_color, string);
    sheet_refresh(sheet, x, y, x + len * 8, y + 16);
}

void make_textbox8(SHEET* sheet, int x0, int y0, int sx, int sy, int color)
{
    boxfill8(sheet->buf, sheet->bxsize, color, x0 - 1, y0 - 1, x0 + sx, y0 + sy);
}
