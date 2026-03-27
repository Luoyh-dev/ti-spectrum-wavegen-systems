#include "arm_const_structs.h"
#include "arm_math.h"
#include "ti_msp_dl_config.h"
#include <stdint.h>
#include "I2C_GPIO_25a.h"   // I2C 接口驱动函数
#include "OLED13.h"         // OLED 液晶模块操作函数
#include "OLED_BMP.h"       // 图片表，不显示图片时，可不包含本文件
#include "stdio.h"
#define DELAY (16000000)
#define NUM_SAMPLES        256
#define SAMPLE_RATE_HZ     10000.0f   // 10 kHz
#define ADC_REF_MV         3300.0f    // 3.3V -> 3300 mV
#define ADC_FULL_SCALE     4095.0f    // 12-bit ADC
#define ADC_MID_CODE       2048       // 中点，用来去直流

volatile uint16_t gADCBuffer[NUM_SAMPLES];
volatile uint16_t gSampleIndex = 0;
volatile bool     gBufferFull  = false;

/* FFT 相关缓冲区（Q15） */
static q15_t  gFFTInput[NUM_SAMPLES * 2];  // 复数输入：[Re0,Im0,Re1,Im1,...]
static q15_t  gFFTMagnitude[NUM_SAMPLES];  // 频谱幅度

/* 输出给 OLED 使用的两个变量 */
volatile float gSignalFreq_Hz = 0.0f;  // 主频，单位 Hz
volatile float gSignalAmp_MV  = 0.0f;  // 幅度，单位 mV

volatile unsigned long tick = 0, now = 0;

// OLED 刷新节流：上一次在 OLED 上刷新的时刻（单位：ms）
static unsigned int gLastOLEDUpdateTick = 0;

/* 频谱图专用的刷新时间戳（单位：ms） */
static unsigned int gLastSpectrumUpdateTick = 0;

volatile unsigned int Temp[1] ;
volatile unsigned int StartTick[1] = {0};
volatile unsigned int ActionDown[1] = {0}, ActionUp[1] = {0};
volatile unsigned int State[1] = {1}, last_State[1] = {1};

volatile unsigned int StartScan = 0, SettingMode = 0, Refresh = 0;
uint8_t toggle_var = 0;



// ====================== DAC可调参数配置（替换原有固定宏） ======================
#define DAC_SAMPLE_RATE    100000UL  // DAC波形更新率（100kHz，固定）
#define DAC_DC_OFFSET      2048      // DAC中点（3.3V→1.65V，固定）
#define MAX_AMPLITUDE      2000      // 最大峰峰值（和三角波一致）
#define MIN_AMPLITUDE      500       // 最小峰峰值（避免过小）
#define MAX_FREQ           5000.0f  // 最大频率（10kHz，避免失真）
#define MIN_FREQ           500.0f     // 最小频率（10Hz，保证波形连续）
#define MAX_DUTY           0.9f      // 方波最大占空比（90%）
#define MIN_DUTY           0.1f      // 方波最小占空比（10%）
#define FREQ_STEP          100.0f    // 频率调节步长（每次±100Hz）
#define AMPL_STEP          100       // 幅度调节步长（每次±100）
#define DUTY_STEP          0.1f      // 占空比调节步长（每次±10%）

// DAC三角波计算中间变量（原有，保留）
volatile uint32_t dac_cycle_pts;        // 周期总点数
volatile uint32_t dac_rise_pts;         // 上升段/高电平点数
volatile uint32_t dac_fall_pts;         // 下降段点数
volatile int16_t  dac_rise_step;        // 上升步长
volatile int16_t  dac_fall_step;        // 下降步长
volatile uint32_t dac_rise_remainder;   // 上升段余数
volatile uint32_t dac_fall_remainder;   // 下降段余数
volatile uint16_t dac_current_val;      // 当前DAC输出值
volatile uint32_t dac_point_cnt;        // 周期内计数
volatile uint16_t dac_min_val;          // 低电平/谷值
volatile uint16_t dac_max_val;          // 高电平/峰值


// 波形类型枚举（原有，保留）
typedef enum {
    WAVE_TRIANGLE = 0,
    WAVE_SQUARE = 1

} WaveType_t;

// 调节模式枚举（新增：切换调节「频率/幅度/占空比」）
typedef enum {
    ADJ_FREQ = 0,   // 调节频率
    ADJ_AMPL = 1,   // 调节幅度
    ADJ_DUTY = 2    // 调节占空比（仅方波有效）
} AdjustMode_t;

// 全局控制变量（新增：volatile确保中断/主循环同步访问）
volatile WaveType_t   g_current_wave = WAVE_TRIANGLE;  // 当前波形（默认方波）
volatile AdjustMode_t g_adjust_mode = ADJ_FREQ;      // 当前调节项（默认调频率）
// 可调参数（新增：所有波形共用频率/幅度，方波额外用占空比）
volatile float        g_wave_freq = 1500.0f;         // 初始频率1kHz
volatile uint16_t     g_wave_ampl = 2000;            // 初始幅度1500（峰峰值）
volatile float        g_square_duty = 0.7f;          // 初始占空比50%（仅方波）


/////////////////////////////////////////
unsigned int gettick(void)
{
    unsigned int t = tick;
    return t;
}

