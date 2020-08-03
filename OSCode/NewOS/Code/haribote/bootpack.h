/* asmhead.nas */
typedef struct
{              /* 0x0ff0-0x0fff */
    char cyls; // 启动区读磁盘读到此为止
    char leds; // 启动时键盘的LED的状态
    /*
        leds 第4位 -> ScrollLock
        leds 第5位 -> NumLock
        leds 第6位 -> CapsLock
     */
    char vmode; // 显卡模式为多少位彩色
    char reserve;
    short screenx, screeny; // 画面分辨率
    char* vram;
} BOOTINFO;

#define ADR_BOOTINFO 0x00000ff0
#define ADR_DISKIMG 0x00100000

/* naskfunc.nas */
void io_hlt();
void io_cli();
void io_sti();
void io_stihlt();
int io_in8(int port);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
int load_cr0();
void store_cr0(int cr0);
void load_tr(int tr);
void asm_inthandler0c();
void asm_inthandler0d();
void asm_inthandler20();
void asm_inthandler21();
void asm_inthandler27();
unsigned int memtest_sub(unsigned int start, unsigned int end);
void farjmp(int eip, int cs);
void farcall(int eip, int cs);
void asm_hrb_api();
void start_app(int eip, int cs, int esp, int ds, int* tss_esp0);
void asm_end_app();

/* fifo.c */
typedef struct
{
    int* buf; // 缓冲区的地址
    int next_write, next_read;
    int size; // 缓冲区大小
    int free; // 缓冲区中没有数据的字节数
    int flags;
    struct TASK* task;
} FIFO32;

void fifo32_init(FIFO32* fifo, int size, int* buf, struct TASK* task);
int fifo32_put(FIFO32* fifo, int data);
int fifo32_get(FIFO32* fifo);
int fifo32_status(FIFO32* fifo);

/* graphic.c */
void init_palette();
void set_palette(int start, int end, unsigned char* rgb);
void boxfill8(unsigned char* vram, int xsize, unsigned char color, int x0, int y0, int x1, int y1);
void init_screen(char* vram, int xsize, int ysize);
void putfont8(char* vram, int xsize, int x, int y, char color, char* font);
void putfont8_ascii(char* vram, int xsize, int x, int y, char color, unsigned char* string);
void putblock8_8(char* vram, int vxsize, int pxsize, int pysize, int px0, int py0, char* buf, int bxsize);

#define COL8_000000 0
#define COL8_FF0000 1
#define COL8_00FF00 2
#define COL8_FFFF00 3
#define COL8_0000FF 4
#define COL8_FF00FF 5
#define COL8_00FFFF 6
#define COL8_FFFFFF 7
#define COL8_C6C6C6 8
#define COL8_840000 9
#define COL8_008400 10
#define COL8_848400 11
#define COL8_000084 12
#define COL8_840084 13
#define COL8_008484 14
#define COL8_848484 15

/* dsctbl.c */
typedef struct
{
    /* 存放GDT内容 */
    /* base:段的基址 */
    /* access_right:段属性(段的访问权限) */
    short limit_low, base_low;   // 2字节
    char base_mid, access_right; // 1字节
    char limit_high, base_high;  // 1字节
} SEGMENT_DESCRIPTOR;

typedef struct
{
    /* 存放IDT内容 */
    short offset_low, selector;
    char dw_count, access_right;
    short offset_high;
} GATE_DESCRIPTOR;

void init_gdtidt();
void set_segmdesc(SEGMENT_DESCRIPTOR* sd, unsigned int limit, int base, int ar);
void set_gatedesc(GATE_DESCRIPTOR* gd, int offset, int selector, int ar);
#define ADR_IDT 0x0026f800
#define LIMIT_IDT 0x000007ff
#define ADR_GDT 0x00270000
#define LIMIT_GDT 0x0000ffff
#define ADR_BOTPAK 0x00280000
#define LIMIT_BOTPAK 0x0007ffff
#define AR_DATA32_RW 0x4092
#define AR_CODE32_ER 0x409a
#define AR_LDT 0x0082
#define AR_TSS32 0x0089
#define AR_INTGATE32 0x008e

/* int.c */
void init_pic();
void inthandler27(int* esp);

