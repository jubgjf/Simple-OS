/* 命令行窗口相关 */

#include "bootpack.h"
#include <stdio.h>
#include <string.h>

void console_task(SHEET* sheet, int memtotal)
{
    TASK* task = task_now();
    MEMMANAGER* memmanager = (MEMMANAGER*)MEMMANAGER_ADDR;
    int i, * fat = (int*)memmanager_alloc_4kb(memmanager, 4 * 2880);
    char cmdline[30];
    CONSOLE console;
    FILEHANDLE filehandle[8];

    console.sheet = sheet;
    console.cursor_x = CONSOLE_BLANK_X;
    console.cursor_y = CONSOLE_BLANK_Y;
    console.cursor_color = -1;
    task->console = &console;
    task->cmdline = cmdline;

    if (sheet != 0)
    {
        console.timer = timer_alloc();
        timer_init(console.timer, &task->fifo, 1);
        timer_settime(console.timer, 50);
    }
    file_readfat(fat, (unsigned char*)(ADR_DISKIMG + 0x000200));

    for (i = 0; i < 8; i++)
    {
        filehandle[i].buf = 0; // 未使用标记
    }
    task->filehandle = filehandle;
    task->fat = fat;

    /* 显示提示符 */
    console_putchar(&console, '>', 1);

    while (1)
    {
        io_cli();

        if (fifo32_status(&task->fifo) == 0)
        {
            task_sleep(task);
            io_sti();
        }
        else
        {
            i = fifo32_get(&task->fifo);
            io_sti();
            if (i <= 1) // 光标用定时器
            {
                if (i != 0)
                {
                    timer_init(console.timer, &task->fifo, 0); // 下次置0
                    if (console.cursor_color >= 0)
                    {
                        console.cursor_color = COL8_FFFFFF;
                    }
                }
                else
                {
                    timer_init(console.timer, &task->fifo, 1); // 下次置1
                    if (console.cursor_color >= 0)
                    {
                        console.cursor_color = COL8_000000;
                    }
                }
                timer_settime(console.timer, 50);
            }
            if (i == 2) // 光标 ON
            {
                console.cursor_color = COL8_FFFFFF;
            }
            if (i == 3) // 光标 OFF
            {
                if (console.sheet != 0)
                {
                    boxfill8(console.sheet->buf, console.sheet->bxsize, COL8_000000,
                        console.cursor_x, console.cursor_y,
                        console.cursor_x + 7, console.cursor_y + 15);
                }
                console.cursor_color = -1;
            }
            if (i == 4) // 鼠标点击console的'x'
            {
                cmd_exit(&console, fat);
            }
            if (i >= 256 && i <= 511) // 键盘数据(通过任务A)
            {
                if (i == 8 + 256) // Backspace
                {
                    if (console.cursor_x > 16)
                    {
                        /* 用空格键把光标消去，后移一次光标 */
                        console_putchar(&console, ' ', 0);
                        console.cursor_x -= 8;
                    }
                }
                else if (i == 10 + 256) // Enter
                {
                    /* 用空格将光标消除 */
                    console_putchar(&console, ' ', 0);
                    cmdline[console.cursor_x / 8 - 2] = 0;
                    console_newline(&console);
                    console_runcmd(cmdline, &console, fat, memtotal); // 运行命令
                    if (console.sheet == 0)
                    {
                        cmd_exit(&console, fat);
                    }
                    /* 显示提示符 */
                    console_putchar(&console, '>', 1);
                }
                else // 一般字符
                {
                    if (console.cursor_x < CONSOLE_TEXT_X)
                    {
                        /* 显示一个字符就前移一次光标 */
                        cmdline[console.cursor_x / 8 - 2] = i - 256;
                        console_putchar(&console, i - 256, 1);
                    }
                }
            }
            /* 重新显示光标 */
            if (console.sheet != 0)
            {
                if (console.cursor_color >= 0)
                {
                    boxfill8(console.sheet->buf, console.sheet->bxsize, console.cursor_color,
                        console.cursor_x, console.cursor_y,
                        console.cursor_x + 7, console.cursor_y + 15);
                }
                sheet_refresh(console.sheet, console.cursor_x, console.cursor_y,
                    console.cursor_x + 8, console.cursor_y + 16);
            }
        }
    }
}