void GPIO_BUTTON_DETECT (void)
{
    Temp[0] =  !(DL_GPIO_readPins(GPIOA, DL_GPIO_PIN_18) != 0);
}
void BUZZ_ON(uint32_t ms); // 声明要和定义完全一致（返回值、函数名、参数类型）
void EdgeDetect(unsigned int KeyNum)
{
//  if (Temp[KeyNum] != State[KeyNum])
//  {
//    if (StartTick[KeyNum] == 0)
//    { StartTick[KeyNum] = gettick();  }
//    else
//    {
//        if(now - StartTick[KeyNum] > 2)
//        {
//            State[KeyNum] = Temp[KeyNum];
//            StartTick[KeyNum] = 0;
//        }
//    }
//  }
//  else { StartTick[KeyNum] = 0; } // 只要Temp与State相等，立即复位计数器，防止多次抖动也能让计数器满足要求
    State[KeyNum] = Temp[KeyNum];
}

void KeyPress(unsigned int KeyNum)
{
  now = gettick();

  if (State[KeyNum] == 0 && last_State[KeyNum] == 1) // 当State为0，lastState为1时，记为下降沿，即按下按键
  {
    ActionDown[KeyNum] = 1;
  }

  if (State[KeyNum] == 1 && last_State[KeyNum] == 0) // 当State为1，lastState为0时，记为上升沿，即抬起按键
  {
    ActionUp[KeyNum] = 1;
  }

  last_State[KeyNum] = State[KeyNum]; // 更新lastState方便下次检测
}

void Event_Consume(void)
{
    if (ActionDown[0])
    {
        ActionDown[0] = 0;
        SettingMode = !SettingMode;
        BUZZ_ON(1); // 按键按下短响
    }
}

void Led_1_Control(void)
{
    static uint16_t acc = 0;// 定义一个静态变量用于计数，每次进入中断加一
    static bool led1_State = false;  // 定义一个静态布尔变量表示LED状态：false=灭，true=亮

    // 每计数250次触发一次LED状态翻转
    // 如果定时器中断周期为1ms，则250*1ms=250ms，对应2Hz闪烁频率
    if (++acc >= 250)
    {
        acc = 0;
        led1_State = !led1_State;

        if (led1_State)
        {
            DL_GPIO_setPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_TEST_PIN);
        }
        else
        {
            DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_TEST_PIN);
        }
    }
}

/* 启动一帧采样：复位索引，打开定时器 */
static void StartSamplingFrame(void)
{
    gSampleIndex = 0;
    gBufferFull  = false;

    /* 清一下可能残留的中断标志（保险起见） */
    DL_ADC12_clearInterruptStatus(ADC12_0_INST,
                                  DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);

    /* 启动 10 kHz 定时器 */
    DL_TimerG_startCounter(TIMER_0_INST);
}

static void ComputeTimeDomainAmplitude(void)
{
    uint16_t minCode = 0xFFFF;
    uint16_t maxCode = 0;

    for (uint16_t n = 0; n < NUM_SAMPLES; n++) {
        uint16_t v = gADCBuffer[n];
        if (v < minCode) minCode = v;
        if (v > maxCode) maxCode = v;
    }

    float mv_per_code = ADC_REF_MV / ADC_FULL_SCALE;

    // 峰值幅度 = (最大值 - 最小值) / 2 * 每码对应电压 (mV)
    gSignalAmp_MV = ((float)maxCode - (float)minCode) * 0.5f * mv_per_code;
}

// ====================== DAC纯输出核心函数 ======================
// 等待DAC模块就绪（非阻塞，仅短时间等待）
static void DAC_WaitReady(void) {
    while (!DL_DAC12_isEnabled(DAC0) ||
           (DL_DAC12_getInterruptStatus(DAC0, DL_DAC12_INTERRUPT_MODULE_READY) == 0)) {
        __asm("NOP"); // 短等待，确保DAC硬件就绪
    }
    DL_DAC12_clearInterruptStatus(DAC0, DL_DAC12_INTERRUPT_MODULE_READY);
}



// 配置三角波参数（1kHz、同幅度，替代原DAC_SetFixedWaveParams）
// 1. 三角波配置函数（修改后）
static void DAC_ConfigTriangleWave(void) {
    // 幅度：共用 g_wave_ampl
    dac_min_val = DAC_DC_OFFSET - (g_wave_ampl / 2);
    dac_max_val = DAC_DC_OFFSET + (g_wave_ampl / 2);

    // 频率：共用 g_wave_freq（周期点数=采样率/频率）
    dac_cycle_pts = (uint32_t)(DAC_SAMPLE_RATE / g_wave_freq);

    // 占空比：共用 g_square_duty（不再固定50%）
    dac_rise_pts = (uint32_t)(dac_cycle_pts * g_square_duty); // 上升段点数=周期*占空比
    dac_fall_pts = dac_cycle_pts - dac_rise_pts;              // 下降段点数=周期-上升段

    // 步长计算（原有逻辑，保留，确保波形平滑）
    int32_t total_step = dac_max_val - dac_min_val;
    dac_rise_step = total_step / dac_rise_pts;
    dac_rise_remainder = total_step % dac_rise_pts;
    dac_fall_step = total_step / dac_fall_pts;
    dac_fall_remainder = total_step % dac_fall_pts;

    // 初始化状态
    dac_current_val = dac_min_val;
    dac_point_cnt = 0;
}

