/* #   I2C代码片段说明
    1.  本文件夹中提供的驱动代码供参赛选手完成程序设计参考。
    2.  参赛选手可以自行编写相关代码或以该代码为基础，根据所选单片机类型、运行速度和试题
        中对单片机时钟频率的要求，进行代码调试和修改。
*/
#include "iic.h"       // 可能包含了一些I2C相关的宏定义或函数声明
#include "intrins.h"   // 包含了Keil C51编译器提供的内部函数，如_nop_()

#define DELAY_TIME  5   // 定义一个延时常量，用于控制I2C信号的速率

// --- 引脚定义 ---
// 使用sbit关键字定义I2C总线的SCL（时钟）和SDA（数据）线所连接的单片机引脚
sbit scl=P2^0; // SCL 时钟线连接到 P2.0
sbit sda=P2^1; // SDA 数据线连接到 P2.1

/**
 * @brief I2C总线延时函数
 * @param n 延时参数，数值越大延时越长
 * @note  这是一个软件延时，通过执行空操作指令_nop_()来产生。
 * I2C的通信速率与此延时有关。
 */
static void I2C_Delay(unsigned char n)
{
    do
    {
        _nop_();_nop_();_nop_();_nop_();_nop_();
        _nop_();_nop_();_nop_();_nop_();_nop_();
        _nop_();_nop_();_nop_();_nop_();_nop_();
    }
    while(n--);
}

/**
 * @brief I2C总线起始信号
 * @note  时序: SCL为高电平时，SDA由高电平跳变为低电平。
 */
void I2CStart(void)
{
    sda = 1; // 1. 确保SDA为高电平
    scl = 1; // 2. 确保SCL为高电平
    I2C_Delay(DELAY_TIME);
    sda = 0; // 3. SCL高电平期间，SDA拉低，产生起始信号
    I2C_Delay(DELAY_TIME);
    scl = 0; // 4. 钳住总线，准备发送数据
}

/**
 * @brief I2C总线停止信号
 * @note  时序: SCL为高电平时，SDA由低电平跳变为高电平。
 */
void I2CStop(void)
{
    sda = 0; // 1. 确保SDA为低电平
    scl = 1; // 2. 拉高SCL
    I2C_Delay(DELAY_TIME);
    sda = 1; // 3. SCL高电平期间，SDA拉高，产生停止信号
    I2C_Delay(DELAY_TIME);
}

/**
 * @brief 通过I2C总线发送一个字节
 * @param byt 要发送的字节数据
 * @note  数据从最高位（MSB）开始发送。
 */
void I2CSendByte(unsigned char byt)
{
    unsigned char i;

    for(i=0; i<8; i++){ // 一个字节8位，循环8次
        scl = 0; // 1. 拉低SCL，准备在SDA上放置数据位
        I2C_Delay(DELAY_TIME);
        if(byt & 0x80){ // 2. 判断当前字节的最高位是1还是0
            sda = 1;    //    如果是1，则SDA置1
        }
        else{
            sda = 0;    //    如果是0，则SDA置0
        }
        I2C_Delay(DELAY_TIME);
        scl = 1; // 3. 拉高SCL，从机在此上升沿读取SDA上的数据位
        byt <<= 1; // 4. 将字节左移一位，准备发送下一位
        I2C_Delay(DELAY_TIME);
    }

    scl = 0;   // 5. 8位发送完毕后，拉低SCL，准备接收ACK信号
}

/**
 * @brief 通过I2C总线接收一个字节
 * @return unsigned char 返回接收到的字节数据
 * @note  数据从最高位（MSB）开始接收。
 */
unsigned char I2CReceiveByte(void)
{
    unsigned char da; // 用于存储接收到的数据
    unsigned char i;
    for(i=0; i<8; i++){
        scl = 1; // 1. 拉高SCL，通知从机可以把数据放到SDA上了
        I2C_Delay(DELAY_TIME);
        da <<= 1; // 2. 将接收变量左移一位，为新来的数据位腾出空间(LSB)
        if(sda)   // 3. 读取SDA线上的电平
            da |= 0x01; // 如果SDA为1，则将接收变量的最低位置1
        scl = 0; // 4. 拉低SCL，完成一位数据的接收
        I2C_Delay(DELAY_TIME);
    }
    return da; // 5. 返回完整的8位数据
}

/**
 * @brief 等待从机的应答(ACK)信号
 * @return unsigned char 返回ACK位。0表示有效应答(ACK)，1表示无应答(NACK)。
 */
unsigned char I2CWaitAck(void)
{
    unsigned char ackbit;

    scl = 1; // 1. 主机拉高SCL，并释放SDA，准备接收从机的应答
    I2C_Delay(DELAY_TIME);
    ackbit = sda; // 2. 读取SDA线上的电平，即ACK位
    scl = 0; // 3. 主机拉低SCL
    I2C_Delay(DELAY_TIME);

    return ackbit; // 返回应答位
}

/**
 * @brief 主机向从机发送应答(ACK)或非应答(NACK)信号
 * @param ackbit 要发送的应答位。0表示ACK，1表示NACK。
 */
void I2CSendAck(unsigned char ackbit)
{
    scl = 0;    // 1. 拉低SCL
    sda = ackbit; // 2. 在SDA上放置ACK(0)或NACK(1)位
    I2C_Delay(DELAY_TIME);
    scl = 1;    // 3. 拉高SCL，从机在上升沿读取此信号
    I2C_Delay(DELAY_TIME);
    scl = 0;    // 4. 拉低SCL
    sda = 1;    // 5. 主机释放SDA线，通常使其恢复高电平
    I2C_Delay(DELAY_TIME);
}

