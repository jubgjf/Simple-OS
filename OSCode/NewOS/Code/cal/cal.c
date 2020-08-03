#include "../apilib.h"
#include "math.h"
#include "stack_char.h"
#include "stack_double.h"
#include <stdio.h>

#define stackLen 20
int stack[stackLen];
int stackTop = 0;

int IsRPN(char* cmdline)
{
    int i = 0;
    while (cmdline[i + 1] != '\0') { i++; }
    if (cmdline[i] == '=')
    {
        return 1;
    }
    return 0;
}

void AddEqualSign(char* pre)
{
    int i = 0;
    while (pre[i + 1] != '\0') { i++; }
    if (pre[i] == '=')
    {
        return;
    }
    else
    {
        pre[i + 2] = '\0';
        pre[i + 1] = '=';
    }
}

int IsOperator(char c)
{
    switch (c)
    {
    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
        return 1;
    default:
        return 0;
    }
}

int OperatorPriority(char c)
{
    switch (c)
    {
    case '*':
    case '/':
    case '%':
        return 2;
    case '+':
    case '-':
        return 1;
    case '(':
        return 0;
    default:
        return -1;
    }
}

void Postfix(char* pre, char post[], int* len)
{
    int i = 0, j = 0;
    StackChar* stack = (StackChar*)api_malloc(sizeof(StackChar));
    PushChar(stack, '=');
    while (pre[i] != '=')
    {
        if (pre[i] == '(')
        {
            PushChar(stack, pre[i]);
        }
        else if ((pre[i] >= '0' && pre[i] <= '9') || pre[i] == '.')
        {
            post[j++] = pre[i];
        }
        else if (pre[i] == ')')
        {
            while (TopChar(stack) != '(')
            {
                post[j++] = TopChar(stack);
                PopChar(stack);
            }
            PopChar(stack);
        }
        else if (IsOperator(pre[i]))
        {
            post[j++] = ' ';
            while (OperatorPriority(pre[i]) <= OperatorPriority((TopChar(stack))))
            {
                post[j++] = TopChar(stack);
                PopChar(stack);
            }
            PushChar(stack, pre[i]);
        }
        i++;
    }
    while (TopChar(stack) != '=')
    {
        post[j++] = TopChar(stack);
        PopChar(stack);
    }
    post[j] = '=';
    *len = j;
}

double ReadNum(char* post, int* i)
{
    double sum = 0;
    int k = 0;

    while (post[*i] >= '0' && post[*i] <= '9')
    {
        sum = sum * 10 + post[*i] - '0';
        (*i)++;
    }
    if (post[*i] == '.')
    {
        (*i)++;
        while (post[*i] >= '0' && post[*i] <= '9')
        {
            sum = sum * 10 + post[*i] - '0';
            (*i)++;
            k++;
        }
    }
    if (k != 0)
    {
        sum /= pow(10, k);
    }
    api_putstr0("return ");
    return sum;
}

double PostValue(char post[])
{
    StackDouble* stack = (StackDouble*)api_malloc(sizeof(StackDouble));
    double x1, x2;
    int i = 0;
    while (post[i] != '=')
    {
        if (post[i] >= '0' && post[i] <= '9')
        {
            PushDouble(stack, ReadNum(post, &i));
        }
        else if (post[i] == ' ')
        {
            i++;
        }
        else if (post[i] == '+')
        {
            x2 = TopDouble(stack);
            PopDouble(stack);
            x1 = TopDouble(stack);
            PopDouble(stack);
            PushDouble(stack, x1 + x2);
            i++;
        }
        else if (post[i] == '-')
        {
            x2 = TopDouble(stack);
            PopDouble(stack);
            x1 = TopDouble(stack);
            PopDouble(stack);
            PushDouble(stack, x1 - x2);
            i++;
        }
        else if (post[i] == '*')
        {
            x2 = TopDouble(stack);
            PopDouble(stack);
            x1 = TopDouble(stack);
            PopDouble(stack);
            PushDouble(stack, x1 * x2);
            i++;
        }
        else if (post[i] == '/')
        {
            x2 = TopDouble(stack);
            PopDouble(stack);
            x1 = TopDouble(stack);
            PopDouble(stack);
            PushDouble(stack, x1 / x2);
            i++;
        }
        else if (post[i] == '%')
        {
            x2 = TopDouble(stack);
            PopDouble(stack);
            x1 = TopDouble(stack);
            PopDouble(stack);
            PushDouble(stack, (int)x1 % (int)x2);
            i++;
        }
    }
    return TopDouble(stack);
}

int HariMain()
{
    int len = 0;
    char post[100];
    char result[100];
    char cmdline[50], * p;
    int i;
    int x1, x2, resultNum;

    api_cmdline(cmdline, 50);
    for (p = cmdline; *p != ' '; p++) {}
    for (; *p == ' '; p++) {}

    api_initmalloc();

    if (IsRPN(p))
    {
    calRPN:
        for (i = 0; p[i] != '='; i++)
        {
            switch (p[i])
            {
            case '+':
                x1 = stack[stackTop];
                x2 = stack[stackTop - 1];
                resultNum = x1 + x2;
                stack[stackTop - 1] = resultNum;
                stackTop -= 1;
                break;
            case '-':
                x1 = stack[stackTop];
                x2 = stack[stackTop - 1];
                resultNum = x2 - x1;
                stack[stackTop - 1] = resultNum;
                stackTop -= 1;
                break;
            case '*':
                x1 = stack[stackTop];
                x2 = stack[stackTop - 1];
                resultNum = x1 * x2;
                stack[stackTop - 1] = resultNum;
                stackTop -= 1;
                break;
            case '/':
                x1 = stack[stackTop];
                x2 = stack[stackTop - 1];
                resultNum = x2 / x1;
                stack[stackTop - 1] = resultNum;
                stackTop -= 1;
                break;
            case ' ':
                break;
            default:
                stack[stackTop + 1] = p[i] - '0';   // 只可以接受个位数的输入
                stackTop += 1;
                break;
            }
        }
        sprintf(result, "result = %d", resultNum);    // 输出结果
    }
    else
    {
        AddEqualSign(p);
        Postfix(p, post, &len);
        sprintf(result, "RPN : %s\n", post);
        api_putstr0(result);    // 输出逆波兰
        p = result;
        goto calRPN;
    }

    api_putstr0(result);
    api_end();
}