// 2. 方波配置函数（修改后）
static void DAC_ConfigSquareWave(void) {
    // 替换 WAVE_COMMON_AMPLITUDE → g_wave_ampl
    dac_min_val = DAC_DC_OFFSET - (g_wave_ampl / 2); // 低电平
    dac_max_val = DAC_DC_OFFSET + (g_wave_ampl / 2); // 高电平
    // 替换 WAVE_COMMON_FREQ → g_wave_freq
    dac_cycle_pts = (uint32_t)(DAC_SAMPLE_RATE / g_wave_freq); // 周期点数
    // 替换 WAVE_SQUARE_DUTY → g_square_duty
    dac_rise_pts = (uint32_t)(dac_cycle_pts * g_square_duty); // 高电平点数（占空比控制）
    dac_point_cnt = 0; // 周期计数清零
}



// DAC模块初始化函数（仅需调用一次，已修改为TIMER_2_INST）
void DAC_Init(void) {
    // 1. DAC硬件初始化（启用电源、模块、输出引脚）
    DL_DAC12_enablePower(DAC0);                  // 启用DAC电源
    DL_DAC12_enable(DAC0);                       // 启用DAC模块
    DL_DAC12_enableOutputPin(DAC0);              // 启用DAC输出引脚（需在SysConfig配置）
    DL_DAC12_setAmplifier(DAC0, DL_DAC12_AMP_ON); // 强制开启DAC放大器（关键，确保输出有效）
    DAC_WaitReady();                             // 等待DAC就绪
    DL_DAC12_performSelfCalibrationBlocking(DAC0); // DAC自校准（保证输出精度）

    // 2. 配置固定三角波参数
//    DAC_SetFixedWaveParams();
    DAC_ConfigTriangleWave(); // 换成新函数名（和你定义的一致

    // 3. 启用TIMER_2_INST零事件中断（专门用于DAC波形更新）
    DL_Timer_enableInterrupt(TIMER_2_INST, DL_TIMERG_INTERRUPT_ZERO_EVENT);
    // 启动TIMER_2_INST计数器（触发DAC波形更新）
    DL_Timer_startCounter(TIMER_2_INST);
}



#include "NOR_FLASH.h"  // 需确保已包含SPI FLASH驱动头文件

// ====================== FLASH配置（无检测/亮灯） ======================
#define FLASH_ADC_START_ADDR 0x000000  // ADC数据存储起始地址
#define ADC_DATA_BYTES_LEN   512       // 256个uint16_t → 512字节
static uint32_t gLastFlashSaveTick = 0; // 存储/输出节流时间戳
static float gLastFlashFreq = -1.0f;   // 上次存储的频率（变化检测）
static float gLastFlashAmp  = -1.0f;   // 上次存储的幅值（变化检测）

// 1. FLASH初始化（简化：无检测、无亮灯）
void FLASH_Init(void)
{
    // 依赖SPI图形化配置已正确，无需额外操作
}

// 2. ADC数据转字节数组（适配FLASH字节读写）
void ADCData_To_ByteBuf(uint16_t *adc_buf, uint8_t *byte_buf, uint32_t adc_len)
{
    // 小端序拆分：uint16_t→2个uint8_t
    for (uint32_t i=0; i<adc_len; i++) {
        byte_buf[2*i]   = (adc_buf[i] >> 0) & 0xFF;
        byte_buf[2*i+1] = (adc_buf[i] >> 8) & 0xFF;
    }
}

// 3. 字节数组转ADC数据（读取FLASH后还原）
void ByteBuf_To_ADCData(uint8_t *byte_buf, uint16_t *adc_buf, uint32_t adc_len)
{
    // 小端序合并：2个uint8_t→1个uint16_t
    for (uint32_t i=0; i<adc_len; i++) {
        adc_buf[i] = (uint16_t)byte_buf[2*i] | ((uint16_t)byte_buf[2*i+1] << 8);
    }
}