void console_putchar(CONSOLE* console, int character, char move)
{
    char string[2] = { character, 0 };

    if (string[0] == 0x09) // 制表符
    {
        while (1)
        {
            if (console->sheet != 0)
            {
                putfonts8_ascii_sheet(console->sheet, console->cursor_x, console->cursor_y,
                    COL8_FFFFFF, COL8_000000, " ", 1);
            }
            console->cursor_x += 8;
            if (console->cursor_x == 8 + CONSOLE_TEXT_X)
            {
                console_newline(console);
            }
            if (((console->cursor_x - 8) & 0x1f) == 0)
            {
                break; // 被32整除则break
            }
        }
    }
    else if (string[0] == 0x0a) // 换行
    {
        console_newline(console);
    }
    else if (string[0] == 0x0d) // 回车
    {
        /* 暂时不进行操作 */
    }
    else // 一般字符
    {
        if (console->sheet != 0)
        {
            putfonts8_ascii_sheet(console->sheet, console->cursor_x, console->cursor_y,
                COL8_FFFFFF, COL8_000000, string, 1);
        }
        if (move != 0) // 光标后移
        {
            console->cursor_x += 8;
            if (console->cursor_x == 8 + CONSOLE_TEXT_X)
            {
                console_newline(console);
            }
        }
    }
}

void console_newline(CONSOLE* console)
{
    int x, y;
    SHEET* sheet = console->sheet;

    if (console->cursor_y < CONSOLE_BLANK_Y + CONSOLE_TEXT_Y - 16)
    {
        console->cursor_y += 16; // 换行
    }
    else // 滚动
    {
        if (sheet != 0)
        {
            for (y = CONSOLE_BLANK_Y; y < CONSOLE_BLANK_Y + CONSOLE_TEXT_Y - 16; y++) // 所有行上移
            {
                for (x = CONSOLE_BLANK_X; x < CONSOLE_BLANK_X + CONSOLE_TEXT_X; x++)
                {
                    sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
                }
            }
            for (y = CONSOLE_BLANK_Y + CONSOLE_TEXT_Y - 16; y < CONSOLE_BLANK_Y + CONSOLE_TEXT_Y; y++) // 最后一行涂黑
            {
                for (x = CONSOLE_BLANK_X; x < CONSOLE_BLANK_X + CONSOLE_TEXT_X; x++)
                {
                    sheet->buf[x + y * sheet->bxsize] = COL8_000000;
                }
            }
            sheet_refresh(sheet, CONSOLE_BLANK_X, CONSOLE_BLANK_Y, CONSOLE_BLANK_X + CONSOLE_TEXT_X, CONSOLE_BLANK_Y + CONSOLE_TEXT_Y);
        }
    }
    console->cursor_x = 8;
}

void console_runcmd(char* cmdline, CONSOLE* console, int* fat, int memtotal)
{
    if (strcmp(cmdline, "mem") == 0 && console->sheet != 0)
    {
        cmd_mem(console, memtotal);
    }
    else if (strcmp(cmdline, "cls") == 0 && console->sheet != 0)
    {
        cmd_cls(console);
    }
    else if (strcmp(cmdline, "dir") == 0 && console->sheet != 0)
    {
        cmd_dir(console);
    }
    else if (strcmp(cmdline, "exit") == 0)
    {
        cmd_exit(console, fat);
    }
    else if (strncmp(cmdline, "start ", 6) == 0)
    {
        cmd_start(console, cmdline, memtotal);
    }
    else if (strncmp(cmdline, "ncst ", 5) == 0)
    {
        cmd_ncst(console, cmdline, memtotal);
    }
    else if (cmdline[0] != 0)
    {
        if (cmd_app(console, fat, cmdline) == 0) // 不是命令也不是空行
        {
            console_putstr0(console, "Command not found.\n\n");
        }
    }
}

