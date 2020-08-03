#include <stdio.h>
#include "../apilib.h"

void HariMain()
{
    char* buf, string[12];
    int window, timer, sec = 0, min = 0, hour = 0;
    int width = 100, height = 16;

    api_initmalloc();
    buf = api_malloc(width * height);
    window = api_openwin(buf, width, height, -1, "");
    timer = api_alloctimer();
    api_inittimer(timer, 128);

    while (1)
    {
        sprintf(string, "%5d:%02d:%02d", hour, min, sec);
        api_boxfilwin(window, 0, 0, 240, 16, 7);    // 白
        api_putstrwin(window, 0, 0, 0, 30, string);   // 黑
        api_settimer(timer, 100);   // 1秒
        if (api_getkey(1) != 128)
        {
            break;
        }
        sec++;
        if (sec == 60)
        {
            sec = 0;
            min++;
            if (min == 60)
            {
                min = 0;
                hour++;
            }
        }
    }
    api_end();
}
