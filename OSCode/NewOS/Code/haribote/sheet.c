/* sheet */

#include "bootpack.h"

#define SHEET_USE 1

SHEETCTRL *sheetctrl_init(MEMMANAGER *memmanager, unsigned char *vram, int xsize, int ysize)
{
    SHEETCTRL *ctrl;
    int i;
    ctrl = (SHEETCTRL *)memmanager_alloc_4kb(memmanager, sizeof(SHEETCTRL));
    if (ctrl == 0)
    {
        goto err;
    }
    ctrl->map = (unsigned char *)memmanager_alloc_4kb(memmanager, xsize * ysize);
    if (ctrl->map == 0)
    {
        memmanager_free_4kb(memmanager, (int)ctrl, sizeof(SHEETCTRL));
        goto err;
    }
    ctrl->vram = vram;
    ctrl->xsize = xsize;
    ctrl->ysize = ysize;
    ctrl->top = -1; // 没有SHEET
    for (i = 0; i < MAX_SHEETS; i++)
    {
        ctrl->sheets0[i].flags = 0;   // 标记为未使用
        ctrl->sheets0[i].ctrl = ctrl; // 记录所属
    }
err:
    return ctrl;
}

SHEET *sheet_alloc(SHEETCTRL *ctrl)
{
    SHEET *sheet;
    int i;
    for (i = 0; i < MAX_SHEETS; i++)
    {
        if (ctrl->sheets0[i].flags == 0)
        {
            sheet = &(ctrl->sheets0[i]);
            sheet->flags = SHEET_USE; // 标记为正在使用
            sheet->height = -1;       // 隐藏
            sheet->task = 0;          // 不使用自动关闭功能
            return sheet;
        }
    }
    return 0; // 所有SHEET都正在使用
}

void sheet_setbuf(SHEET *sheet, unsigned char *buf, int xsize, int ysize, int col_inv)
{
    sheet->buf = buf;
    sheet->bxsize = xsize;
    sheet->bysize = ysize;
    sheet->col_inv = col_inv;
}

void sheet_updown(SHEET *sheet, int height)
{
    SHEETCTRL *ctrl = sheet->ctrl;
    int h, old = sheet->height; /* 存储设置前的高度信息 */
    if (height > ctrl->top + 1)
    {
        height = ctrl->top + 1;
    }
    if (height < -1)
    {
        height = -1;
    }
    sheet->height = height; /* 设定高度 */

    /* 下面主要是进行sheets[]的重新排列 */
    if (old > height)
    { /* 比以前低 */
        if (height >= 0)
        {
            /* 把中间的往上提 */
            for (h = old; h > height; h--)
            {
                ctrl->sheets[h] = ctrl->sheets[h - 1];
                ctrl->sheets[h]->height = h;
            }
            ctrl->sheets[height] = sheet;
            sheet_refreshmap(ctrl, sheet->vx0, sheet->vy0, sheet->vx0 + sheet->bxsize, sheet->vy0 + sheet->bysize, height + 1);
            sheet_refreshsub(ctrl, sheet->vx0, sheet->vy0, sheet->vx0 + sheet->bxsize, sheet->vy0 + sheet->bysize, height + 1, old);
        }
        else
        { /* 隐藏 */
            if (ctrl->top > old)
            {
                /* 把上面的降下来 */
                for (h = old; h < ctrl->top; h++)
                {
                    ctrl->sheets[h] = ctrl->sheets[h + 1];
                    ctrl->sheets[h]->height = h;
                }
            }
            ctrl->top--; /* 由于显示中的图层减少了一个，所以最上面的图层高度下降 */
            sheet_refreshmap(ctrl, sheet->vx0, sheet->vy0, sheet->vx0 + sheet->bxsize, sheet->vy0 + sheet->bysize, 0);
            sheet_refreshsub(ctrl, sheet->vx0, sheet->vy0, sheet->vx0 + sheet->bxsize, sheet->vy0 + sheet->bysize, 0, old - 1);
        }
    }
    else if (old < height)
    { /* 比以前高 */
        if (old >= 0)
        {
            /* 把中间的拉下去 */
            for (h = old; h < height; h++)
            {
                ctrl->sheets[h] = ctrl->sheets[h + 1];
                ctrl->sheets[h]->height = h;
            }
            ctrl->sheets[height] = sheet;
        }
        else
        { /* 由隐藏状态转为显示状态 */
            /* 将已在上面的提上来 */
            for (h = ctrl->top; h >= height; h--)
            {
                ctrl->sheets[h + 1] = ctrl->sheets[h];
                ctrl->sheets[h + 1]->height = h + 1;
            }
            ctrl->sheets[height] = sheet;
            ctrl->top++; /* 由于已显示的图层增加了1个，所以最上面的图层高度增加 */
        }
        sheet_refreshmap(ctrl, sheet->vx0, sheet->vy0, sheet->vx0 + sheet->bxsize, sheet->vy0 + sheet->bysize, height);
        sheet_refreshsub(ctrl, sheet->vx0, sheet->vy0, sheet->vx0 + sheet->bxsize, sheet->vy0 + sheet->bysize, height, height); /* 按新图层信息重新绘制画面 */
    }
}

