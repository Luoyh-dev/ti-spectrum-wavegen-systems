#include "NOR_FLASH.h"

// =============================================================================
// ★★★ 程序 NOR_FLASH.c 参考了 ALIENTEK STM32F103 开发板SPI示例，
// ★★★ 从 STM32F103 移植到 MSPM0G3507 微控制器
// ★★★ 仅供学习参考，不得用于其它任何用途，如有其他应用，应联系原作者许可
// ★★★ 原作者：正点原子 @ALIENTEK（广州市星翼电子科技有限公司）
// =============================================================================

// =============================================================================
// 实验使用 W25Q128JV, 芯片引脚与 MSPM0G3507 核心板连接关系如下:
// W25Q128JV 引脚   G3507引脚       备注
// CS               PB6             片选, 用GPIO模式控制
// SLK(CLK)         PB16(SPI SCLK)  SPI时钟
// DO(IO1)          PB7(SPI POCI)   主入从出(控制器输入,外设输出)
// DI(IO0)          PB8(SPI PICO)   主出从入(控制器输出,外设输入)

// =============================================================================
// MSPM0G3507 使用 SPI1 模块, 时钟主频32MHz, SPI模块选择时钟源 SYSCLK(32MHz)
// 配置 CLKDIV.RATIO=0,即预分频为1(不启用),CLKCTL.SCR=31,即32分频
// SPI输出时钟为: (32000000)/((1 + 31) * 2) = 500kHz

#define DELAY_NorFlash (32)


uint16_t NORFLASH_TYPE = W25Q64; // 默认为 W25Q64

// =============================================================================
// W25Q128JV 容量为 128M 比特, 即16M 字节, 由65536页组成, 每页256字节
// 可以一次编程(写入) 256 字节
// 擦除闪存可以按16个页擦除（即1个Sector）,128个页擦除（8个Sector）,
// 256个页擦除（16个Sector）,或整片擦除
// 标准SPI通信支持时钟频率最高为 104MHz
// 详情见 W25Q128JV

// =============================================================================
//SPI1 读写一个字节
//TxData:要写入的字节
//返回值:读取到的字节
uint8_t  SPI1_ReadWriteByte(uint8_t  TxData)
{
    uint8_t Rxdata;
    /* Transmit data and wait to receive it */
    DL_SPI_transmitData8(SPI1_INST, TxData);
    Rxdata = DL_SPI_receiveDataBlocking8(SPI1_INST);
    return Rxdata;                      //返回收到的数据
}