void cmd_mem(CONSOLE* console, int memtotal)
{
    MEMMANAGER* memmanager = (MEMMANAGER*)MEMMANAGER_ADDR;
    char string[60];

    sprintf(string, "total %dMB\nfree %dKB\n\n",
        memtotal / (1024 * 1024), memmanager_total(memmanager) / 1024);
    console_putstr0(console, string);
}

void cmd_cls(CONSOLE* console)
{
    int x, y;
    SHEET* sheet = console->sheet;

    for (y = CONSOLE_BLANK_Y; y < CONSOLE_BLANK_Y + CONSOLE_TEXT_Y; y++)
    {
        for (x = CONSOLE_BLANK_X; x < CONSOLE_BLANK_X + CONSOLE_TEXT_X; x++)
        {
            sheet->buf[x + y * sheet->bxsize] = COL8_000000;
        }
    }
    sheet_refresh(sheet, CONSOLE_BLANK_X, CONSOLE_BLANK_Y, CONSOLE_BLANK_X + CONSOLE_TEXT_X, CONSOLE_BLANK_Y + CONSOLE_TEXT_Y);
    console->cursor_y = CONSOLE_BLANK_Y;
}

void cmd_dir(CONSOLE* console)
{
    int i, j;
    FILEINFO* fileinfo = (FILEINFO*)(ADR_DISKIMG + 0x002600);
    char string[30];

    for (i = 0; i < 224; i++)
    {
        if (fileinfo[i].name[0] == 0x00)
        {
            break;
        }
        if (fileinfo[i].name[0] != 0xe5)
        {
            if ((fileinfo[i].type & 0x18) == 0)
            {
                sprintf(string, "filename.ext %7d\n", fileinfo[i].size);
                for (j = 0; j < 8; j++)
                {
                    string[j] = fileinfo[i].name[j];
                }
                string[9] = fileinfo[i].ext[0];
                string[10] = fileinfo[i].ext[1];
                string[11] = fileinfo[i].ext[2];
                console_putstr0(console, string);
            }
        }
    }
    console_newline(console);
}

void cmd_exit(CONSOLE* console, int* fat)
{
    MEMMANAGER* memmanager = (MEMMANAGER*)MEMMANAGER_ADDR;
    TASK* task = task_now();
    SHEETCTRL* sheetctrl = (SHEETCTRL*)*((int*)0x0fe4); // task_a
    FIFO32* fifo = (FIFO32*)*((int*)0x0fec);

    if (console->sheet != 0)
    {
        timer_cancel(console->timer); // 取消光标定时器
    }
    memmanager_free_4kb(memmanager, (int)fat, 4 * 2880);
    io_cli();
    if (console->sheet != 0)
    {
        fifo32_put(fifo, console->sheet - sheetctrl->sheets0 + 768); // 768 ~ 1023
    }
    else
    {
        fifo32_put(fifo, task - taskctrl->tasks0 + 1024); // 1024 ~ 2023
    }
    io_sti();
    while (1)
    {
        task_sleep(task);
    }
}

void cmd_start(CONSOLE* console, char* cmdline, int memtotal)
{
    SHEETCTRL* sheetctrl = (SHEETCTRL*)*((int*)0x0fe4);
    SHEET* sheet = open_console(sheetctrl, memtotal);
    FIFO32* fifo = &sheet->task->fifo;
    int i;

    sheet_slide(sheet, 32, 4);
    sheet_updown(sheet, sheetctrl->top);
    /* 将命令行输入的字符串逐字复制到新的命令行窗口中 */
    for (i = 6; cmdline[i] != 0; i++)
    {
        fifo32_put(fifo, cmdline[i] + 256);
    }
    fifo32_put(fifo, 10 + 256); // Enter
    console_newline(console);
}

