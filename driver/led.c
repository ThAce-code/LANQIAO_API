#include "led.h" // 包含LED及其他外设相关的头文件

// --- LED 显示相关全局变量 ---
// 使用 "idata" 关键字将变量存储在8051的内部RAM中，访问速度更快。
// temp_1 用于存储当前想要显示的LED状态
idata unsigned char temp_1 = 0x00;
// temp_1_old 用于缓存上一次写入硬件的LED状态，用于判断状态是否变化
idata unsigned char temp_1_old = 0xff;

/**
 * @brief LED显示函数
 * @param ucLed 一个指向8元素数组的指针，数组每个元素为0或1，分别对应8个LED的灭和亮。
 * 例如 ucLed[0] 控制 LED1, ucLed[7] 控制 LED8。
 */
void Led_Disp(unsigned char *ucLed)
{
	unsigned char temp; // 用于P2口操作的临时变量

	// 1. 将8个独立的0/1值合并成一个8位的字节(temp_1)
	//    例如，如果ucLed = {1,0,1,0,0,0,0,0}, temp_1 将变为 00000101b = 0x05
	temp_1 = 0x00; // 先清零
	temp_1 = (ucLed[0] << 0) | (ucLed[1] << 1) | (ucLed[2] << 2) | (ucLed[3] << 3) |
			 (ucLed[4] << 4) | (ucLed[5] << 5) | (ucLed[6] << 6) | (ucLed[7] << 7);

	// 2. 状态改变检测：只有当期望的LED状态与上一次设置的状态不同时，才执行硬件操作
	if (temp_1 != temp_1_old)
	{
		// 3. 更新硬件
		// P0口写入temp_1的反码。因为LED是共阳极连接，低电平(0)点亮，高电平(1)熄灭。
		// 例如，要点亮LED1(ucLed[0]=1), temp_1的bit0为1, ~temp_1的bit0则为0，输出低电平点亮LED。
		P0 = ~temp_1;

		// 使用138译码器选择LED外设组（Y4输出，地址100）
		temp = P2 & 0x1f;	// 保留P2低5位
		temp = temp | 0x80; // 设置P2高3位为100
		P2 = temp;			// 选中LED锁存器
		temp = P2 & 0x1f;	// 恢复P2高3位为000
		P2 = temp;			// 取消选择，完成数据锁存

		// 4. 更新状态缓存
		temp_1_old = temp_1;
	}
}

/**
 * @brief 关闭所有LED
 */
void Led_Off()
{
	unsigned char temp;

	// P0口写入全1(0xff)，对于共阳极LED，所有灯都会熄灭。
	P0 = 0xff;

	// 同样地，选择LED外设组来使P0的设置生效。
	temp = P2 & 0x1f;
	temp = temp | 0x80;
	P2 = temp;
	temp = P2 & 0x1f;
	P2 = temp;

	// 更新状态缓存。因为所有灯都关了，所以“亮灯”的状态是0x00。
	// 这样下次调用Led_Disp()时，只要有灯要亮，就一定会触发硬件更新。
	temp_1_old = 0x00;
}

// --- 蜂鸣器、继电器、电机 相关全局变量 ---
// 这三个外设通常连接在同一个锁存器上，由138译码器的同一个输出（Y5）控制。
// 因此，它们的状态被合并在同一个字节(temp_2)里进行管理。
idata unsigned char temp_2 = 0x00;
idata unsigned char temp_2_old = 0xff;

/**
 * @brief 控制蜂鸣器
 * @param enable bit类型的参数，1 为响，0 为不响
 */
void Beep(bit enable)
{
	unsigned char temp; // P2口操作临时变量

	// 1. 根据 'enable' 参数，修改状态字节 temp_2 中对应的位
	//    蜂鸣器由 bit 6 (0x40 = 0100 0000b) 控制
	if (enable)
		temp_2 |= 0x40; // 使用“按位或”将蜂鸣器对应的位置1，表示开启
	else
		temp_2 &= (~0x40); // 使用“按位与”和“反码”将对应的位清0，表示关闭

	// 2. 状态改变检测：只有在整体状态改变时才操作硬件
	if (temp_2 != temp_2_old)
	{
		// 3. 更新硬件
		// 直接将状态字节写入P0。这里没有取反，表示高电平(1)开启设备。
		P0 = temp_2;

		// 使用138译码器选择杂项外设组（Y5输出，地址101）
		temp = P2 & 0x1f;	// 保留P2低5位
		temp = temp | 0xa0; // 设置P2高3位为101 (0xa0 = 1010 0000b)
		P2 = temp;			// 选中杂项外设锁存器
		temp = P2 & 0x1f;	// 恢复P2高3位
		P2 = temp;			// 取消选择，完成锁存

		// 4. 更新状态缓存
		temp_2_old = temp_2;
	}
}

/**
 * @brief 控制电机
 * @param enable bit类型的参数，1 为转，0 为停
 * @note  此函数逻辑与Beep完全相同，只是操作的位不同。
 */
void Motor(bit enable)
{
	unsigned char temp;

	// 电机由 bit 5 (0x20 = 0010 0000b) 控制
	if (enable)
		temp_2 |= 0x20;
	else
		temp_2 &= (~0x20);

	if (temp_2 != temp_2_old)
	{
		P0 = temp_2;

		// 同样选择Y5外设组
		temp = P2 & 0x1f;
		temp = temp | 0xa0;
		P2 = temp;
		temp = P2 & 0x1f;
		P2 = temp;

		temp_2_old = temp_2;
	}
}

/**
 * @brief 控制继电器
 * @param enable bit类型的参数，1 为吸合，0 为断开
 * @note  此函数逻辑与Beep完全相同，只是操作的位不同。
 */
void Relay(bit enable)
{
	unsigned char temp;

	// 继电器由 bit 4 (0x10 = 0001 0000b) 控制
	if (enable)
		temp_2 |= 0x10;
	else
		temp_2 &= (~0x10);

	if (temp_2 != temp_2_old)
	{
		P0 = temp_2;

		// 同样选择Y5外设组
		temp = P2 & 0x1f;
		temp = temp | 0xa0;
		P2 = temp;
		temp = P2 & 0x1f;
		P2 = temp;

		temp_2_old = temp_2;
	}
}