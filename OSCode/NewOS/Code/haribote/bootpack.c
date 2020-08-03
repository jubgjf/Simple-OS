/* bootpack */

#include "bootpack.h"
#include <stdio.h>

#define KEYCMD_LED 0xed

void keywindow_on(SHEET* key_window);
void keywindow_off(SHEET* key_window);
void close_consolestack(TASK* task);
void close_console(SHEET* sheet);
void show_memory_usage(unsigned memtotal, MEMMANAGER* memmanager, SHEET* sheet_back);

void HariMain()
{
    BOOTINFO* bootinfo = (BOOTINFO*)ADR_BOOTINFO;
    FIFO32 fifo, keycmd;
    char string[40];
    int fifobuf[128], keycmd_buf[32];
    int cursorx = -20, cursory = -20, i, j;
    unsigned int memtotal;
    MEMMANAGER* memmanager = (MEMMANAGER*)MEMMANAGER_ADDR;
    SHEETCTRL* sheetctrl;
    static char keytable0[0x80] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, 0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0, 0, 'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', 0, 0, '\\', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0x5c, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x5c, 0, 0 };
    static char keytable1[0x80] = {
        0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0, 0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0, 0, 'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', 0, 0, '|', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, '_', 0, 0, 0, 0, 0, 0, 0, 0, 0, '|', 0, 0 };
    unsigned char* buf_back, buf_mouse[256];
    SHEET* sheet_back, * sheet_mouse;
    SHEET* sheet2, * key_window;
    TASK* task_a, * task;
    int key_shift = 0, key_leds = (bootinfo->leds >> 4) & 7, keycmd_wait = -1;

    init_gdtidt();
    init_pic();
    io_sti(); // IDT/PIC的初始化已经完成，于是开放CPU的中断
    fifo32_init(&fifo, 128, fifobuf, 0);
    *((int*)0x0fec) = (int)&fifo;
    init_pit();
    init_keyboard(&fifo, 256);
    io_out8(PIC0_IMR, 0xf8); // 开放PIT,PIC1和键盘中断(11111000)
    fifo32_init(&keycmd, 32, keycmd_buf, 0);

    memtotal = memtest(0x00400000, 0xbfffffff);
    memmanager_init(memmanager);
    memmanager_free(memmanager, 0x00001000, 0x0009e000);
    memmanager_free(memmanager, 0x00400000, memtotal - 0x00400000);

    init_palette();
    sheetctrl = sheetctrl_init(memmanager, bootinfo->vram, bootinfo->screenx, bootinfo->screeny);
    task_a = task_init(memmanager);
    fifo.task = task_a;
    task_run(task_a, 1, 2);
    *((int*)0x0fe4) = (int)sheetctrl;

    /* sheet_back */
    sheet_back = sheet_alloc(sheetctrl);
    buf_back = (unsigned char*)memmanager_alloc_4kb(memmanager, bootinfo->screenx * bootinfo->screeny);
    sheet_setbuf(sheet_back, buf_back, bootinfo->screenx, bootinfo->screeny, -1); // 无透明色
    init_screen(buf_back, bootinfo->screenx, bootinfo->screeny);

    /* sheet_console */
    key_window = open_console(sheetctrl, memtotal);

    /* sheet_mouse */
    sheet_mouse = sheet_alloc(sheetctrl);
    sheet_setbuf(sheet_mouse, buf_mouse, 16, 16, 99); // 透明色号99

    sheet_slide(sheet_back, 0, 0);
    sheet_slide(key_window, 8, 8);
    sheet_slide(sheet_mouse, cursorx, cursory);
    sheet_updown(sheet_back, 0);
    sheet_updown(key_window, 1);
    sheet_updown(sheet_mouse, 2);
    keywindow_on(key_window);

    /* 为了避免和键盘当前状态冲突，在一开始先进行设置 */
    fifo32_put(&keycmd, KEYCMD_LED);
    fifo32_put(&keycmd, key_leds);

    while (1)
    {
        show_memory_usage(memtotal, memmanager, sheet_back);

        if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0)
        {
            /* 如果存在向键盘控制器发送的数据，则发送它 */
            keycmd_wait = fifo32_get(&keycmd);
            wait_KBC_sendready();
            io_out8(PORT_KEYDAT, keycmd_wait);
        }

        io_cli(); // 屏蔽中断,专注于当前任务

        if (fifo32_status(&fifo) == 0)
        {
            task_sleep(task_a);
            io_sti();
        }
        else
        {
            i = fifo32_get(&fifo);
            io_sti();

            if (key_window != 0 && key_window->flags == 0) // 输入窗口被关闭
            {
                if (sheetctrl->top == 1) // 画面上只有鼠标
                {
                    key_window = 0;
                }
                else
                {
                    key_window = sheetctrl->sheets[sheetctrl->top - 1];
                    keywindow_on(key_window);
                }
            }

            if (i >= 256 && i <= 511) // 键盘数据
            {
                if (i < 256 + 0x80) // 将按键编码转换为字符编码
                {
                    if (key_shift == 0)
                    {
                        string[0] = keytable0[i - 256];
                    }
                    else
                    {
                        string[0] = keytable1[i - 256];
                    }
                }
                else
                {
                    string[0] = 0;
                }

                if (string[0] >= 'A' && string[0] <= 'Z') // 输入为英文字母
                {
                    if (((key_leds & 4) == 0 && key_shift == 0) || // 按下Shift, 按下CapsLock
                        ((key_leds & 4) != 0 && key_shift != 0))   // 未按Shift, 未按CapsLock
                    {
                        string[0] += 0x20; // 大写转小写
                    }
                }

                if (string[0] != 0 && key_window != 0) // 一般字符
                {
                    fifo32_put(&key_window->task->fifo, string[0] + 256);
                }
                if (i == 256 + 0x0e && key_window != 0) // Backspace
                {
                    fifo32_put(&key_window->task->fifo, 8 + 256);
                }
                if (i == 256 + 0x0f && key_window != 0) // Tab
                {
                    keywindow_off(key_window);
                    j = key_window->height - 1;
                    if (j == 0)
                    {
                        j = sheetctrl->top - 1;
                    }
                    key_window = sheetctrl->sheets[j];
                    keywindow_on(key_window);
                }
                if (i == 256 + 0x1c && key_window != 0) // Enter
                {
                    fifo32_put(&key_window->task->fifo, 10 + 256);
                }
                if (i == 256 + 0x2a) // 左 Shift ON
                {
                    key_shift |= 1;
                }
                if (i == 256 + 0x36) // 右 Shift ON
                {
                    key_shift |= 2;
                }
                if (i == 256 + 0xaa) // 左 Shift OFF
                {
                    key_shift &= ~1;
                }
                if (i == 256 + 0xb6) // 右 Shift OFF
                {
                    key_shift &= ~2;
                }
                if (i == 256 + 0xba) // CapsLock
                {
                    key_leds ^= 4;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 256 + 0x3b && key_window != 0) // F1
                {
                    task = key_window->task;
                    if (task != 0 && task->tss.ss0 != 0)
                    {
                        console_putstr0(task->console, "\nBreak(key) :\n");
                        io_cli();
                        task->tss.eax = (int)&(task->tss.esp0);
                        task->tss.eip = (int)asm_end_app;
                        io_sti();
                        task_run(task, -1, 0); // 为了执行结束处理，若处于休眠状态则唤醒
                    }
                }
                if (i == 256 + 0xfa) // 键盘成功接收到数据
                {
                    keycmd_wait = -1;
                }
                if (i == 256 + 0xfe) // 键盘没有成功接收到数据
                {
                    wait_KBC_sendready();
                    io_out8(PORT_KEYDAT, keycmd_wait);
                }
            }
            else if (i >= 768 && i <= 1023) // console关闭
            {
                close_console(sheetctrl->sheets0 + (i - 768));
            }
            else if (i >= 1024 && i <= 2023)
            {
                close_consolestack(taskctrl->tasks0 + (i - 1024));
            }
            else if (i >= 2024 && i <= 2279) // 只关闭console
            {
                sheet2 = sheetctrl->sheets0 + (i - 2024);
                memmanager_free_4kb(memmanager, (int)sheet2->buf, 256 * 165);
                sheet_free(sheet2);
            }
        }
    }
}