#define PIC0_ICW1 0x0020
#define PIC0_OCW2 0x0020
#define PIC0_IMR 0x0021
#define PIC0_ICW2 0x0021
#define PIC0_ICW3 0x0021
#define PIC0_ICW4 0x0021
#define PIC1_ICW1 0x00a0
#define PIC1_OCW2 0x00a0
#define PIC1_IMR 0x00a1
#define PIC1_ICW2 0x00a1
#define PIC1_ICW3 0x00a1
#define PIC1_ICW4 0x00a1

/* keyboard.c */
void inthandler21(int* esp);
void wait_KBC_sendready();
void init_keyboard(FIFO32* fifo, int data0);
#define PORT_KEYDAT 0x0060
#define PORT_KEYCMD 0x0064

/* memory.c */
#define MEMMANAGER_FREES 4090 // 约32KB
#define MEMMANAGER_ADDR 0x003c0000

typedef struct
{
    /* 内存可用状况 */
    unsigned int addr, size;
} FREEINFO;

typedef struct
{
    /* 内存管理 */
    int frees, maxfrees, lostsize, losts;
    FREEINFO free[MEMMANAGER_FREES];
} MEMMANAGER;

unsigned int memtest(unsigned int start, unsigned int end);
void memmanager_init(MEMMANAGER* manager);
unsigned int memmanager_total(MEMMANAGER* manager);
unsigned int memmanager_alloc(MEMMANAGER* manager, unsigned int size);
int memmanager_free(MEMMANAGER* manager, unsigned int addr, unsigned int size);
unsigned int memmanager_alloc_4kb(MEMMANAGER* manager, unsigned int size);
int memmanager_free_4kb(MEMMANAGER* manager, unsigned int addr, unsigned int size);

/* sheet.c */
#define MAX_SHEETS 256

struct SHEET
{
    unsigned char* buf; // 图层所画内容的地址
    int bxsize, bysize; // 图层大小
    int vx0, vy0;       // 图层坐标
    int col_inv;        // 透明色色号
    int height;         // 图层高度
    int flags;          // 图层设定
    struct SHEETCTRL* ctrl;
    struct TASK* task;
};

struct SHEETCTRL
{
    unsigned char* vram; // vram地址
    unsigned char* map;
    int xsize, ysize; // 画面大小
    int top;          // 最上层图层高度
    struct SHEET* sheets[MAX_SHEETS];
    struct SHEET sheets0[MAX_SHEETS];
};

typedef struct SHEETCTRL SHEETCTRL;
typedef struct SHEET SHEET;

SHEETCTRL* sheetctrl_init(MEMMANAGER* memmanager, unsigned char* vram, int xsize, int ysize);
SHEET* sheet_alloc(SHEETCTRL* ctrl);
void sheet_setbuf(SHEET* sheet, unsigned char* buf, int xsize, int ysize, int col_inv);
void sheet_updown(SHEET* sheet, int height);
void sheet_refresh(SHEET* sheet, int bx0, int by0, int bx1, int by1);
void sheet_slide(SHEET* sheet, int vx0, int vy0);
void sheet_free(SHEET* sheet);
void sheet_refreshmap(SHEETCTRL* ctrl, int vx0, int vy0, int vx1, int vy1, int h0);
void sheet_refreshsub(SHEETCTRL* ctrl, int vx0, int vy0, int vx1, int vy1, int h0, int h1);

/* timer.c */
#define MAX_TIMER 500

struct TIMER
{
    char flags;
    char flags2;          // 区分定时器是否需要在应用程序结束时自动取消
    FIFO32* fifo;         // 若超时，则向fifo中发送数据
    unsigned int timeout; // 记录予定时刻
    struct TIMER* next;   // 下一个即将超时的定时器的地址(链表)
    int data;
};

typedef struct TIMER TIMER;

typedef struct
{
    unsigned int count, next;
    TIMER timers0[MAX_TIMER];
    TIMER* t0;
} TIMERCTRL;

extern TIMERCTRL timerctrl;

void init_pit();
TIMER* timer_alloc();
void timer_free(TIMER* timer);
void timer_init(TIMER* timer, FIFO32* fifo, int data);
void timer_settime(TIMER* timer, unsigned int timeout);
void inthandler20(int* esp);
int timer_cancel(TIMER* timer);
void timer_cancelall(FIFO32* fifo);

/* mtask.c */
#define MAX_TASKS 1000    // 最大任务数量
#define TASK_GDT0 3       // 定义从GDT的几号开始分配给TSS
#define MAX_TASKS_LV 100  // 每个LEVEL中最多创建的任务数
#define MAX_TASKLEVELS 10 // 总共的LEVEL数