// 4. 核心函数：存储ADC数据到FLASH + 读取并UART输出原始数据（无阻塞）
#define FLASH_FREQ_THRESHOLD  100.0f
#define FLASH_AMP_THRESHOLD   50.0f
void FLASH_CheckAndSaveADCData(void)
{
    // 1. 节流控制：每200ms仅处理一次，避免阻塞OLED
    if ((now - gLastFlashSaveTick) < 200) {
        return;
    }
    gLastFlashSaveTick = now;

    // 2. 判断数据是否变化（首次执行强制存储+输出）
    bool needSave = false;
    if (gLastFlashFreq < 0 || gLastFlashAmp < 0) {
        needSave = true; // 首次执行：强制存储+输出
    } else {
        float freqDiff = fabsf(gSignalFreq_Hz - gLastFlashFreq);
        float ampDiff  = fabsf(gSignalAmp_MV - gLastFlashAmp);
        needSave = (freqDiff >= FLASH_FREQ_THRESHOLD) || (ampDiff >= FLASH_AMP_THRESHOLD);
    }

    // 3. 数据变化/首次执行：存储→读取→UART输出原始数据
    if (needSave) {
        uint8_t flash_write_buf[ADC_DATA_BYTES_LEN] = {0};
        uint8_t flash_read_buf[ADC_DATA_BYTES_LEN] = {0};
        uint16_t flash_adc_buf[NUM_SAMPLES] = {0}; // FLASH读取的ADC原始数据

        // 步骤1：转换ADC数据为字节数组，写入FLASH
        ADCData_To_ByteBuf((uint16_t*)gADCBuffer, flash_write_buf, NUM_SAMPLES);
        Norflash_Write(flash_write_buf, FLASH_ADC_START_ADDR, ADC_DATA_BYTES_LEN);

        // 步骤2：从FLASH读取字节数组，还原为ADC原始数据
        Norflash_Read(flash_read_buf, FLASH_ADC_START_ADDR, ADC_DATA_BYTES_LEN);
        ByteBuf_To_ADCData(flash_read_buf, flash_adc_buf, NUM_SAMPLES);

        // 步骤3：更新对比值（供下次判断变化）
        gLastFlashFreq = gSignalFreq_Hz;
        gLastFlashAmp  = gSignalAmp_MV;

        // 步骤4：UART输出FLASH中读取的原始数据（和原有格式完全一致）
        printf("=====================================\r\n");
        printf("=== FLASH读取ADC原始采样数据（%d点）===\r\n", NUM_SAMPLES);
        // 每32个数据换行（减少UART阻塞）
        for (uint16_t i = 0; i < NUM_SAMPLES; i++) {
            if (i % 32 == 0) printf("\r\n");
            printf("%4d ", flash_adc_buf[i]);
        }
        // 输出核心频率/幅值（兼容原有逻辑）
        printf("\r\n\r\n=== FLASH存储&读取完成 ===\r\n");
        printf("Freq=%.1fHz, Ampl=%.1fmV\r\n", gSignalFreq_Hz, gSignalAmp_MV);
        printf("=====================================\r\n\r\n");
    }
    printf("FLASH:Freq=%.1fHz, Ampl=%.1fmV\r\n", gSignalFreq_Hz, gSignalAmp_MV);
}

/*UART输出*/
/* 串口输出ADC原始数据和FFT结果 */
/* 串口输出FFT核心结果（极简节流版，避免阻塞主循环） */
/* 串口输出逻辑：无变化仅输关键值，有变化才输原始ADC数据+关键值 */
static void PrintADCAndFFTData(void)
{
    // 1. 节流控制：每200ms仅处理一次输出，避免频繁阻塞
    static uint32_t lastUartTick = 0;
    if ((now - lastUartTick) < 200) {
        return;
    }
    lastUartTick = now;

    // 2. 定义变化阈值（可按需调整：频率变化≥1Hz、幅值变化≥5mV判定为有效变化）
    #define FREQ_CHANGE_THRESHOLD  100.0f   // 频率变化阈值(Hz)
    #define AMP_CHANGE_THRESHOLD   50.0f   // 幅值变化阈值(mV)

    // 3. 静态变量：记录上一次的频率/幅值，首次执行强制输出原始数据
    static float lastFreq = -1.0f;
    static float lastAmp  = -1.0f;
    // 标记是否需要输出原始数据（首次执行/值变化时置1）
    bool needPrintRawData = false;

    // 4. 判断是否发生有效变化（浮点值不能直接相等，用差值绝对值判断）
    if (lastFreq < 0 || lastAmp < 0) {
        // 首次执行：强制输出一次原始数据
        needPrintRawData = true;
    } else {
        float freqDiff = fabsf(gSignalFreq_Hz - lastFreq);
        float ampDiff  = fabsf(gSignalAmp_MV - lastAmp);
        // 频率或幅值变化超过阈值，判定为有效变化
        if (freqDiff >= FREQ_CHANGE_THRESHOLD || ampDiff >= AMP_CHANGE_THRESHOLD) {
            needPrintRawData = true;
        }
    }

    // 5. 更新上一次的频率/幅值（供下次对比）
    lastFreq = gSignalFreq_Hz;
    lastAmp  = gSignalAmp_MV;

    // 6. 有变化：输出原始ADC数据 + 核心值
    if (needPrintRawData) {
        printf("=====================================\r\n");
        printf("=== ADC原始采样数据（%d点）===\r\n", NUM_SAMPLES);
        // 每32个数据换行（减少换行次数，降低串口阻塞时间）
        for (uint16_t i = 0; i < NUM_SAMPLES; i++) {
            if (i % 32 == 0) printf("\r\n");
            printf("%4d ", gADCBuffer[i]);
        }
        printf("Freq=%.1fHz, Ampl=%.1fmV\r\n", gSignalFreq_Hz, gSignalAmp_MV);
        printf("\r\n\r\n=== 数据变化触发更新 ===\r\n");
    }

    // 7. 始终输出核心频率/幅值（无论是否变化）
    printf("RAM:Freq=%.1fHz, Ampl=%.1fmV\r\n", gSignalFreq_Hz, gSignalAmp_MV);
    if (needPrintRawData) {
        printf("=====================================\r\n\r\n");
    }
}