void sheet_refresh(SHEET *sheet, int bx0, int by0, int bx1, int by1)
{
    if (sheet->height >= 0)
    { /* 如果正在显示，则按新图层的信息刷新画面*/
        sheet_refreshsub(sheet->ctrl, sheet->vx0 + bx0, sheet->vy0 + by0, sheet->vx0 + bx1, sheet->vy0 + by1, sheet->height, sheet->height);
    }
}

void sheet_refreshsub(SHEETCTRL *ctrl, int vx0, int vy0, int vx1, int vy1, int h0, int h1)
{
    int h, bx, by, vx, vy, bx0, by0, bx1, by1, bx2, sheet_id4, i, i1, *p, *q, *r;
    unsigned char *buf, *vram = ctrl->vram, *map = ctrl->map, sheet_id;
    SHEET *sheet;

    /* 如果refresh的范围超出了画面则修正 */
    if (vx0 < 0)
    {
        vx0 = 0;
    }
    if (vy0 < 0)
    {
        vy0 = 0;
    }
    if (vx1 > ctrl->xsize)
    {
        vx1 = ctrl->xsize;
    }
    if (vy1 > ctrl->ysize)
    {
        vy1 = ctrl->ysize;
    }
    for (h = h0; h <= h1; h++)
    {
        sheet = ctrl->sheets[h];
        buf = sheet->buf;
        sheet_id = sheet - ctrl->sheets0;

        /* 使用vx0～vy1，对bx0～by1进行倒推 */
        bx0 = vx0 - sheet->vx0;
        by0 = vy0 - sheet->vy0;
        bx1 = vx1 - sheet->vx0;
        by1 = vy1 - sheet->vy0;
        if (bx0 < 0)
        {
            bx0 = 0;
        } /* 处理刷新范围在图层外侧 */
        if (by0 < 0)
        {
            by0 = 0;
        }
        if (bx1 > sheet->bxsize)
        {
            bx1 = sheet->bxsize;
        } /* 应对不同的重叠方式 */
        if (by1 > sheet->bysize)
        {
            by1 = sheet->bysize;
        }
        if ((sheet->vx0 & 3) == 0)
        {
            /* 4字节型 */
            i = (bx0 + 3) / 4; // bx0除以4(小数进位)
            i1 = bx1 / 4;      // bx1除以4(小数舍去)
            i1 = i1 - i;
            sheet_id4 = sheet_id | sheet_id << 8 | sheet_id << 16 | sheet_id << 24;
            for (by = by0; by < by1; by++)
            {
                vy = sheet->vy0 + by;
                for (bx = bx0; bx < bx1 && (bx & 3) != 0; bx++)
                {
                    /* 前面被4除多余的部分逐个字节写入 */
                    vx = sheet->vx0 + bx;
                    if (map[vy * ctrl->xsize + vx] == sheet_id)
                    {
                        vram[vy * ctrl->xsize + vx] = buf[by * sheet->bxsize + bx];
                    }
                }
                vx = sheet->vx0 + bx;
                p = (int *)&map[vy * ctrl->xsize + vx];
                q = (int *)&vram[vy * ctrl->xsize + vx];
                r = (int *)&buf[by * sheet->bxsize + bx];
                for (i = 0; i < i1; i++)
                {
                    /* 4的倍数部分 */
                    if (p[i] == sheet_id4)
                    {
                        q[i] = r[i]; // 估计大多数会是这种情况，因此速度会变快
                    }
                    else
                    {
                        bx2 = bx + i * 4;
                        vx = sheet->vx0 + bx2;
                        if (map[vy * ctrl->xsize + vx + 0] == sheet_id)
                        {
                            vram[vy * ctrl->xsize + vx + 0] = buf[by * sheet->bxsize + bx2 + 0];
                        }
                        if (map[vy * ctrl->xsize + vx + 1] == sheet_id)
                        {
                            vram[vy * ctrl->xsize + vx + 1] = buf[by * sheet->bxsize + bx2 + 1];
                        }
                        if (map[vy * ctrl->xsize + vx + 2] == sheet_id)
                        {
                            vram[vy * ctrl->xsize + vx + 2] = buf[by * sheet->bxsize + bx2 + 2];
                        }
                        if (map[vy * ctrl->xsize + vx + 3] == sheet_id)
                        {
                            vram[vy * ctrl->xsize + vx + 3] = buf[by * sheet->bxsize + bx2 + 3];
                        }
                    }
                }
                for (bx += i1 * 4; bx < bx1; bx++)
                {
                    /* 后面被4除多余的部分逐个字节写入 */
                    vx = sheet->vx0 + bx;
                    if (map[vy * ctrl->xsize + vx] == sheet_id)
                    {
                        vram[vy * ctrl->xsize + vx] = buf[by * sheet->bxsize + bx];
                    }
                }
            }
        }
        else
        {
            /* 1字节型 */
            for (by = by0; by < by1; by++)
            {
                vy = sheet->vy0 + by;
                for (bx = bx0; bx < bx1; bx++)
                {
                    vx = sheet->vx0 + bx;
                    if (map[vy * ctrl->xsize + vx] == sheet_id)
                    {
                        vram[vy * ctrl->xsize + vx] = buf[by * sheet->bxsize + bx];
                    }
                }
            }
        }
    }
}

