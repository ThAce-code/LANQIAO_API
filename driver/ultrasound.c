#include "ultrasound.h" // 包含了超声波模块相关的函数声明
#include "intrins.h"    // 包含了Keil C51的内部函数，如_nop_()

sbit US_TX = P1^0; // 定义超声波模块的触发(Trig)引脚，连接到P1.0
sbit US_RX = P1^1; // 定义超声波模块的回波(Echo)引脚，连接到P1.1

// 部分开发板使用下面的短延时可能会跳，可以使用这个
//void Delay12us(void)  //@12.000MHz
//{
//  unsigned char data i;
//
//  _nop_();
//  _nop_();
//  i = 33;//38
//  while (--i);
//}

/**
 * @brief  一个短延时函数
 * @note   这是一个基于特定时钟频率(12.000MHz)校准的软件延时。
 * 其精确的延时时间对超声波模块的触发信号至关重要。
 */
void Delay12us(void)    //@12.000MHz
{
    unsigned char data i;

    _nop_(); // 空指令，用于微调延时
    i = 3;
    while (--i);
}

/**
 * @brief  产生超声波模块所需的触发信号
 * @note   此函数通过循环8次，产生一串40kHz的脉冲波形作为触发信号，
 */
void Ut_Wave_Init()
{
    unsigned char i;
    EA = 0; // 关总中断，以保证时序的精确性，防止在产生脉冲时被中断打断
    for(i=0; i<8; i++)
    {
        US_TX = 1;      // 将触发引脚拉高
        Delay12us();    // 延时
        US_TX = 0;      // 将触发引脚拉低
        Delay12us();    // 延时
    }
    EA = 1; // 恢复总中断
}

// --- 距离计算原理 ---
// 声速 V = 340 m/s = 34000 cm/s = 0.034 cm/us
// 测得的时间 T 是超声波来回的总时间
// 距离 S = (T * V) / 2 = T * (0.034 / 2) = T * 0.017 cm
// 其中T的单位是微秒(us)，S的单位是厘米(cm)。

/**
 * @brief  执行一次完整的超声波测距
 * @return unsigned char - 返回测量到的距离（单位：厘米）。
 * 如果测量超时（溢出），则返回0。
 * @note   返回值是unsigned char，意味着最大可报告距离为255cm。
 */
unsigned char Ut_Wave_Data()
{
    unsigned int time; // 用于存储回波高电平的持续时间（计时器计数值）
    
    // 配置单片机内部的PCA（可编程计数器阵列）作为高精度计时器
    // CMOD = 0x00: 设置PCA的时钟源为系统时钟的12分频 (在12MHz下，每1us计数一次)
    CMOD = 0x00;
    // 清空PCA计数器
    CH = CL = 0;
    
    // 发送超声波触发脉冲
    Ut_Wave_Init();
    
    // CR是PCA的运行控制位。置1后，PCA计数器开始计数。
    CR = 1;
    
    // 等待，直到接收到回波（US_RX变为低电平）或计时器溢出
    // CF是PCA的溢出标志位，如果计数器从0xFFFF变为0x0000，CF会被硬件置1
    while((US_RX == 1) && (CF == 0));
    
    // 停止PCA计时器
    CR = 0;
    
    // 判断循环退出的原因
    if(CF == 0) // 如果是没有溢出（即成功接收到回波）
    {
        // 读取PCA计数器的16位值
        time = CH << 8 | CL;
        // 应用公式计算距离。time的值就是us数。
        // 注意：结果(time * 0.017)是一个浮点数，但被强制转换为unsigned char返回。
        return (time * 0.017);
    }
    else // 如果是计时器溢出（距离太远或无障碍物）
    {
        CF = 0; // 清除溢出标志，为下一次测量做准备
        return 0; // 返回0表示测量失败或超出范围
    }
}