void keywindow_on(SHEET* key_window)
{
    if ((key_window->flags & 0x20) != 0)
    {
        fifo32_put(&key_window->task->fifo, 2); // console 光标 ON
    }
}

void keywindow_off(SHEET* key_window)
{
    if ((key_window->flags & 0x20) != 0)
    {
        fifo32_put(&key_window->task->fifo, 3); // console 光标 OFF
    }
}

TASK* open_consolestask(SHEET* sheet, unsigned int memtotal)
{
    MEMMANAGER* memmanager = (MEMMANAGER*)MEMMANAGER_ADDR;
    TASK* task = task_alloc();
    int* console_fifo = (int*)memmanager_alloc_4kb(memmanager, 128 * 4);
    task->console_stack = memmanager_alloc_4kb(memmanager, 64 * 1024);
    task->tss.esp = task->console_stack + 64 * 1024 - 12;
    task->tss.eip = (int)&console_task;
    task->tss.es = 1 * 8;
    task->tss.cs = 2 * 8;
    task->tss.ss = 1 * 8;
    task->tss.ds = 1 * 8;
    task->tss.fs = 1 * 8;
    task->tss.gs = 1 * 8;
    *((int*)(task->tss.esp + 4)) = (int)sheet;
    *((int*)(task->tss.esp + 8)) = memtotal;
    task_run(task, 2, 2); // level = 2, priority = 2
    fifo32_init(&task->fifo, 128, console_fifo, task);
    return task;
}