void cmd_ncst(CONSOLE* console, char* cmdline, int memtotal)
{
    TASK* task = open_consolestask(0, memtotal);
    FIFO32* fifo = &task->fifo;
    int i;
    /* 将命令行输入的字符串逐字复制到新的命令行窗口中 */
    for (i = 5; cmdline[i] != 0; i++)
    {
        fifo32_put(fifo, cmdline[i] + 256);
    }
    fifo32_put(fifo, 10 + 256); // Enter
    console_newline(console);
}

int cmd_app(CONSOLE* console, int* fat, char* cmdline)
{
    MEMMANAGER* memmanager = (MEMMANAGER*)MEMMANAGER_ADDR;
    FILEINFO* fileinfo;
    char name[18], * p, * q;
    TASK* task = task_now();
    int i;
    int segsize, datasize, esp, datahrb;
    SHEET* sheet;
    SHEETCTRL* sheetctrl;

    /* 根据命令行生成文件名 */
    for (i = 0; i < 13; i++)
    {
        if (cmdline[i] <= ' ')
        {
            break;
        }
        name[i] = cmdline[i];
    }
    name[i] = 0; // 暂且将文件名的后面置为0

    /* 寻找文件 */
    fileinfo = file_search(name, (FILEINFO*)(ADR_DISKIMG + 0x002600), 224);
    if (fileinfo == 0 && name[i - 1] != '.') // 找不到文件，在文件名后面加上".hrb"重新找
    {
        name[i] = '.';
        name[i + 1] = 'H';
        name[i + 2] = 'R';
        name[i + 3] = 'B';
        name[i + 4] = 0;
        fileinfo = file_search(name, (FILEINFO*)(ADR_DISKIMG + 0x002600), 224);
    }

    if (fileinfo != 0) // 找到文件的情况
    {
        p = (char*)memmanager_alloc_4kb(memmanager, fileinfo->size);
        file_loadfile(fileinfo->clustno, fileinfo->size, p, fat, (char*)(ADR_DISKIMG + 0x003e00));
        if (fileinfo->size >= 36 && strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00)
        {
            segsize = *((int*)(p + 0x0000));
            esp = *((int*)(p + 0x000c));
            datasize = *((int*)(p + 0x0010));
            datahrb = *((int*)(p + 0x0014));
            q = (char*)memmanager_alloc_4kb(memmanager, segsize);
            task->ds_base = (int)q;
            set_segmdesc(task->ldt + 0, fileinfo->size - 1, (int)p, AR_CODE32_ER + 0x60); // 加0x60将段设置为应用程序用
            set_segmdesc(task->ldt + 1, segsize - 1, (int)q, AR_DATA32_RW + 0x60);
            for (i = 0; i < datasize; i++)
            {
                q[esp + i] = p[datahrb + i];
            }
            start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0));
            sheetctrl = (SHEETCTRL*)*((int*)0x0fe4);
            for (i = 0; i < MAX_SHEETS; i++)
            {
                sheet = &(sheetctrl->sheets0[i]);
                if ((sheet->flags & 0x11) == 0x11 && sheet->task == task)
                {
                    /* 找到被应用程序遗留的窗口 */
                    sheet_free(sheet);
                }
            }
            for (i = 0; i < 8; i++) // 将未关闭的文件关闭
            {
                if (task->filehandle[i].buf != 0)
                {
                    memmanager_free_4kb(memmanager, (int)task->filehandle[i].buf, task->filehandle[i].size);
                    task->filehandle[i].buf = 0;
                }
            }
            timer_cancelall(&task->fifo);
            memmanager_free_4kb(memmanager, (int)q, segsize);
        }
        else
        {
            console_putstr0(console, ".hrb file format error.\n");
        }
        memmanager_free_4kb(memmanager, (int)p, fileinfo->size);
        console_newline(console);
        return 1;
    }
    /* 没有找到文件的情况 */
    return 0;
}

