# TI-Based Intelligent Spectrum Analysis and Programmable Waveform Generation System
基于 TI MSPM0G3507 单片机的智能频谱分析与可编程波形生成系统，实现**可编程波形输出、实时信号采集、FFT频谱分析、掉电存储、可视化交互**全流程功能。


## 项目简介
本项目基于 TI MSPM0G3507 单片机开发，集成多波形 DAC 输出、ADC 信号采集、FFT 频谱分析、FLASH 数据存储、OLED 可视化、按键交互与蜂鸣器反馈功能，可完成波形可编程输出、信号实时采集、频谱分析、数据掉电存储与界面交互。

## 硬件平台
- 主控：TI MSPM0G3507 (ARM Cortex-M0+)
- 外设：OLED 显示屏、NOR FLASH、按键模块、蜂鸣器、DAC/ADC 模拟电路
- 接口：I2C、SPI、UART、ADC、DAC、PWM

## 核心功能
1. **多波形 DAC 生成**
   - 支持三角波、方波可编程输出
   - 频率：500~5000Hz（步长 ±500Hz）
   - 幅度：500~2000mV（步长 ±100mV）
   - 占空比：10%~90%（步长 ±10%）

2. **ADC 数据采集**
   - 12 位精度，10kHz 采样率
   - 256 点缓存，定时器硬件触发
   - 无丢失数据采集

3. **FFT 频谱分析**
   - 256 点 FFT 运算（CMSIS-DSP 库）
   - 实时计算主频、时域幅值
   - OLED 频谱柱状图显示

4. **数据存储**
   - RAM 实时输出模式
   - FLASH 掉电存储模式
   - 数据写入+读取校验

5. **可视化与交互**
   - OLED 双模式：数值显示 / 频谱图显示
   - 5 按键参数调节、模式切换
   - 蜂鸣器按键操作反馈

## 工程结构
├── /src # 主程序与驱动源码

├── /include # 头文件

├── /Debug # 编译输出（.gitignore 忽略）

├── README.md # 项目说明

└── .gitignore # Git 忽略配置

## 开发环境
- IDE：Code Composer Studio (CCS)
- 库：TI MSPM0 DriverLib、CMSIS-DSP
- 采样率：10kHz
- 波形更新率：100kHz

## 按键功能
- B1：切换 RAM/FLASH 存储模式
- B2：切换三角波/方波
- B3：切换调节项（频率/幅度/占空比）
- B4/B5：参数+ / 参数-
- 独立按键：切换 OLED 显示模式

## 演示效果
- OLED 实时显示频率、幅值、存储模式、波形类型
- FFT 频谱图直观展示 0~5kHz 信号分布
- UART 输出原始采集数据与分析结果
- FLASH 掉电数据保存与读取验证
见b站视频
https://www.bilibili.com/video/BV1dJXVBhEb6/