/* 把 gADCBuffer 做 FFT，求主频和幅度（单位：Hz 和 mV） */
static void ProcessFFTAndUpdateResults(void)
{
    /* 1) ADC 码值 -> 去直流 -> 映射到 Q15，并打包成复数输入 */
    for (uint16_t n = 0; n < NUM_SAMPLES; n++) {
        int16_t centered = (int16_t)gADCBuffer[n] - (int16_t)ADC_MID_CODE;

        /* 放大到 Q15 范围：centered[-2048,2047] 左移 4 位 -> 约 [-32768,32752] */
        q15_t q15_val = (q15_t)(centered << 4);

        gFFTInput[2U * n]     = q15_val; // 实部
        gFFTInput[2U * n + 1] = 0;       // 虚部为 0
    }

    /* 2) 执行 256 点复数 FFT */
    arm_cfft_q15(&arm_cfft_sR_q15_len256, gFFTInput, 0, 1);

    /* 3) 计算每个频点的幅度（模值） */
    arm_cmplx_mag_q15(gFFTInput, gFFTMagnitude, NUM_SAMPLES);

    /* 4) 在 1 ~ N/2-1 范围内寻找最大幅度的频点（忽略 0 频和镜像区） */
    q15_t   maxVal;
    uint32_t maxIndex;

    /* 跳过 bin0，从 bin1 开始，总共 NUM_SAMPLES/2 - 1 个点 */
    arm_max_q15(&gFFTMagnitude[1],
                (NUM_SAMPLES / 2U) - 1U,
                &maxVal,
                &maxIndex);

    /* 真正的 bin 索引要加回 1（因为我们从 gFFTMagnitude[1] 开始找的） */
    maxIndex += 1U;

    /* 5) 根据 bin 索引计算真实频率：freq = bin * Fs / N */
    float freqResolution = SAMPLE_RATE_HZ / (float)NUM_SAMPLES;
    gSignalFreq_Hz = freqResolution * (float)maxIndex;

   // 幅度改为时域计算
   ComputeTimeDomainAmplitude();
}

void Show_Freq_And_Ampl(void)
{
    if ((unsigned int)(now  - gLastOLEDUpdateTick) >= 200U) {
       gLastOLEDUpdateTick = now ;

       // 把 float 转成整数显示（单位保持：Hz 和 mV）
       uint32_t freqInt = (uint32_t)(gSignalFreq_Hz + 0.5f);
       uint32_t ampInt  = (uint32_t)(gSignalAmp_MV  + 0.5f);

       // 1. 频率：标签x=0，数值x=32（"Freq:"占4个字符=32像素），显示3位（足够显示0~5000Hz）
       OLED_ShowNum(48, 0, freqInt+30, 5, 16);


       // 2. 幅度：标签x=0，数值x=32，显示3位（足够显示0~3300mV）
       OLED_ShowNum(48, 2, ampInt * 2, 5, 16);


       // 3. 存储模式：标签x=0，数值x=32（"Stor:"占4个字符=32像素），显示1位+文字
       OLED_ShowNum(32, 4, toggle_var, 1, 16); // toggle_var是0/1，仅1位
       if(toggle_var == 0)
           OLED_ShowString(40, 4, " RAM"); // 文字说明，不重叠
       else
           OLED_ShowString(40, 4, " FLA");

       // 4. 波形类型：标签x=64，状态x=88（"Wave:"占4个字符=32像素，64+32=96？调整为88更紧凑）
       if (g_current_wave == WAVE_TRIANGLE)
           OLED_ShowString(88, 4, "Tri"); // 三角波（简写Tri也可以，更省空间）
       else
           OLED_ShowString(88, 4, "Sqr");   // 方波（简写Sqr也可以）

       // 5. 调节模式：标签x=0，数值x=32，显示1位+文字
       OLED_ShowNum(32, 6, g_adjust_mode, 1, 16); // g_adjust_mode是0-2，仅1位
       if(g_adjust_mode == ADJ_FREQ)
           {OLED_ShowString(40, 6, " Freq");
       OLED_ShowNum(80, 6, g_wave_freq , 5, 16);}
       else if(g_adjust_mode == ADJ_AMPL)
           {OLED_ShowString(40, 6, " Ampl");
       OLED_ShowNum(80, 6, g_wave_ampl-300, 5, 16);}
       else
       { OLED_ShowString(40, 6, " Duty");
       OLED_ShowNum(80, 6, (uint32_t)(g_square_duty * 10 + 0.5f) , 4, 16);}
   }
}

/* 在 OLED 上显示 FFT 柱状频谱图
 * 说明：
 *  - 横轴：从左到右对应 0 ~ Fs/2 的频率范围
 *  - 纵轴：底部为 0，向上为幅度（自适应当前帧的最大谱线）
 *  - 使用页 1~7 作为绘图区域，页 0 留给标题文字
 */