void sheet_refreshmap(SHEETCTRL *ctrl, int vx0, int vy0, int vx1, int vy1, int h0)
{
    int h, bx, by, vx, vy, bx0, by0, bx1, by1, sheet_id4, *p;
    unsigned char *buf, sheet_id, *map = ctrl->map;
    SHEET *sheet;

    if (vx0 < 0)
    {
        vx0 = 0;
    }
    if (vy0 < 0)
    {
        vy0 = 0;
    }
    if (vx1 > ctrl->xsize)
    {
        vx1 = ctrl->xsize;
    }
    if (vy1 > ctrl->ysize)
    {
        vy1 = ctrl->ysize;
    }

    for (h = h0; h <= ctrl->top; h++)
    {
        sheet = ctrl->sheets[h];
        sheet_id = sheet - ctrl->sheets0; /* 将进行了减法计算的地址作为图层号码使用 */
        buf = sheet->buf;
        bx0 = vx0 - sheet->vx0;
        by0 = vy0 - sheet->vy0;
        bx1 = vx1 - sheet->vx0;
        by1 = vy1 - sheet->vy0;
        if (bx0 < 0)
        {
            bx0 = 0;
        }
        if (by0 < 0)
        {
            by0 = 0;
        }
        if (bx1 > sheet->bxsize)
        {
            bx1 = sheet->bxsize;
        }
        if (by1 > sheet->bysize)
        {
            by1 = sheet->bysize;
        }

        if (sheet->col_inv == -1)
        {
            if ((sheet->vx0 & 3) == 0 && (bx0 & 3) == 0 && (bx1 & 3) == 0) // 无透明色图层专用的高速版(4字节型)
            {
                bx1 = (bx1 - bx0) / 4; // MOV次数
                sheet_id4 = sheet_id | sheet_id << 8 | sheet_id << 16 | sheet_id << 24;
                for (by = by0; by < by1; by++)
                {
                    vy = sheet->vy0 + by;
                    vx = sheet->vx0 + bx0;
                    p = (int *)&map[vy * ctrl->xsize + vx];
                    for (bx = 0; bx < bx1; bx++)
                    {
                        p[bx] = sheet_id4;
                    }
                }
            }
            else // 无透明色图层专用的高速版(1字节型)
            {
                for (by = by0; by < by1; by++)
                {
                    vy = sheet->vy0 + by;
                    for (bx = bx0; bx < bx1; bx++)
                    {
                        vx = sheet->vx0 + bx;
                        map[vy * ctrl->xsize + vx] = sheet_id;
                    }
                }
            }
        }
        else // 有透明色图层用的普通版
        {
            for (by = by0; by < by1; by++)
            {
                vy = sheet->vy0 + by;
                for (bx = bx0; bx < bx1; bx++)
                {
                    vx = sheet->vx0 + bx;
                    if (buf[by * sheet->bxsize + bx] != sheet->col_inv)
                    {
                        map[vy * ctrl->xsize + vx] = sheet_id;
                    }
                }
            }
        }
    }
}

void sheet_slide(SHEET *sheet, int vx0, int vy0)
{
    SHEETCTRL *ctrl = sheet->ctrl;
    int old_vx0 = sheet->vx0, old_vy0 = sheet->vy0;
    sheet->vx0 = vx0;
    sheet->vy0 = vy0;
    if (sheet->height >= 0)
    { /* 如果正在显示，则按新图层的信息刷新画面 */
        sheet_refreshmap(ctrl, old_vx0, old_vy0, old_vx0 + sheet->bxsize, old_vy0 + sheet->bysize, 0);
        sheet_refreshmap(ctrl, vx0, vy0, vx0 + sheet->bxsize, vy0 + sheet->bysize, sheet->height);
        sheet_refreshsub(ctrl, old_vx0, old_vy0, old_vx0 + sheet->bxsize, old_vy0 + sheet->bysize, 0, sheet->height - 1);
        sheet_refreshsub(ctrl, vx0, vy0, vx0 + sheet->bxsize, vy0 + sheet->bysize, sheet->height, sheet->height);
    }
}

void sheet_free(SHEET *sheet)
{
    if (sheet->height >= 0)
    {
        sheet_updown(sheet, -1); /* 如果处于显示状态，则先设定为隐藏 */
    }
    sheet->flags = 0; /* "未使用"标志 */
}