/*
// --- PCF8591 A/D D/A 转换芯片相关函数 ---
// PCF8591控制字节说明:
// bit6: 模拟输出使能位 (1=使能)
// bit[1:0]: A/D通道选择
//    00: AIN0, 对应通道0，外部输入
//    01: AIN1, 对应通道1，光敏电阻
//    10: AIN2, 对应通道2，差分输入
//    11: AIN3, 对应通道3，滑动变阻器
// 例如:
// 0x41 = 0100 0001b -> 使能DA输出，并选择AD通道1（光敏电阻）
// 0x03 = 0000 0011b -> 仅选择AD通道3（滑动变阻器），不使能DA
*/

/**
 * @brief 从PCF8591读取A/D转换结果
 * @param addr 控制字节，用于选择A/D转换的通道
 * @return unsigned char 返回8位的数字量结果 (0~255)
 */
unsigned char Ad_Read(unsigned char addr)
{
    unsigned char temp;

    I2CStart();         // 启动I2C总线
    I2CSendByte(0x90);  // 发送PCF8591的设备地址和写操作位(1001 0000)
    I2CWaitAck();       // 等待ACK
    I2CSendByte(addr);  // 发送控制字节，选择要进行AD转换的通道
    I2CWaitAck();       // 等待ACK

    // 注意：这里发起一个“重复起始”信号，用于切换到读模式，而无需先发送停止信号
    I2CStart();         
    I2CSendByte(0x91);  // 发送PCF8591的设备地址和读操作位(1001 0001)
    I2CWaitAck();       // 等待ACK
    temp = I2CReceiveByte(); // 接收从机发回的AD转换结果

    I2CSendAck(1);      // 主机发送NACK(1)，表示读取结束，不再需要更多数据
    I2CStop();          // 发送停止信号，释放总线

    return temp;        // 返回读取到的数据
}

/**
 * @brief 通过PCF8591输出D/A转换后的模拟电压
 * @param dat 要转换的8位数字量 (0~255)，对应输出电压 0~5
 */
void Da_Write(unsigned char dat)
{
    I2CStart();         // 启动I2C总线
    I2CSendByte(0x90);  // 发送PCF8591的设备地址和写操作位
    I2CWaitAck();       // 等待ACK
    I2CSendByte(0x41);  // 发送控制字节，bit6=1使能DA输出。bit[1:0]=01同时选择了AD通道1。
    I2CWaitAck();       // 等待ACK
    I2CSendByte(dat);   // 发送要进行DA转换的数字量
    I2CWaitAck();       // 等待ACK
}

// --- AT24C02 EEPROM 存储芯片相关函数 ---

/**
 * @brief 向EEPROM写入多个字节
 * @param str   指向要写入数据的数组的指针
 * @param addr  EEPROM内部的起始写入地址
 * @param num   要写入的数据个数（字节数）
 */
void EEPROM_Write(unsigned char *str, unsigned char addr, unsigned num)
{
    I2CStart();
    I2CSendByte(0xa0);  // 发送AT24C02的设备地址和写操作位(1010 0000)
    I2CWaitAck();
    I2CSendByte(addr);  // 发送要写入的EEPROM内部起始地址
    I2CWaitAck();

    while(num--)
    {
        I2CSendByte(*str++); // 发送一个数据字节，然后指针指向下一个
        I2CWaitAck();
        I2C_Delay(200); // 延时等待EEPROM内部写操作完成。这是一种简单的延时方法。
    }
    I2CStop(); // 所有数据发送完毕后，发送停止信号

    // 在停止信号后进行一个长延时，确保EEPROM有足够的时间完成最后一次写入周期
    I2C_Delay(255);
    I2C_Delay(255);
    I2C_Delay(255);
    I2C_Delay(255);
    I2C_Delay(255);
    I2C_Delay(255);
    I2C_Delay(255);
    I2C_Delay(255);
    I2C_Delay(255);
    I2C_Delay(255);
}

/**
 * @brief 从EEPROM读取多个字节
 * @param str   指向用于存放读取数据的数组的指针
 * @param addr  EEPROM内部的起始读取地址
 * @param num   要读取的数据个数（字节数）
 */
void EEPROM_Read(unsigned char *str, unsigned char addr, unsigned num)
{
    // 1. "虚拟写"操作：先设置EEPROM的内部地址指针
    I2CStart();
    I2CSendByte(0xa0); // 发送设备地址和写操作位
    I2CWaitAck();
    I2CSendByte(addr); // 发送要开始读取的内部地址
    I2CWaitAck();

    // 2. 发送“重复起始”信号，切换到读模式
    I2CStart();
    I2CSendByte(0xa1); // 发送设备地址和读操作位 (1010 0001)
    I2CWaitAck();

    while(num--)
    {
        *str++ = I2CReceiveByte(); // 接收一个字节并存入数组，然后指针后移
        if(num) // 判断是否是最后一个字节
        {
            I2CSendAck(0); // 如果不是最后一个，主机发送ACK(0)，表示还想继续读
        }
        else
        {
            I2CSendAck(1); // 如果是最后一个，主机发送NACK(1)，通知从机读取结束
        }
    }
    I2CStop(); // 发送停止信号
}