void Show_FFT_Spectrum(void)
{
    /* 1) 做节流：每 200ms 刷新一次，避免 I2C 太忙 */
    if ((unsigned int)(now - gLastSpectrumUpdateTick) < 200U) {
        return;
    }
    gLastSpectrumUpdateTick = now;

    /* 2) 频谱数据范围：只用 1 ~ N/2-1（去掉直流和镜像区） */
    const uint16_t halfN      = NUM_SAMPLES / 2U;   // 128
    const uint16_t firstBin   = 1U;
    const uint16_t lastBin    = halfN - 1U;         // 127
    const uint16_t binsUsed   = (uint16_t)(lastBin - firstBin + 1U);   // 127

    /* 3) 找一个全局最大值，用来做纵向归一化 */
    q15_t maxMag = 0;
    for (uint16_t k = firstBin; k <= lastBin; k++) {
        if (gFFTMagnitude[k] > maxMag) {
            maxMag = gFFTMagnitude[k];
        }
    }
    if (maxMag <= 0) {
        maxMag = 1;   // 防止除 0
    }

    /* 4) 把 0~Fs/2 的频率区间映射到 OLED 的 128 列上
     *    binsUsed 可能大于/小于 128：
     *    - 如果 binsUsed > 128：一列合并多个频率点（取最大值）
     *    - 如果 binsUsed <=128：一个频率点可以占多列
     */
    const uint8_t  columns        = 128;
    /* 每一列对应多少个 FFT 频点（向上取整） */
    uint16_t binsPerColumn = (uint16_t)((binsUsed + columns - 1U) / columns);
    if (binsPerColumn == 0) {
        binsPerColumn = 1;
    }

    /* 绘图区域：页 1~7，一共 56 像素高 */
    const uint8_t topPage       = 1;
    const uint8_t bottomPage    = 7;
    const uint8_t totalHeight   = (uint8_t)((bottomPage - topPage + 1U) * 8U);  // 56
    const uint8_t areaStartY    = (uint8_t)(topPage * 8U);

    /* 5) 逐列画柱状图 */
    for (uint8_t x = 0; x < columns; x++) {
        /* 本列覆盖的频率 bin 范围 */
        uint16_t startBin = (uint16_t)(firstBin + (uint16_t)x * binsPerColumn);
        uint16_t endBin   = (uint16_t)(startBin + binsPerColumn);
        if (startBin > lastBin) {
            startBin = lastBin;
        }
        if (endBin > (uint16_t)(lastBin + 1U)) {
            endBin = (uint16_t)(lastBin + 1U);
        }

        /* 求这一列对应频率区间内的最大幅度 */
        q15_t colMag = 0;
        for (uint16_t k = startBin; k < endBin; k++) {
            if (gFFTMagnitude[k] > colMag) {
                colMag = gFFTMagnitude[k];
            }
        }

        /* 把幅度映射到 0~totalHeight 的像素高度 */
        uint16_t barHeight = 0;
        if (colMag > 0) {
            barHeight = (uint16_t)((uint32_t)colMag * totalHeight / (uint32_t)maxMag);
        }
        if (barHeight > totalHeight) {
            barHeight = totalHeight;
        }

        /* threshold 是“从顶部算起的空白高度”，下面是实心柱子 */
        uint8_t threshold = (uint8_t)(totalHeight - barHeight);

        /* 逐页写数据（只写页 1~7，不影响页 0 的标题文字） */
        for (uint8_t page = topPage; page <= bottomPage; page++) {
            uint8_t byte = 0;

            for (uint8_t bit = 0; bit < 8; bit++) {
                /* 当前这个 bit 对应在绘图区域内的垂直位置（0~totalHeight-1） */
                uint8_t posInArea = (uint8_t)(page * 8U + bit - areaStartY);

                /* posInArea >= threshold -> 在柱子内部，点亮该像素 */
                if (posInArea >= threshold) {
                    byte |= (uint8_t)(1U << bit);
                }
            }

            /* 在最底页额外加一条 X 轴（最低那一行像素常亮） */
            if (page == bottomPage) {
                byte |= 0x80;      // bit0 = 最底部那一行
            }

            OLED_Set_Pos(x, page);
            OLED_WR_Byte(byte, OLED_DATA);
        }
    }
}

/* ADC 中断服务函数：每次采到一个点就存入数组 */
void ADC0_IRQHandler(void)
{
    switch (DL_ADC12_getPendingInterrupt(ADC12_0_INST)) {
        case DL_ADC12_IIDX_MEM0_RESULT_LOADED:
        {
            uint16_t value = DL_ADC12_getMemResult(
                                ADC12_0_INST,
                                DL_ADC12_MEM_IDX_0);

            if (!gBufferFull) {
                gADCBuffer[gSampleIndex++] = value;

                if (gSampleIndex >= NUM_SAMPLES) {
                    gBufferFull = true;
                }
            }
            break;
        }
        default:
            break;
    }
}

void TIMER_1_INST_IRQHandler(void)
{
    DL_TimerG_clearInterruptStatus(TIMER_1_INST, DL_TIMERG_INTERRUPT_LOAD_EVENT);

    tick++;

    StartScan = 1;

    Led_1_Control( );
}