// =============================================================================
// 读取SPI_FLASH的状态寄存器
// BIT7  6   5   4   3   2   1   0
// SPR   RV  TB BP2 BP1 BP0 WEL BUSY
// SPR: 默认0,状态寄存器保护位,配合WP使用
// TB,BP2,BP1,BP0:FLASH区域写保护设置
// WEL:写使能锁定
// BUSY:忙标记位(1,忙;0,空闲)
// 默认:0x00
uint8_t Norflash_ReadSR(void)
{
    uint8_t byte = 0;
    NOR_FLASH_CS_L;                          //使能器件
    SPI1_ReadWriteByte(FLASH_ReadStatusReg);    //发送读取状态寄存器命令
    byte = SPI1_ReadWriteByte(0Xff);           //读取一个字节
    NOR_FLASH_CS_H;                          //取消片选
    return byte;
}
// =============================================================================
//写SPI_FLASH状态寄存器
//只有SPR,TB,BP2,BP1,BP0(bit 7,5,4,3,2)可以写!!!
void Norflash_Write_SR(uint8_t sr)
{
    NOR_FLASH_CS_L;                          //使能器件
    SPI1_ReadWriteByte(FLASH_WriteStatusReg);   //发送写取状态寄存器命令
    SPI1_ReadWriteByte(sr);               //写入一个字节
    NOR_FLASH_CS_H;                          //取消片选
}
// =============================================================================
//SPI_FLASH写使能
//将WEL置位
void Norflash_Write_Enable(void)
{
    NOR_FLASH_CS_L;                          //使能器件
    SPI1_ReadWriteByte(FLASH_WriteEnable);      //发送写使能
    NOR_FLASH_CS_H;                          //取消片选
}
// =============================================================================
//SPI_FLASH写禁止
//将WEL清零
void Norflash_Write_Disable(void)
{
    NOR_FLASH_CS_L;                          //使能器件
    SPI1_ReadWriteByte(FLASH_WriteDisable);     //发送写禁止指令
    NOR_FLASH_CS_H;                          //取消片选
}
// =============================================================================
//读取芯片ID W25X16的ID:0XEF14
uint16_t Norflash_ReadID(void)
{
    uint16_t Temp = 0;
    NOR_FLASH_CS_L;
    SPI1_ReadWriteByte(0x90);//发送读取ID命令
    SPI1_ReadWriteByte(0x00);
    SPI1_ReadWriteByte(0x00);
    SPI1_ReadWriteByte(0x00);
    Temp |= SPI1_ReadWriteByte(0xFF) << 8;
    Temp |= SPI1_ReadWriteByte(0xFF);
    NOR_FLASH_CS_H;
    return Temp;
}
// =============================================================================
//读取SPI FLASH
//在指定地址开始读取指定长度的数据
//pBuffer:数据存储区
//ReadAddr:开始读取的地址(24bit)
//NumByteToRead:要读取的字节数(最大65535)
void Norflash_Read(uint8_t *pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead)
{
    uint16_t i;
    NOR_FLASH_CS_L;                          //使能器件
    SPI1_ReadWriteByte(FLASH_ReadData);         //发送读取命令
    SPI1_ReadWriteByte((uint8_t)((ReadAddr) >> 16)); //发送24bit地址
    SPI1_ReadWriteByte((uint8_t)((ReadAddr) >> 8));
    SPI1_ReadWriteByte((uint8_t)ReadAddr);

    for (i = 0; i < NumByteToRead; i++)
    {
        pBuffer[i] = SPI1_ReadWriteByte(0XFF); //循环读数
    }

    NOR_FLASH_CS_H;                          //取消片选
}
// =============================================================================
//SPI在一页(0~65535)内写入少于256个字节的数据
//在指定地址开始写入最大256字节的数据
//pBuffer:数据存储区
//WriteAddr:开始写入的地址(24bit)
//NumByteToWrite:要写入的字节数(最大256),该数不应该超过该页的剩余字节数!!!
void Norflash_Write_Page(uint8_t *pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
    uint16_t i;
    Norflash_Write_Enable();                  //SET WEL
    NOR_FLASH_CS_L;                          //使能器件
    SPI1_ReadWriteByte(FLASH_PageProgram);      //发送写页命令
    SPI1_ReadWriteByte((uint8_t)((WriteAddr) >> 16)); //发送24bit地址
    SPI1_ReadWriteByte((uint8_t)((WriteAddr) >> 8));
    SPI1_ReadWriteByte((uint8_t)WriteAddr);

    for (i = 0; i < NumByteToWrite; i++)SPI1_ReadWriteByte(pBuffer[i]); //循环写数

    NOR_FLASH_CS_H;                          //取消片选
    Norflash_Wait_Busy();                       //等待写入结束
}
// =============================================================================
//无检验写SPI FLASH
//必须确保所写的地址范围内的数据全部为0XFF,否则在非0XFF处写入的数据将失败!
//具有自动换页功能
//在指定地址开始写入指定长度的数据,但是要确保地址不越界!
//pBuffer:数据存储区
//WriteAddr:开始写入的地址(24bit)
//NumByteToWrite:要写入的字节数(最大65535)
//CHECK OK
void Norflash_Write_NoCheck(uint8_t *pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
    uint16_t pageremain;
    pageremain = 256 - WriteAddr % 256; //单页剩余的字节数

    if (NumByteToWrite <= pageremain)pageremain = NumByteToWrite; //不大于256个字节

    while (1)
    {
        Norflash_Write_Page(pBuffer, WriteAddr, pageremain);

        if (NumByteToWrite == pageremain)break; //写入结束了
        else //NumByteToWrite>pageremain
        {
            pBuffer += pageremain;
            WriteAddr += pageremain;

            NumByteToWrite -= pageremain;             //减去已经写入了的字节数

            if (NumByteToWrite > 256)pageremain = 256; //一次可以写入256个字节
            else pageremain = NumByteToWrite;      //不够256个字节了
        }
    };
}
// =============================================================================
//写SPI FLASH
//在指定地址开始写入指定长度的数据
//该函数带擦除操作!
//pBuffer:数据存储区
//WriteAddr:开始写入的地址(24bit)
//NumByteToWrite:要写入的字节数(最大65535)
uint8_t _25QXX_BUFFER[4096];
void Norflash_Write(uint8_t *pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
    uint32_t secpos;
    uint16_t secoff;
    uint16_t secremain;
    uint16_t i;

    secpos = WriteAddr / 4096; //扇区地址 0~511 for w25x16
    secoff = WriteAddr % 4096; //在扇区内的偏移
    secremain = 4096 - secoff; //扇区剩余空间大小

    if (NumByteToWrite <= secremain)secremain = NumByteToWrite; //不大于4096个字节

    while (1)
    {
        Norflash_Read(_25QXX_BUFFER, secpos * 4096, 4096); //读出整个扇区的内容

        for (i = 0; i < secremain; i++) //校验数据
        {
            if (_25QXX_BUFFER[secoff + i] != 0XFF)break; //需要擦除
        }

        if (i < secremain) //需要擦除
        {
            Norflash_Erase_Sector(secpos);//擦除这个扇区

            for (i = 0; i < secremain; i++)     //复制
            {
                _25QXX_BUFFER[i + secoff] = pBuffer[i];
            }

            Norflash_Write_NoCheck(_25QXX_BUFFER, secpos * 4096, 4096); //写入整个扇区

        }
        else Norflash_Write_NoCheck(pBuffer, WriteAddr, secremain); //写已经擦除了的,直接写入扇区剩余区间.

        if (NumByteToWrite == secremain)break; //写入结束了
        else//写入未结束
        {
            secpos++;//扇区地址增1
            secoff = 0; //偏移位置为0

            pBuffer += secremain; //指针偏移
            WriteAddr += secremain; //写地址偏移
            NumByteToWrite -= secremain;                //字节数递减

            if (NumByteToWrite > 4096)secremain = 4096;    //下一个扇区还是写不完
            else secremain = NumByteToWrite;            //下一个扇区可以写完了
        }
    };
}
// =============================================================================
//擦除整个芯片
//整片擦除时间:
//W25X16:25s
//W25X32:40s
//W25X64:40s
//等待时间超长...
void Norflash_Erase_Chip(void)
{
    Norflash_Write_Enable();                  //SET WEL
    Norflash_Wait_Busy();
    NOR_FLASH_CS_L;                         //使能器件
    SPI1_ReadWriteByte(FLASH_ChipErase);    //发送片擦除命令
    NOR_FLASH_CS_H;                         //取消片选
    Norflash_Wait_Busy();                   //等待芯片擦除结束
}
// =============================================================================
//擦除一个扇区
//Dst_Addr:扇区地址 0~511 for w25x16
//擦除一个山区的最少时间:150ms
void Norflash_Erase_Sector(uint32_t Dst_Addr)
{
    Dst_Addr *= 4096;
    Norflash_Write_Enable();                    //SET WEL
    Norflash_Wait_Busy();
    NOR_FLASH_CS_L;                             //使能器件
    SPI1_ReadWriteByte(FLASH_SectorErase);      //发送扇区擦除指令
    SPI1_ReadWriteByte((uint8_t)((Dst_Addr) >> 16)); //发送24bit地址
    SPI1_ReadWriteByte((uint8_t)((Dst_Addr) >> 8));
    SPI1_ReadWriteByte((uint8_t)Dst_Addr);
    NOR_FLASH_CS_H;                             //取消片选
    Norflash_Wait_Busy();                       //等待擦除完成
}
// =============================================================================
//等待空闲
void Norflash_Wait_Busy(void)
{
    while ((Norflash_ReadSR() & 0x01) == 0x01); // 等待BUSY位清空
}

// =============================================================================
//进入掉电模式
void Norflash_PowerDown(void)
{
    NOR_FLASH_CS_L;                          //使能器件
    SPI1_ReadWriteByte(FLASH_PowerDown);        //发送掉电命令
    NOR_FLASH_CS_H;                          //取消片选
    delay_cycles(DELAY_NorFlash);
    //delay_us(3);                               //等待TPD
}
// =============================================================================
//唤醒
void Norflash_WAKEUP(void)
{
    NOR_FLASH_CS_L;                          //使能器件
    SPI1_ReadWriteByte(FLASH_ReleasePowerDown);   //  send W25X_PowerDown command 0xAB
    NOR_FLASH_CS_H;                          //取消片选
    delay_cycles(DELAY_NorFlash);
    //delay_us(3);                               //等待TRES1
}
// =============================================================================
// End of file.
