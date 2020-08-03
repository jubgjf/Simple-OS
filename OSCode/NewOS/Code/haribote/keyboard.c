/* 键盘控制代码 */

#include "bootpack.h"

FIFO32 *keyfifo;
int keydata0;

void inthandler21(int *esp)
/* 来自PS/2键盘的中断 */
{
    int data;
    io_out8(PIC0_OCW2, 0x61); // 通知PIC "IRQ-01"已经受理完毕
    data = io_in8(PORT_KEYDAT);
    fifo32_put(keyfifo, data + keydata0);
}

#define PORT_KEYSTA 0x0064
#define KEYSTA_SEND_NOTREADY 0x02
#define KEYCMD_WRITE_MODE 0x60
#define KBC_MODE 0x47

void wait_KBC_sendready()
{
    /* 等待键盘控制电路(KBC)准备完毕 */
    while (1)
    {
        if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0)
        {
            break;
        }
    }
}

void init_keyboard(FIFO32 *fifo, int data0)
{
    /* 将FIFO缓冲区的信息保存到全局变量里 */
    keyfifo = fifo;
    keydata0 = data0;
    /* 初始化键盘控制电路 */
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);
}