void console_putstr0(CONSOLE* console, char* string)
{
    /* 通过识别'0'来结束显示字符串 */
    for (; *string != 0; string++)
    {
        console_putchar(console, *string, 1);
    }
}

void console_putstr1(CONSOLE* console, char* string, int len)
{
    int i;
    for (i = 0; i < len; i++)
    {
        console_putchar(console, string[i], 1);
    }
}

int* hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax)
{
    TASK* task = task_now();
    int ds_base = task->ds_base;
    CONSOLE* console = task->console;
    SHEETCTRL* sheetctrl = (SHEETCTRL*)*((int*)0x0fe4);
    SHEET* sheet;
    FIFO32* sys_fifo = (FIFO32*)*((int*)0x0fec);
    FILEINFO* fileinfo;
    FILEHANDLE* filehandle;
    MEMMANAGER* memmanager = (MEMMANAGER*)MEMMANAGER_ADDR;
    int i;
    int* reg = &eax + 1; // EAX后面的地址

    /*
        强行改写通过PUSHAD保存的值
        reg[0] : EDI
        reg[1] : ESI
        reg[2] : EBP
        reg[3] : ESP
        reg[4] : EBX
        reg[5] : EDX
        reg[6] : ECX
        reg[7] : EAX
    */

    switch (edx)
    {
    case 1:
        console_putchar(console, eax & 0xff, 1);
        break;
    case 2:
        console_putstr0(console, (char*)ebx + ds_base);
        break;
    case 3:
        console_putstr1(console, (char*)ebx + ds_base, ecx);
        break;
    case 4:
        return &(task->tss.esp0);
        break;
    case 5: // 窗口
        /*
            EDX = 5
            EBX = 窗口缓冲区
            ESI = 窗口宽度(x)
            EDI = 窗口高度(y)
            EAX = 透明色
            ECX = 窗口名称

            返回值
            EAX = 用于操作窗口的句柄
        */
        sheet = sheet_alloc(sheetctrl);
        sheet->task = task;
        sheet->flags |= 0x10;
        sheet_setbuf(sheet, (char*)ebx + ds_base, esi, edi, eax);
        make_window8((char*)ebx + ds_base, esi, edi);
        //sheet_slide(sheet, ((sheetctrl->xsize - esi) / 2) & ~3, (sheetctrl->ysize - edi) / 2);    // 窗口置于屏幕正中间
        sheet_slide(sheet, 0, TIME_WINDOW_Y); // 窗口置于屏幕底部
        sheet_updown(sheet, sheetctrl->top); // 位于鼠标图层的高度
        reg[7] = (int)sheet;
        break;
    case 6: // 字符串
        /*
            EDX = 6
            EBX = 窗口句柄
            ESI = 显示位置x坐标
            EDI = 显示位置y坐标
            EAX = 色号
            ECX = 字符串长度
            EBP = 字符串
        */
        sheet = (SHEET*)(ebx & 0xfffffffe); // 对EBX按2的倍数取整
        putfont8_ascii(sheet->buf, sheet->bxsize, esi, edi, eax, (char*)ebp + ds_base);
        if ((ebx & 1) == 0) // 取出EBX最低位(判断偶数)
        {
            sheet_refresh(sheet, esi, edi, esi + ecx * 8, edi + 16);
        }
        break;
    case 7: // 矩形
        /*
            EDX = 7
            EBX = 窗口句柄
            ESI = x0
            EDI = y0
            EAX = x1
            ECX = y1
            EBP = 色号
        */
        sheet = (SHEET*)(ebx & 0xfffffffe); // 对EBX按2的倍数取整
        boxfill8(sheet->buf, sheet->bxsize, ebp, eax, ecx, esi, edi);
        if ((ebx & 1) == 0) // 取出EBX最低位(判断偶数)
        {
            sheet_refresh(sheet, eax, ecx, esi + 1, edi + 1);
        }
        break;
    case 8: // memmanager初始化
        /*
            EDX = 8
            EBX = memmanager地址
            EAX = memmanager管理的内存空间的起始地址
            ECX = memmanager管理的内存空间的字节数
        */
        memmanager_init((MEMMANAGER*)(ebx + ds_base));
        ecx &= 0xfffffff0; // 以16字节为单位
        memmanager_free((MEMMANAGER*)(ebx + ds_base), eax, ecx);
        break;
    case 9: // malloc
        /*
            EDX = 9
            EBX = memmanager地址
            EAX = 分配到的的内存空间的地址
            ECX = 需要请求的的字节数
        */
        ecx = (ecx + 0x0f) & 0xfffffff0; // 以16字节为单位进位取整
        reg[7] = memmanager_alloc((MEMMANAGER*)(ebx + ds_base), ecx);
        break;
    case 10: // free
        /*
            EDX = 10
            EBX = memmanager地址
            EAX = 需要释放的的内存空间的地址
            ECX = 需要释放的的字节数
        */
        ecx = (ecx + 0x0f) & 0xfffffff0; // 以16字节为单位进位取整
        memmanager_free((MEMMANAGER*)(ebx + ds_base), eax, ecx);
        break;
    case 11: // 点
        /*
            EDX = 11
            EBX = 窗口句柄
            ESI = 显示位置的x坐标
            EDI = 显示位置的y坐标
            EAX = 色号
        */
        sheet = (SHEET*)(ebx & 0xfffffffe); // 对EBX按2的倍数取整
        sheet->buf[sheet->bxsize * edi + esi] = eax;
        if ((ebx & 1) == 0) // 取出EBX最低位(判断偶数)
        {
            sheet_refresh(sheet, esi, edi, esi + 1, edi + 1);
        }
        break;
    case 12: // 刷新窗口
        /*
            EDX = 12
            EBX = 窗口句柄
            EAX = x0
            ECX = y0
            ESI = x1
            EDI = y1
        */
        sheet = (SHEET*)ebx;
        sheet_refresh(sheet, eax, ecx, esi, edi);
        break;
    case 13: // 矩形
        /*
            EDX = 13
            EBX = 窗口句柄
            EAX = x0
            ECX = y0
            ESI = x1
            EDI = y1
            EBP = 色号
        */
        sheet = (SHEET*)(ebx & 0xfffffffe); // 对EBX按2的倍数取整
        hrb_api_linewin(sheet, eax, ecx, esi, edi, ebp);
        if ((ebx & 1) == 0) // 取出EBX最低位(判断偶数)
        {
            sheet_refresh(sheet, eax, ecx, esi + 1, edi + 1);
        }
        break;
    case 14: // 关闭窗口
        /*
            EDX = 14
            EBX = 窗口句柄
        */
        sheet_free((SHEET*)ebx);

        break;
    case 15: // 键盘输入
        /*
            EDX = 15
            EAX = 0 -> 没有键盘输入时返回-1,不休眠
                = 1 -> 休眠直到发生键盘输入
            EBX = 字符编码
        */
        while (1)
        {
            io_cli();
            if (fifo32_status(&task->fifo) == 0)
            {
                if (eax != 0)
                {
                    task_sleep(task); // FIFO为空，休眠并等待
                }
                else
                {
                    io_sti();
                    reg[7] = -1;
                    return 0;
                }
            }
            i = fifo32_get(&task->fifo);
            io_sti();
            if (i <= 1) // 光标用定时器
            {
                /* 应用程序运行时不需要显示光标，因此总是将下次显示用的值置为1 */
                timer_init(console->timer, &task->fifo, 1); // 下次置为1
                timer_settime(console->timer, 50);
            }
            if (i == 2) // 光标ON
            {
                console->cursor_color = COL8_FFFFFF;
            }
            if (i == 3) // 光标OFF
            {
                console->cursor_color = -1;
            }
            if (i == 4) // 只关闭console
            {
                timer_cancel(console->timer);
                io_cli();
                fifo32_put(sys_fifo, console->sheet - sheetctrl->sheets0 + 2024); // 2024 ~ 2279
                console->sheet = 0;
                io_sti();
            }
            if (i >= 256) // 键盘数据(通过任务A)等
            {
                reg[7] = i - 256;
                return 0;
            }
        }
        break;
    case 16: // 获取定时器(alloc)
        /*
            EDX = 16
            EAX = 定时器句柄(由操作系统返回)
        */
        reg[7] = (int)timer_alloc();
        ((TIMER*)reg[7])->flags2 = 1; // 允许自动取消
        break;
    case 17: // 设置定时器的发送数据(init)
        /*
            EDX = 17
            EBX = 定时器句柄
            EAX = 数据
        */
        timer_init((TIMER*)ebx, &task->fifo, eax + 256);
        break;
    case 18: // 定时器时间设定(set)
        /*
            EDX = 18
            EBX = 定时器句柄
            EAX = 时间
        */
        timer_settime((TIMER*)ebx, eax);
        break;
    case 19: // 释放定时器(free)
        /*
            EDX = 19
            EBX = 定时器句柄
        */
        timer_free((TIMER*)ebx);
        break;
    case 20: // 蜂鸣器发声
        /*
            EDX = 20
            EAX = 声音频率(mHz)
        */
        if (eax == 0)
        {
            i = io_in8(0x61);
            io_out8(0x61, i & 0x0d);
        }
        else
        {
            i = 1193180000 / eax;
            io_out8(0x43, 0xb6);
            io_out8(0x42, i & 0xff);
            io_out8(0x42, i >> 8);
            i = io_in8(0x61);
            io_out8(0x61, (i | 0x03) & 0x0f);
        }
        break;
    case 21: // 打开文件
        /*
            EDX = 21
            EBX = 文件名
            EAX = 文件句柄(为0时打开失败)(由操作系统返回)
        */
        for (i = 0; i < 8; i++)
        {
            if (task->filehandle[i].buf == 0)
            {
                break;
            }
        }
        filehandle = &task->filehandle[i];
        reg[7] = 0;
        if (i < 8)
        {
            fileinfo = file_search((char*)ebx + ds_base, (FILEINFO*)(ADR_DISKIMG + 0x002600), 224);
            if (fileinfo != 0)
            {
                reg[7] = (int)filehandle;
                filehandle->buf = (char*)memmanager_alloc_4kb(memmanager, fileinfo->size);
                filehandle->size = fileinfo->size;
                filehandle->position = 0;
                file_loadfile(fileinfo->clustno, fileinfo->size, filehandle->buf, task->fat, (char*)(ADR_DISKIMG + 0x003e00));
            }
        }
        break;
    case 22: // 关闭文件
        /*
            EDX = 22
            EAX = 文件句柄
        */
        filehandle = (FILEHANDLE*)eax;
        memmanager_free_4kb(memmanager, (int)filehandle->buf, filehandle->size);
        filehandle->buf = 0;
        break;
    case 23: // 文件定位
        /*
            EDX = 23
            EAX = 文件句柄
            ECX = 定位模式
                    = 0 : 定位起点为文件开头
                    = 1 : 定位起点为当前访问位置
                    = 2 : 定位起点为文件末尾
        */
        filehandle = (FILEHANDLE*)eax;
        if (ecx == 0)
        {
            filehandle->position = ebx;
        }
        else if (ecx == 1)
        {
            filehandle->position += ebx;
        }
        else if (ecx == 2)
        {
            filehandle->position = filehandle->size + ebx;
        }
        if (filehandle->position < 0)
        {
            filehandle->position = 0;
        }
        if (filehandle->position > filehandle->size)
        {
            filehandle->position = filehandle->size;
        }
        break;
    case 24: // 获取文件大小
        /*
            EDX = 24
            EAX = 文件句柄
            ECX = 文件大小获取模式
                    = 0 : 普通文件大小
                    = 1 : 当前读取位置从文件开头起计算的偏移量
                    = 2 : 当前读取位置从文件末尾起计算的偏移量
            EAX = 文件大小(由操作系统返回)
        */
        filehandle = (FILEHANDLE*)eax;
        if (ecx == 0)
        {
            reg[7] = filehandle->size;
        }
        else if (ecx == 1)
        {
            reg[7] = filehandle->position;
        }
        else if (ecx == 2)
        {
            reg[7] = filehandle->position - filehandle->size;
        }
        break;
    case 25: // 文件读取
        /*
            EDX = 25
            EAX = 文件句柄
            EBX = 缓冲区地址
            ECX = 最大读取的字节数
            EAX = 本次读取的字节数(由操作系统返回)
        */
        filehandle = (FILEHANDLE*)eax;
        for (i = 0; i < ecx; i++)
        {
            if (filehandle->position == filehandle->size)
            {
                break;
            }
            *((char*)ebx + ds_base + i) = filehandle->buf[filehandle->position];
            filehandle->position++;
        }
        reg[7] = i;
        break;
    case 26:
        /*
            EDX = 26
            EBX = 存放console内容的地址
            ECX = 最大存放的字节数
            EAX = 实际存放的字节数
        */
        i = 0;
        while (1)
        {
            *((char*)ebx + ds_base + i) = task->cmdline[i];
            if (task->cmdline[i] == 0)
            {
                break;
            }
            if (i >= ecx)
            {
                break;
            }
            i++;
        }
        reg[7] = i;
        break;
    }
    return 0;
}

