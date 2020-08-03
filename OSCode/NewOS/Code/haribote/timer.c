/* 定时器 */

#include "bootpack.h"

#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040
#define TIMER_FLAGS_ALLOC 1 // 已配置状态
#define TIMER_FLAGS_USING 2 // 定时器运行中

TIMERCTRL timerctrl;

void init_pit()
{
    int i;
    TIMER *t;
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);
    io_out8(PIT_CNT0, 0x2e);
    timerctrl.count = 0;
    for (i = 0; i < MAX_TIMER; i++)
    {
        timerctrl.timers0[i].flags = 0; // 未使用
    }
    t = timer_alloc(); // 取得一个
    t->timeout = 0xffffffff;
    t->flags = TIMER_FLAGS_USING;
    t->next = 0;                 // 末尾
    timerctrl.t0 = t;            // 因为现在只有哨兵，所以他就在最前面
    timerctrl.next = 0xffffffff; // 因为只有哨兵，所以下一个超时时刻就是哨兵的时刻
}

TIMER *timer_alloc()
{
    int i;
    for (i = 0; i < MAX_TIMER; i++)
    {
        if (timerctrl.timers0[i].flags == 0)
        {
            timerctrl.timers0[i].flags = TIMER_FLAGS_ALLOC;
            timerctrl.timers0[i].flags2 = 0;
            return &timerctrl.timers0[i];
        }
    }
    return 0; // 没找到
}

void timer_free(TIMER *timer)
{
    timer->flags = 0; // 未使用
}

void timer_init(TIMER *timer, FIFO32 *fifo, int data)
{
    timer->fifo = fifo;
    timer->data = data;
}

void timer_settime(TIMER *timer, unsigned int timeout)
{
    int eflags;
    TIMER *t;   // 从头开始对timers[]进行遍历
    TIMER *old; // 保存前一个值
    timer->timeout = timeout + timerctrl.count;
    timer->flags = TIMER_FLAGS_USING;
    eflags = io_load_eflags();
    io_cli();
    t = timerctrl.t0;
    if (timer->timeout <= t->timeout)
    {
        /* 插入最前面的情况 */
        timerctrl.t0 = timer;
        timer->next = t; // 下面是设定t
        timerctrl.next = timer->timeout;
        io_store_eflags(eflags);
        return;
    }
    /* 搜索插入位置 */
    while (1)
    {
        old = t;
        t = t->next;
        if (timer->timeout <= t->timeout)
        {
            /* 插入old和t之间的情况 */
            old->next = timer; // old下一个是timer
            timer->next = t;   // timer的下一个是t
            io_store_eflags(eflags);
            return;
        }
    }
}

void inthandler20(int *esp)
{
    TIMER *timer;
    char ts = 0;

    io_out8(PIC0_OCW2, 0x60); // 把IRQ-00信号接收完了的信息通知给PIC
    timerctrl.count++;        // 每秒自动增加100
    if (timerctrl.next > timerctrl.count)
    {
        return; // 没到下一个时刻，所以直接结束
    }
    timer = timerctrl.t0; // 首先把最前面的地址赋给timer
    while (1)
    {
        /* 因为timers的定时器都处于运行状态，所以不确认flags */
        if (timer->timeout > timerctrl.count)
        {
            break;
        }
        /* 超时 */
        timer->flags = TIMER_FLAGS_ALLOC;
        if (timer != task_timer)
        {
            fifo32_put(timer->fifo, timer->data);
        }
        else
        {
            ts = 1; // mt_timer超时
        }
        timer = timer->next; // 将下一个定时器的地址赋给timer
    }
    timerctrl.t0 = timer;
    timerctrl.next = timer->timeout;
    if (ts != 0)
    {
        task_switch();
    }
}

int timer_cancel(TIMER *timer)
{
    int eflags;
    TIMER *t;

    eflags = io_load_eflags();
    io_cli();                              // 禁止改变定时器状态
    if (timer->flags == TIMER_FLAGS_USING) // 是否要取消
    {
        if (timer == timerctrl.t0) // 第一个定时器的取消处理
        {
            t = timer->next;
            timerctrl.t0 = t;
            timerctrl.next = t->timeout;
        }
        else // 非第一个定时器的取消处理
        {
            /* 找到timer前的一个定时器 */
            t = timerctrl.t0;
            while (1)
            {
                if (t->next == timer)
                {
                    break;
                }
                t = t->next;
            }
            t->next = timer->next;
        }
        timer->flags = TIMER_FLAGS_ALLOC;
        io_store_eflags(eflags);
        return 1; // 取消成功
    }
    io_store_eflags(eflags);
    return 0; // 不需要取消
}

void timer_cancelall(FIFO32 *fifo)
{
    int eflags, i;
    TIMER *t;

    eflags = io_load_eflags();
    io_cli();
    for (i = 0; i < MAX_TIMER; i++)
    {
        t = &timerctrl.timers0[i];
        if (t->flags != 0 && t->flags2 != 0 && t->fifo == fifo)
        {
            timer_cancel(t);
            timer_free(t);
        }
    }
    io_store_eflags(eflags);
}
