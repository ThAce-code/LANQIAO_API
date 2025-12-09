#include "uart.h"
#include "stdio.h"
void Uart1_Init(void)	//9600bps@12.000MHz
{
	SCON = 0x50;		//8位数据,可变波特率
	AUXR |= 0x01;		//串口1选择定时器2为波特率发生器
	AUXR &= 0xFB;		//定时器时钟12T模式
	T2L = 0xE6;			//设置定时初始值
	T2H = 0xFF;			//设置定时初始值
	AUXR |= 0x10;		//定时器2开始计时
	ES = 1;				//使能串口1中断
	EA = 1;
}

/**
 * @brief 重写标准库的putchar函数
 * @param ch 要通过串口发送的单个字符
 * @return char 返回已发送的字符
 * @note  在Keil C51环境下，通过重写此函数，可以使printf函数的内容
 * 自动通过串口1发送出去，极大地方便了调试。
 */
extern char putchar (char ch)
{
    // SBUF是串行数据缓冲寄存器。向它写入数据即启动一次发送。
    SBUF = ch;
    
    // TI是SCON寄存器中的发送中断标志位。
    // 当一个字节的数据被完整发送出去后，硬件会自动将TI置1。
    // 此循环通过不断查询TI位的状态，来等待发送完成。这是一种“忙等”或“查询”方式。
    while(TI == 0)
        ;
    
    // TI标志位必须由软件清零。
    // 将TI清零，为下一次发送做准备。
    TI = 0;
    
    // 按照标准函数定义，返回发送的字符。
    return ch;
}