/* 多任务管理 */

#include "bootpack.h"

TIMER *task_timer;
TASKCTRL *taskctrl;

TASK *task_now()
{
    /* 返回现在活动中的TASK的地址 */
    TASKLEVEL *tl = &taskctrl->level[taskctrl->now_level];
    return tl->tasks[tl->now];
}

void task_add(TASK *task)
{
    /* 向TASKLEVEL中添加一个任务 */
    TASKLEVEL *tl = &taskctrl->level[task->level];
    tl->tasks[tl->running] = task;
    tl->running++;
    task->flags = 2; // 活动中
}

void task_remove(TASK *task)
{
    /* 从TASKLEVEL中删除一个任务 */
    int i;
    TASKLEVEL *tl = &taskctrl->level[task->level];

    /*寻找task所在的位置*/
    for (i = 0; i < tl->running; i++)
    {
        if (tl->tasks[i] == task)
        {
            break; // 在这里
        }
    }

    tl->running--;
    if (i < tl->now)
    {
        tl->now--; // 需要移动成员，要相应地处理
    }
    if (tl->now >= tl->running)
    {
        /* 如果now的值出现异常，则进行修正 */
        tl->now = 0;
    }
    task->flags = 1; // 休眠中

    /* 移动 */
    for (; i < tl->running; i++)
    {
        tl->tasks[i] = tl->tasks[i + 1];
    }
}

void task_switchsub()
{
    /* 在任务切换时决定接下来切换到哪个LEVEL */
    int i;
    /* 寻找最上层的LEVEL */
    for (i = 0; i < MAX_TASKLEVELS; i++)
    {
        if (taskctrl->level[i].running > 0)
        {
            break; // 找到了
        }
    }
    taskctrl->now_level = i;
    taskctrl->level_change = 0;
}

void task_idle()
{
    while (1)
    {
        io_hlt();
    }
}

TASK *task_init(MEMMANAGER *memmanager)
{
    int i;
    TASK *task, *idle;
    SEGMENT_DESCRIPTOR *gdt = (SEGMENT_DESCRIPTOR *)ADR_GDT;
    taskctrl = (TASKCTRL *)memmanager_alloc_4kb(memmanager, sizeof(TASKCTRL));
    for (i = 0; i < MAX_TASKS; i++)
    {
        taskctrl->tasks0[i].flags = 0;
        taskctrl->tasks0[i].selector = (TASK_GDT0 + i) * 8;
        taskctrl->tasks0[i].tss.ldtr = (TASK_GDT0 + MAX_TASKS + i) * 8;
        set_segmdesc(gdt + TASK_GDT0 + i, 103, (int)&taskctrl->tasks0[i].tss, AR_TSS32);
        set_segmdesc(gdt + TASK_GDT0 + MAX_TASKS + i, 15, (int)&taskctrl->tasks0[i].ldt, AR_LDT);
    }
    for (i = 0; i < MAX_TASKLEVELS; i++)
    {
        taskctrl->level[i].running = 0;
        taskctrl->level[i].now = 0;
    }
    task = task_alloc();
    task->flags = 2;    // 活动中标志
    task->priority = 2; // 0.02秒
    task->level = 0;    // 最高LEVEL
    task_add(task);
    task_switchsub(); // LEVEL设置
    load_tr(task->selector);
    task_timer = timer_alloc();
    timer_settime(task_timer, task->priority);

    idle = task_alloc();
    idle->tss.esp = memmanager_alloc_4kb(memmanager, 64 * 1024) + 64 * 1024;
    idle->tss.eip = (int)&task_idle;
    idle->tss.es = 1 * 8;
    idle->tss.cs = 2 * 8;
    idle->tss.ss = 1 * 8;
    idle->tss.ds = 1 * 8;
    idle->tss.fs = 1 * 8;
    idle->tss.gs = 1 * 8;
    task_run(idle, MAX_TASKLEVELS - 1, 1);

    return task;
}

TASK *task_alloc()
{
    int i;
    TASK *task;
    for (i = 0; i < MAX_TASKS; i++)
    {
        if (taskctrl->tasks0[i].flags == 0)
        {
            task = &taskctrl->tasks0[i];
            task->flags = 1;               // 正在使用的标志
            task->tss.eflags = 0x00000202; // IF = 1;
            task->tss.eax = 0;             // 这里先置为0
            task->tss.ecx = 0;
            task->tss.edx = 0;
            task->tss.ebx = 0;
            task->tss.ebp = 0;
            task->tss.esi = 0;
            task->tss.edi = 0;
            task->tss.es = 0;
            task->tss.ds = 0;
            task->tss.fs = 0;
            task->tss.gs = 0;
            task->tss.iomap = 0x40000000;
            task->tss.ss0 = 0;
            return task;
        }
    }
    return 0; // 全部正在使用
}

void task_run(TASK *task, int level, int priority)
{
    if (level < 0)
    {
        level = task->level; // 不改变LEVEL
    }
    if (priority > 0)
    {
        task->priority = priority;
    }

    if (task->flags == 2 && task->level != level) // 改变活动中的LEVEL
    {
        task_remove(task);
        /* 这里执行之后flag的值会变为1，于是下边的if也会被执行 */
    }
    if (task->flags != 2) // 从休眠状态唤醒
    {
        task->level = level;
        task_add(task);
    }

    taskctrl->level_change = 1; // 下次任务切换时检查LEVEL
}

void task_switch()
{
    TASKLEVEL *tl = &taskctrl->level[taskctrl->now_level];
    TASK *new_task, *now_task = tl->tasks[tl->now];
    tl->now++;
    if (tl->now == tl->running)
    {
        tl->now = 0;
    }
    if (taskctrl->level_change != 0)
    {
        task_switchsub();
        tl = &taskctrl->level[taskctrl->now_level];
    }
    new_task = tl->tasks[tl->now];
    timer_settime(task_timer, new_task->priority);
    if (new_task != now_task)
    {
        farjmp(0, new_task->selector);
    }
}

void task_sleep(TASK *task)
{
    TASK *now_task;
    if (task->flags == 2)
    {
        /* 若处于活动状态 */
        now_task = task_now();
        task_remove(task); // 会将flag变为1
        if (task == now_task)
        {
            /* 若是让自己休眠，则需要进行任务切换 */
            task_switchsub();
            now_task = task_now(); // 在设定后获取当前任务的值
            farjmp(0, now_task->selector);
        }
    }
}
