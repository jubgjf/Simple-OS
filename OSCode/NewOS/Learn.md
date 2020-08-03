# day_01

- 软盘总容量1440KB，2880个扇区，每个扇区512字节

- 1字节 =
  - 一个英文字母
  - 8位2进制数
  - 2位16进制数

DB|word|DW
:-:|:-:|:-:
8位|16位|32位
1字节|2字节|4字节
data byte||double word

- `RESB x` 预留x个字节为0x00

# day_02

- AX 累加寄存器
- BX 基址寄存器
- CX 计数寄存器
- DX 数据寄存器
- SP 栈指针寄存器
- BP 基址指针寄存器
- SI 源变址寄存器
- DI 目的变址寄存器

- 位寄存器  
  - EAX 一共32位
  - AX  EAX的后16位
  - AH  AX的前8位
  - AL  AX的后8位

- 段寄存器
  - ES 附加段寄存器
  - CS 代码段寄存器
  - SS 栈段寄存器
  - DS 数据段寄存器
  - FS 无名
  - GS 无名

```
MOV BYTE [678], 123     ;用内存678号地址保存123这个8位数
MOV WORD [678], 123     ;用内存678和679号保存123这个16位数，678保存低8位，679保存高8位
```

- 启动区内容的装载地址`0x00007c00~0x00007dff` 一共512字节

- C语言函数参数

> ```c
> write_memory(int addr, int data)
> ```
> 第1个参数存放地址为 [ESP + 4]  
> 第2个参数存放地址为 [ESP + 8]  
> 第3个参数存放地址为 [ESP + 16]  
> ......  

# day_04

- `char`->`BYTE`类型->`AL`
- `short`->`WORD`类型->`AX`
- `int`->`DWORD`类型->`EAX`

```
CLI     ;禁止中断
STI     ;恢复中断
```

- `PUSHFD`将EFLAGS的值按照double-word压入栈(相当于PUSH EFLAGS)
- `POP EAX`从栈中弹出的值存入EAX

- 320x200画面，像素坐标(x, y)对应VRAM地址:`0xa0000 + x + y * 320`

# day_05

- `0x00`和`\0`在此代码中作用相同

```c
for (; *string != 0x00; string++) // *string != '\0' 也行
{
    putfont8(vram, xsize, x, y, color, font + *string * 16);
    x += 8; // 一个字符的宽
}
```

- `%x`16进制数

# day_09

- `~`取反运算符,将所有位反转

# day_12

- `unsigned char`类型用`%u`输出

# day_15

- `TR`寄存器:记住当前正在执行哪一个任务