SHEET* open_console(SHEETCTRL* sheetctrl, unsigned int memtotal)
{
    MEMMANAGER* memmanager = (MEMMANAGER*)MEMMANAGER_ADDR;
    SHEET* sheet = sheet_alloc(sheetctrl);
    unsigned char* buf = (unsigned char*)memmanager_alloc_4kb(memmanager, CONSOLE_X * CONSOLE_Y);
    sheet_setbuf(sheet, buf, CONSOLE_X, CONSOLE_Y, -1); // 无透明色
    make_window8(buf, CONSOLE_X, CONSOLE_Y);
    make_textbox8(sheet, CONSOLE_BLANK_X, CONSOLE_BLANK_Y,
        CONSOLE_TEXT_X, CONSOLE_TEXT_Y, COL8_000000);
    sheet->task = open_consolestask(sheet, memtotal);
    sheet->flags |= 0x20; // 有光标
    return sheet;
}

void close_consolestack(TASK* task)
{
    MEMMANAGER* memmanager = (MEMMANAGER*)MEMMANAGER_ADDR;
    task_sleep(task);
    memmanager_free_4kb(memmanager, task->console_stack, 64 * 1024);
    memmanager_free_4kb(memmanager, (int)task->fifo.buf, 128 * 4);
    task->flags = 0; // 用来代替task_free(task)
}

void close_console(SHEET* sheet)
{
    /* 关闭图层 */
    MEMMANAGER* memmanager = (MEMMANAGER*)MEMMANAGER_ADDR;
    TASK* task = sheet->task;
    memmanager_free_4kb(memmanager, (int)sheet->buf, CONSOLE_X * CONSOLE_Y);
    sheet_free(sheet);
    close_consolestack(task);
}

void show_memory_usage(unsigned memtotal, MEMMANAGER* memmanager, SHEET* sheet_back)
{
    char string[25];
    sprintf(string, "total %dMB    free %dKB",
        memtotal / (1024 * 1024), memmanager_total(memmanager) / 1024);
    putfonts8_ascii_sheet(sheet_back, 824, 752,
        COL8_000000, COL8_FFFFFF, string, 25);
}
