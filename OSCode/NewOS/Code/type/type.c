#include "../apilib.h"

void HariMain()
{
    int filehandle;
    char c, cmdline[30], *p;

    api_cmdline(cmdline, 30);
    for (p = cmdline; *p > ' '; p++)
    {
        // 跳过之前的内容，直到遇到空格
    }
    for (; *p == ' '; p++)
    {
        // 跳过空格 
    }
    filehandle = api_fopen(p);
    if (filehandle != 0)
    {
        while (1)
        {
            if (api_fread(&c, 1, filehandle) == 0)
            {
                break;
            }
            api_putchar(c);
        }
    }
    else
    {
        api_putstr0("File not found.\n");
    }
    api_end();
}
