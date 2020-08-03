/* 文件相关函数 */

#include "bootpack.h"

void file_readfat(int *fat, unsigned char *img)
{
    /* 将磁盘映像中的FAT解压缩 */
    int i, j = 0;
    for (i = 0; i < 2880; i += 2)
    {
        fat[i] = (img[j] | img[j + 1] << 8) & 0xfff;
        fat[i + 1] = (img[j + 1] >> 4 | img[j + 2] << 4) & 0xfff;
        j += 3;
    }
}

void file_loadfile(int clustno, int size, char *buf, int *fat, char *img)
{
    int i;
    while (1)
    {
        if (size <= 512)
        {
            for (i = 0; i < size; i++)
            {
                buf[i] = img[clustno * 512 + i];
            }
            break;
        }
        for (i = 0; i < 512; i++)
        {
            buf[i] = img[clustno * 512 + i];
        }

        size -= 512;
        buf += 512;
        clustno = fat[clustno];
    }
}

FILEINFO *file_search(char *name, FILEINFO *fileinfo, int max)
{
    int i, j;
    char string[12];

    /* 准备文件名 */
    for (j = 0; j < 11; j++)
    {
        string[j] = ' '; // 先空白填充string
    }
    j = 0;
    for (i = 0; name[i] != 0; i++)
    {
        if (j >= 11)
        {
            return 0; // 没有找到
        }
        if (name[i] == '.' && j <= 8) // 遇到'.'可以确定接下来的字符属于扩展名
        {
            j = 8;
        }
        else
        {
            string[j] = name[i]; // 把cmdline复制到string
            if (string[j] >= 'a' && string[j] <= 'z')
            {
                string[j] -= 0x20; // 小写转大写
            }
            j++;
        }
    }
    /* 寻找文件 */
    for (i = 0; i < max;)
    {
        if (fileinfo[i].name[0] == 0x00)
        {
            break;
        }
        if ((fileinfo[i].type & 0x18) == 0)
        {
            for (j = 0; j < 11; j++)
            {
                if (fileinfo[i].name[j] != string[j])
                {
                    goto next;
                }
            }
            return fileinfo + i; // 找到文件
        }
    next:
        i++;
    }
    return 0; // 没有找到
}
