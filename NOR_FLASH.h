#ifndef __NOR_FLASH_H
#define __NOR_FLASH_H

// =============================================================================
#include "ti_msp_dl_config.h"

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

// =============================================================================
// ★★★ 程序 NOR_FLASH.c 参考了 ALIENTEK STM32F103 开发板SPI示例，
// ★★★ 从 STM32F103 移植到 MSPM0G3507 微控制器
// ★★★ 仅供学习参考，不得用于其它任何用途，如有其他应用，应联系原作者许可
// ★★★ 原作者：正点原子 @ALIENTEK（广州市星翼电子科技有限公司）
// =============================================================================

//W25X系列/Q系列芯片列表
#define W25Q80 	    0XEF13          /* W25Q80   芯片ID */
#define W25Q16 	    0XEF14          /* W25Q16   芯片ID */
#define W25Q32 	    0XEF15          /* W25Q32   芯片ID */
#define W25Q64 	    0XEF16          /* W25Q64   芯片ID */
#define W25Q128	    0XEF17          /* W25Q128  芯片ID */
#define NM25Q16     0X6814          /* NM25Q16  芯片ID */
#define NM25Q64     0X5216          /* NM25Q64  芯片ID */
#define NM25Q128    0X5217          /* NM25Q128 芯片ID */
#define BY25Q64     0X6816          /* BY25Q64  芯片ID */
#define BY25Q128    0X6817          /* BY25Q128 芯片ID */

extern uint16_t NORFLASH_TYPE;      //定义我们使用的flash芯片型号
extern uint8_t _25QXX_BUFFER[4096];

// =============================================================================
// FLASH 片选=High
#define  NOR_FLASH_CS_H   DL_GPIO_setPins(SPI1_GPIO_PORT, SPI1_GPIO_CS_PIN)

// FLASH 片选=Low 
#define  NOR_FLASH_CS_L   DL_GPIO_clearPins(SPI1_GPIO_PORT, SPI1_GPIO_CS_PIN)

// =============================================================================


// =============================================================================
// 指令表
#define FLASH_WriteEnable           0x06
#define FLASH_WriteDisable          0x04
#define FLASH_ReadStatusReg         0x05
#define FLASH_WriteStatusReg        0x01
#define FLASH_ReadData              0x03
#define FLASH_FastReadData          0x0B
#define FLASH_FastReadDual          0x3B
#define FLASH_PageProgram           0x02
#define FLASH_BlockErase            0xD8
#define FLASH_SectorErase           0x20
#define FLASH_ChipErase             0xC7
#define FLASH_PowerDown             0xB9
#define FLASH_ReleasePowerDown      0xAB
#define FLASH_DeviceID              0xAB
#define FLASH_ManufactDeviceID      0x90
#define FLASH_JedecDeviceID         0x9F

// =============================================================================
/*
void Norflash_Init(void);
uint8_t  Norflash_ReadID(void);         //读取FLASH ID
uint8_t	 Norflash_ReadSR(void);         //读取状态寄存器
void Norflash_Write_SR(uint8_t sr);     //写状态寄存器
void Norflash_Write_Enable(void);       //写使能
void Norflash_Write_Disable(void);      //写保护
void Norflash_Read(uint8_t *pBuffer, uint32_t ReadAddr, uint8_t NumByteToRead);     //读取flash
void Norflash_Write(uint8_t *pBuffer, uint32_t WriteAddr, uint8_t NumByteToWrite);  //写入flash
void Norflash_Erase_Chip(void);        //整片擦除
void Norflash_Erase_Sector(uint32_t Dst_Addr);//扇区擦除
void Norflash_Wait_Busy(void);          //等待空闲
void Norflash_PowerDown(void);          //进入掉电模式
void Norflash_WAKEUP(void);             //唤醒
*/

// =============================================================================

//TxData:要写入的字节
//返回值:读取到的字节
uint8_t  SPI1_ReadWriteByte(uint8_t  TxData);    //SPI1 读写一个字节

//初始化SPI FLASH的IO口
//void Norflash_Init(void);

//读取SPI_FLASH的状态寄存器
//BIT7  6   5   4   3   2   1   0
//SPR   RV  TB BP2 BP1 BP0 WEL BUSY
//SPR:默认0,状态寄存器保护位,配合WP使用
//TB,BP2,BP1,BP0:FLASH区域写保护设置
//WEL:写使能锁定
//BUSY:忙标记位(1,忙;0,空闲)
//默认:0x00
uint8_t Norflash_ReadSR(void);

//写SPI_FLASH状态寄存器
//只有SPR,TB,BP2,BP1,BP0(bit 7,5,4,3,2)可以写!!!
void Norflash_Write_SR(uint8_t sr);

//SPI_FLASH写使能
//将WEL置位
void Norflash_Write_Enable(void);

//SPI_FLASH写禁止
//将WEL清零
void Norflash_Write_Disable(void);

//读取芯片ID W25X16的ID:0XEF14
uint16_t Norflash_ReadID(void);

//读取SPI FLASH
//在指定地址开始读取指定长度的数据
//pBuffer:数据存储区
//ReadAddr:开始读取的地址(24bit)
//NumByteToRead:要读取的字节数(最大65535)
void Norflash_Read(uint8_t *pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);

//SPI在一页(0~65535)内写入少于256个字节的数据
//在指定地址开始写入最大256字节的数据
//pBuffer:数据存储区
//WriteAddr:开始写入的地址(24bit)
//NumByteToWrite:要写入的字节数(最大256),该数不应该超过该页的剩余字节数!!!
void Norflash_Write_Page(uint8_t *pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);

//无检验写SPI FLASH
//必须确保所写的地址范围内的数据全部为0XFF,否则在非0XFF处写入的数据将失败!
//具有自动换页功能
//在指定地址开始写入指定长度的数据,但是要确保地址不越界!
//pBuffer:数据存储区
//WriteAddr:开始写入的地址(24bit)
//NumByteToWrite:要写入的字节数(最大65535)
//CHECK OK
void Norflash_Write_NoCheck(uint8_t *pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);

//写SPI FLASH
//在指定地址开始写入指定长度的数据
//该函数带擦除操作!
//pBuffer:数据存储区
//WriteAddr:开始写入的地址(24bit)
//NumByteToWrite:要写入的字节数(最大65535)
uint8_t _25QXX_BUFFER[4096];
void Norflash_Write(uint8_t *pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);

//擦除整个芯片
//整片擦除时间:
//W25X16:25s
//W25X32:40s
//W25X64:40s
//等待时间超长...
void Norflash_Erase_Chip(void);

//擦除一个扇区
//Dst_Addr:扇区地址 0~511 for w25x16
//擦除一个山区的最少时间:150ms
void Norflash_Erase_Sector(uint32_t Dst_Addr);

//等待空闲
void Norflash_Wait_Busy(void);

//进入掉电模式
void Norflash_PowerDown(void);

//唤醒
void Norflash_WAKEUP(void);

// =============================================================================

#endif
// =============================================================================
// End of file.