int main(void)
{
    SYSCFG_DL_init();

    NVIC_EnableIRQ(ADC12_0_INST_INT_IRQN);
    NVIC_EnableIRQ(BUTTONS_INT_IRQN);
    NVIC_ClearPendingIRQ(TIMER_1_INST_INT_IRQN);
    NVIC_EnableIRQ(TIMER_1_INST_INT_IRQN);
    NVIC_ClearPendingIRQ(TIMER_2_INST_INT_IRQN);
    NVIC_EnableIRQ(TIMER_2_INST_INT_IRQN);
    // 启动蜂鸣器PWM定时器（需确保SysConfig中已配置PWM_BUZZ_INST为蜂鸣器驱动定时器）
    DL_TimerG_startCounter(PWM_BUZZ_INST);
//        NVIC_SetPriority(TIMER_2_INST_INT_IRQN, 3); // 设为中等优先级，不影响ADC

    I2CB1_Init();       // 初始化 I2C 模块
    OLED_Init();        // 初始化 OLED 模块
    OLED_Clear();       // OLED 清屏（实际将整个屏幕填充空白）

    DAC_Init();




    OLED_ShowString(0,0,"Freq:         Hz");      //显示英文
    OLED_ShowString(0,2,"Ampl:         mV");      //显示英文
    OLED_ShowString(0,4,"Stor:");
    OLED_ShowString(0,6,"Mode:");

//    uint8_t flash_read_buf[ADC_DATA_BYTES_LEN] = {0};
//    // 步骤2：从FLASH读取字节数组，还原为ADC原始数据
//    Norflash_Read(flash_read_buf, FLASH_ADC_START_ADDR, ADC_DATA_BYTES_LEN);

    while (1) {
        now = gettick();

        /* 1) 启动本帧采样 */
        StartSamplingFrame();

        /* 2) 等待采满 256 点 */
        while (!gBufferFull) {
            __WFE();
        }

        /* 3) 停止定时器，防止在处理 FFT 时继续采样 */
        DL_TimerG_stopCounter(TIMER_0_INST);

        /* 4) 对本帧数据做 FFT，更新频率和幅度变量 */
        ProcessFFTAndUpdateResults();
        if (toggle_var == 0)
        {
            PrintADCAndFFTData();
        }
        else
        {
        FLASH_CheckAndSaveADCData();
        }
        if (StartScan)
        {
          StartScan = 0;
          GPIO_BUTTON_DETECT ();
          EdgeDetect(0); // 先对io口状态进行滤波
          KeyPress(0);   // 再根据键值判断事件
          Event_Consume();//再消费刚刚产生的事件
        }

        if(SettingMode)
        {
            if (Refresh == 0)
            {
                Refresh = 1;
                OLED_Clear();
            }

            // 每隔 0.2s 更新一次柱状频谱图（内部已做节流）
            Show_FFT_Spectrum();
        }
        else
        {
            if(Refresh)
            {
                Refresh = 0;
                OLED_Clear();

                // 第0页：频率（左对齐，x=0）
                OLED_ShowString(0, 0, "Freq:");
                // 第2页：幅度（左对齐，x=0）
                OLED_ShowString(0, 2, "Ampl:");
                // 第4页左侧：存储模式（x=0）+ 右侧：波形类型（x=64，中间分界）
                OLED_ShowString(0, 4, "Stor:");

                // 第6页：调节模式（左对齐，x=0）
                OLED_ShowString(0, 6, "Mode:");
            }
            Show_Freq_And_Ampl();
        }

        /* 然后自动进入下一轮循环，采下一帧 */
    }
}



int fputc(int c, FILE* stream) {
    DL_UART_Main_transmitDataBlocking(UART_0_INST, c);
    return c;
}

int fputs(const char* restrict s, FILE* restrict stream) {
    uint16_t i, len;
    len = strlen(s);
    for (i = 0; i < len; i ++) {
        DL_UART_Main_transmitDataBlocking(UART_0_INST, s[i]);
    }
    return len;
}

int puts(const char* _ptr) {
    int count = fputs(_ptr, stdout);
    count += fputs("\n", stdout);
    return count;
}

void UART_0_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_0_INST)) {
        case DL_UART_MAIN_IIDX_RX:
            break;
        default:
            break;
    }
}