int* inthandler0c(int* esp)
{
    TASK* task = task_now();
    CONSOLE* console = task->console;
    char string[30];

    console_putstr0(console, "\nINT 0C :\n Stack Exception.\n");
    sprintf(string, "EIP = %08X\n", esp[11]); // ESP(栈)的11号元素是EIP
    console_putstr0(console, string);
    return &(task->tss.esp0); // 强制结束程序
}

int* inthandler0d(int* esp)
{
    TASK* task = task_now();
    CONSOLE* console = task->console;
    char string[30];

    console_putstr0(console, "\nINT 0D :\n General Protected Exception.\n");
    sprintf(string, "EIP = %08X\n", esp[11]); // ESP(栈)的11号元素是EIP
    console_putstr0(console, string);
    return &(task->tss.esp0); // 强制结束程序
}

void hrb_api_linewin(SHEET* sheet, int x0, int y0, int x1, int y1, int color)
{
    int i, x, y, len, dx, dy;

    dx = x1 - x0;
    dy = y1 - y0;
    x = x0 << 10;
    y = y0 << 10;
    if (dx < 0)
    {
        dx = -dx;
    }
    if (dy < 0)
    {
        dy = -dy;
    }
    if (dx >= dy)
    {
        len = dx + 1;
        if (x0 > x1)
        {
            dx = -1024;
        }
        else
        {
            dx = 1024;
        }
        if (y0 <= y1)
        {
            dy = ((y1 - y0 + 1) << 10) / len;
        }
        else
        {
            dy = ((y1 - y0 - 1) << 10) / len;
        }
    }
    else
    {
        len = dy + 1;
        if (y0 > y1)
        {
            dy = -1024;
        }
        else
        {
            dy = 1024;
        }
        if (x0 <= x1)
        {
            dx = ((x1 - x0 + 1) << 10) / len;
        }
        else
        {
            dx = ((x1 - x0 - 1) << 10) / len;
        }
    }

    for (i = 0; i < len; i++)
    {
        sheet->buf[(y >> 10) * sheet->bxsize + (x >> 10)] = color;
        x += dx;
        y += dy;
    }
}