typedef struct
{
    int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
    int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    int es, cs, ss, ds, fs, gs;
    int ldtr, iomap;
} TSS32; // 任务状态段(task status segment)

struct TASK
{
    int selector; // (选择符)存放GDT的编号
    int priority; // 优先级
    int flags;
    int level;
    FIFO32 fifo;
    TSS32 tss;
    struct CONSOLE* console;
    int ds_base;
    int console_stack;         // console专用的栈的地址
    SEGMENT_DESCRIPTOR ldt[2]; // LDT
    struct FILEHANDLE* filehandle;
    int* fat;
    char* cmdline;
};
typedef struct TASK TASK;

typedef struct
{
    int running; // 正在运行的任务数量
    int now;     // 正在运行的是哪个任务
    TASK* tasks[MAX_TASKS_LV];
} TASKLEVEL;

typedef struct
{
    int now_level;     // 现在活动中的LEVEL
    char level_change; // 下次任务切换时是否需要改变LEVEL
    TASKLEVEL level[MAX_TASKLEVELS];
    TASK tasks0[MAX_TASKS];
} TASKCTRL;

extern TIMER* task_timer;
extern TASKCTRL* taskctrl;

TASK* task_init(MEMMANAGER* memmanager);
TASK* task_alloc();
void task_run(TASK* task, int level, int priority);
void task_switch();
void task_sleep(TASK* task);
TASK* task_now();

/* window.c */
void make_window8(unsigned char* buf, int xsize, int ysize);
void putfonts8_ascii_sheet(struct SHEET* sheet, int x, int y, int string_color, int back_color, char* string, int len);
void make_textbox8(SHEET* sheet, int x0, int y0, int sx, int sy, int color);

/* console.c */
#define TIME_WINDOW_Y 752

struct CONSOLE
{
    SHEET* sheet;
    int cursor_x, cursor_y, cursor_color;
    TIMER* timer;
};

struct FILEHANDLE
{
    char* buf;
    int size;
    int position;
};

typedef struct CONSOLE CONSOLE;
typedef struct FILEHANDLE FILEHANDLE;

void console_task(SHEET* sheet, int memtotal);
void console_putchar(CONSOLE* console, int character, char move);
void console_newline(CONSOLE* console);
void console_runcmd(char* cmdline, CONSOLE* console, int* fat, int memtotal);
void cmd_mem(CONSOLE* console, int memtotal);
void cmd_cls(CONSOLE* console);
void cmd_dir(CONSOLE* console);
void cmd_exit(CONSOLE* console, int* fat);
void cmd_start(CONSOLE* console, char* cmdline, int memtotal);
void cmd_ncst(CONSOLE* console, char* cmdline, int memtotal);
int cmd_app(CONSOLE* console, int* fat, char* cmdline);
void console_putstr0(CONSOLE* console, char* string);
void console_putstr1(CONSOLE* console, char* string, int len);
int* hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax);
int* inthandler0c(int* esp);
int* inthandler0d(int* esp);
void hrb_api_linewin(SHEET* sheet, int x0, int y0, int x1, int y1, int color);

/* file.c */
typedef struct
{
    unsigned char name[8], ext[3], type;
    char reserve[10];
    unsigned short time, date;
    unsigned short clustno; // 文件从磁盘上哪个扇区开始存放
    /* 磁盘映像中的地址 = clustno * 512 + 0x004200 */
    unsigned int size;
} FILEINFO;

void file_readfat(int* fat, unsigned char* img);
void file_loadfile(int clustno, int size, char* buf, int* fat, char* img);
FILEINFO* file_search(char* name, FILEINFO* fileinfo, int max);

/* bootpack.c */
#define CONSOLE_BLANK_X 8   // console文本区距离边缘的水平距离
#define CONSOLE_BLANK_Y 8   // console文本区距离边缘的竖直距离
#define CONSOLE_TEXT_X 992  // console文本区宽度(需要是 8的倍数)
#define CONSOLE_TEXT_Y 720  // console文本区高度(需要是16的倍数)
#define CONSOLE_X (2 * CONSOLE_BLANK_X + CONSOLE_TEXT_X)  // console整体宽度
#define CONSOLE_Y (2 * CONSOLE_BLANK_Y + CONSOLE_TEXT_Y)  // console整体高度

TASK* open_consolestask(SHEET* sheet, unsigned int memtotal);
SHEET* open_console(SHEETCTRL* sheetctrl, unsigned int memtotal);
