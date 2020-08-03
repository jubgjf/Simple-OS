/* FIFO */

#include "bootpack.h"

#define FLAGS_OVERRUN 0x0001

void fifo32_init(FIFO32 *fifo, int size, int *buf, TASK *task)
/* 初始化FIFO缓冲区 */
{
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size; // 剩余空间
    fifo->flags = 0;
    fifo->next_write = 0; // 写入位置
    fifo->next_read = 0;  //读取位置
    fifo->task = task;    // 有数据写入时需要唤醒的任务
}

int fifo32_put(FIFO32 *fifo, int data)
/* 向FIFO写入数据并保存 */
{
    if (fifo->free == 0) // 溢出
    {
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo->next_write] = data;
    fifo->next_write++;
    if (fifo->next_write == fifo->size)
    {
        fifo->next_write = 0;
    }
    fifo->free--;
    if (fifo->task != 0)
    {
        if (fifo->task->flags != 2) // 若任务处于休眠状态
        {
            task_run(fifo->task, -1, 0); // 将任务唤醒
        }
    }
    return 0;
}

int fifo32_get(FIFO32 *fifo)
/* 从FIFO取得一个数据 */
{
    int data;
    if (fifo->free == fifo->size)
    {
        /* 如果缓冲区为空则返回-1 */
        return -1;
    }
    data = fifo->buf[fifo->next_read];
    fifo->next_read++;
    if (fifo->next_read == fifo->size)
    {
        fifo->next_read = 0;
    }
    fifo->free++;
    return data;
    /* 
        0-1     光标闪烁定时器
        3       3秒定时器
        10      10秒定时器
        256-511 键盘输入(键盘控制器的值再加上256)
        512-767 鼠标输入(键盘控制器的值再加上512)
    */
}

int fifo32_status(FIFO32 *fifo)
/* 报告一下积攒的数据量 */
{
    return fifo->size - fifo->free;
}