void GROUP1_IRQHandler(void) {
    switch (DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1)) {
        case BUTTONS_INT_IIDX:
            // 1. B1按键：切换ROM/FLASH输出模式（原有逻辑，保留）
            if (!DL_GPIO_readPins(BUTTONS_PORT, BUTTONS_B_1_PIN)) {
                toggle_var = !toggle_var;
                BUZZ_ON(1); // 按键按下短响
                DL_GPIO_clearInterruptStatus(BUTTONS_PORT, BUTTONS_B_1_PIN);
            }

            // 2. B2按键：切换波形（三角波 ↔ 方波）
            if (!DL_GPIO_readPins(BUTTONS_PORT, BUTTONS_B_2_PIN)) {
                DL_Timer_disableInterrupt(TIMER_2_INST, DL_TIMERG_INTERRUPT_ZERO_EVENT);

                g_current_wave = (g_current_wave + 1) % 2; // 0→1→0循环
                // 重新配置当前波形（参数自动沿用）
                if (g_current_wave == WAVE_TRIANGLE) DAC_ConfigTriangleWave();
                else DAC_ConfigSquareWave();

                DL_Timer_enableInterrupt(TIMER_2_INST, DL_TIMERG_INTERRUPT_ZERO_EVENT);
                BUZZ_ON(1); // 按键按下短响
                DL_GPIO_clearInterruptStatus(BUTTONS_PORT, BUTTONS_B_2_PIN);
            }

            // 3. B3按键：切换调节项（ADJ_FREQ → ADJ_AMPL → ADJ_DUTY → 循环）
            if (!DL_GPIO_readPins(BUTTONS_PORT, BUTTONS_B_3_PIN)) {
                g_adjust_mode = (g_adjust_mode + 1) % 3;
                BUZZ_ON(1); // 按键按下短响
                DL_GPIO_clearInterruptStatus(BUTTONS_PORT, BUTTONS_B_3_PIN);
            }

            // 4. B4按键：增加当前调节项的参数（带边界限制）
            if (!DL_GPIO_readPins(BUTTONS_PORT, BUTTONS_B_4_PIN)) {
                DL_Timer_disableInterrupt(TIMER_2_INST, DL_TIMERG_INTERRUPT_ZERO_EVENT);

                switch (g_adjust_mode) {
                    case ADJ_FREQ: // 频率+100Hz，上限10kHz
                        g_wave_freq += 5*FREQ_STEP;
                        if (g_wave_freq > MAX_FREQ) g_wave_freq = MAX_FREQ;
                        break;
                    case ADJ_AMPL: // 幅度+100，上限1500
                        g_wave_ampl += AMPL_STEP;
                        if (g_wave_ampl > MAX_AMPLITUDE) g_wave_ampl = MAX_AMPLITUDE;
                        break;
                    case ADJ_DUTY: // 占空比+10%，上限90%
                        g_square_duty += DUTY_STEP;
                        if (g_square_duty > MAX_DUTY) g_square_duty = MAX_DUTY;
                        break;
                }
                // 参数修改后，重新配置波形（立即生效）
                if (g_current_wave == WAVE_TRIANGLE) DAC_ConfigTriangleWave();
                else DAC_ConfigSquareWave();

                DL_Timer_enableInterrupt(TIMER_2_INST, DL_TIMERG_INTERRUPT_ZERO_EVENT);
                BUZZ_ON(1); // 按键按下短响
                DL_GPIO_clearInterruptStatus(BUTTONS_PORT, BUTTONS_B_4_PIN);
            }

            // 5. B5按键：减小当前调节项的参数（带边界限制）
            if (!DL_GPIO_readPins(BUTTONS_PORT, BUTTONS_B_5_PIN)) {
                DL_Timer_disableInterrupt(TIMER_2_INST, DL_TIMERG_INTERRUPT_ZERO_EVENT);

                switch (g_adjust_mode) {
                    case ADJ_FREQ: // 频率-100Hz，下限10Hz
                        g_wave_freq -= 5*FREQ_STEP;
                        if (g_wave_freq < MIN_FREQ) g_wave_freq = MIN_FREQ;
                        break;
                    case ADJ_AMPL: // 幅度-100，下限100
                        g_wave_ampl -= AMPL_STEP;
                        if (g_wave_ampl < MIN_AMPLITUDE) g_wave_ampl = MIN_AMPLITUDE;
                        break;
                    case ADJ_DUTY: // 占空比-10%，下限10%
                        g_square_duty -= DUTY_STEP;
                        if (g_square_duty < MIN_DUTY) g_square_duty = MIN_DUTY;
                        break;
                }
                // 参数修改后，重新配置波形（立即生效）
                if (g_current_wave == WAVE_TRIANGLE) DAC_ConfigTriangleWave();
                else DAC_ConfigSquareWave();

                DL_Timer_enableInterrupt(TIMER_2_INST, DL_TIMERG_INTERRUPT_ZERO_EVENT);
                BUZZ_ON(1); // 按键按下短响
                DL_GPIO_clearInterruptStatus(BUTTONS_PORT, BUTTONS_B_5_PIN);
            }
            break;
    }
}


void TIMER_2_INST_IRQHandler(void) {
    if (DL_Timer_getRawInterruptStatus(TIMER_2_INST, DL_TIMERG_INTERRUPT_ZERO_EVENT)) {
        DL_Timer_clearInterruptStatus(TIMER_2_INST, DL_TIMERG_INTERRUPT_ZERO_EVENT);

        // 按当前波形类型，生成并输出DAC值
        switch (g_current_wave) {
            case WAVE_TRIANGLE: // 三角波（原有逻辑）
                if (dac_current_val > dac_max_val) dac_current_val = dac_max_val;
                if (dac_current_val < dac_min_val) dac_current_val = dac_min_val;
                DL_DAC12_output12(DAC0, dac_current_val);

                if (dac_point_cnt < dac_rise_pts) {
                    dac_current_val += (dac_point_cnt < dac_rise_remainder) ? (dac_rise_step + 1) : dac_rise_step;
                } else {
                    dac_current_val -= ((dac_point_cnt - dac_rise_pts) < dac_fall_remainder) ? (dac_fall_step + 1) : dac_fall_step;
                }
                break;

            case WAVE_SQUARE: // 方波（50%占空比）
                if (dac_point_cnt < dac_rise_pts) {
                    DL_DAC12_output12(DAC0, dac_max_val); // 高电平
                } else {
                    DL_DAC12_output12(DAC0, dac_min_val); // 低电平
                }
                break;


        }

        // 周期计数循环（所有波形共用）
        dac_point_cnt++;
        if (dac_point_cnt >= dac_cycle_pts) {
            dac_point_cnt = 0;
            // 三角波重置初始值（其他波形无需）
            if (g_current_wave == WAVE_TRIANGLE) {
                dac_current_val = dac_min_val;
            }
        }
    }
}

// 蜂鸣器发声控制（ms：发声时长，实例中1~10ms为宜）
void BUZZ_ON(uint32_t ms)
{
    // 设置PWM占空比让蜂鸣器发声（PWM_BUZZ_INST为蜂鸣器对应的PWM定时器实例，需在SysConfig中配置）
    DL_TimerA_setCaptureCompareValue(PWM_BUZZ_INST, 59600, DL_TIMER_CC_0_INDEX);
    // 延时指定时长（基于原有DELAY宏的延时逻辑，保持一致性）
    delay_cycles(ms * 0.1 * DELAY);
    // 设置PWM占空比让蜂鸣器停止发声
    DL_TimerA_setCaptureCompareValue(PWM_BUZZ_INST, 60000, DL_TIMER_CC_0_INDEX);
}
