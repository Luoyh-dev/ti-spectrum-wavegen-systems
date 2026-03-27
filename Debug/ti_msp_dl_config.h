/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)



#define CPUCLK_FREQ                                                     32000000



/* Defines for PWM_BUZZ */
#define PWM_BUZZ_INST                                                      TIMG8
#define PWM_BUZZ_INST_IRQHandler                                TIMG8_IRQHandler
#define PWM_BUZZ_INST_INT_IRQN                                  (TIMG8_INT_IRQn)
#define PWM_BUZZ_INST_CLK_FREQ                                          16000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_BUZZ_C0_PORT                                              GPIOB
#define GPIO_PWM_BUZZ_C0_PIN                                      DL_GPIO_PIN_15
#define GPIO_PWM_BUZZ_C0_IOMUX                                   (IOMUX_PINCM32)
#define GPIO_PWM_BUZZ_C0_IOMUX_FUNC                  IOMUX_PINCM32_PF_TIMG8_CCP0
#define GPIO_PWM_BUZZ_C0_IDX                                 DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_BUZZ_C1_PORT                                              GPIOA
#define GPIO_PWM_BUZZ_C1_PIN                                       DL_GPIO_PIN_0
#define GPIO_PWM_BUZZ_C1_IOMUX                                    (IOMUX_PINCM1)
#define GPIO_PWM_BUZZ_C1_IOMUX_FUNC                   IOMUX_PINCM1_PF_TIMG8_CCP1
#define GPIO_PWM_BUZZ_C1_IDX                                 DL_TIMER_CC_1_INDEX



/* Defines for TIMER_0 */
#define TIMER_0_INST                                                     (TIMG0)
#define TIMER_0_INST_IRQHandler                                 TIMG0_IRQHandler
#define TIMER_0_INST_INT_IRQN                                   (TIMG0_INT_IRQn)
#define TIMER_0_INST_LOAD_VALUE                                            (99U)
#define TIMER_0_INST_PUB_0_CH                                                (1)
/* Defines for TIMER_1 */
#define TIMER_1_INST                                                     (TIMG7)
#define TIMER_1_INST_IRQHandler                                 TIMG7_IRQHandler
#define TIMER_1_INST_INT_IRQN                                   (TIMG7_INT_IRQn)
#define TIMER_1_INST_LOAD_VALUE                                         (31999U)
/* Defines for TIMER_2 */
#define TIMER_2_INST                                                     (TIMG6)
#define TIMER_2_INST_IRQHandler                                 TIMG6_IRQHandler
#define TIMER_2_INST_INT_IRQN                                   (TIMG6_INT_IRQn)
#define TIMER_2_INST_LOAD_VALUE                                            (39U)
#define TIMER_2_INST_PUB_0_CH                                                (2)



/* Defines for UART_0 */
#define UART_0_INST                                                        UART0
#define UART_0_INST_FREQUENCY                                           32000000
#define UART_0_INST_IRQHandler                                  UART0_IRQHandler
#define UART_0_INST_INT_IRQN                                      UART0_INT_IRQn
#define GPIO_UART_0_RX_PORT                                                GPIOA
#define GPIO_UART_0_TX_PORT                                                GPIOA
#define GPIO_UART_0_RX_PIN                                        DL_GPIO_PIN_11
#define GPIO_UART_0_TX_PIN                                        DL_GPIO_PIN_10
#define GPIO_UART_0_IOMUX_RX                                     (IOMUX_PINCM22)
#define GPIO_UART_0_IOMUX_TX                                     (IOMUX_PINCM21)
#define GPIO_UART_0_IOMUX_RX_FUNC                      IOMUX_PINCM22_PF_UART0_RX
#define GPIO_UART_0_IOMUX_TX_FUNC                      IOMUX_PINCM21_PF_UART0_TX
#define UART_0_BAUD_RATE                                                  (9600)
#define UART_0_IBRD_32_MHZ_9600_BAUD                                       (208)
#define UART_0_FBRD_32_MHZ_9600_BAUD                                        (21)




/* Defines for SPI1 */
#define SPI1_INST                                                          SPI0
#define SPI1_INST_IRQHandler                                    SPI0_IRQHandler
#define SPI1_INST_INT_IRQN                                        SPI0_INT_IRQn
#define GPIO_SPI1_PICO_PORT                                               GPIOB
#define GPIO_SPI1_PICO_PIN                                       DL_GPIO_PIN_17
#define GPIO_SPI1_IOMUX_PICO                                    (IOMUX_PINCM43)
#define GPIO_SPI1_IOMUX_PICO_FUNC                    IOMUX_PINCM43_PF_SPI0_PICO
#define GPIO_SPI1_POCI_PORT                                               GPIOB
#define GPIO_SPI1_POCI_PIN                                       DL_GPIO_PIN_19
#define GPIO_SPI1_IOMUX_POCI                                    (IOMUX_PINCM45)
#define GPIO_SPI1_IOMUX_POCI_FUNC                    IOMUX_PINCM45_PF_SPI0_POCI
/* GPIO configuration for SPI1 */
#define GPIO_SPI1_SCLK_PORT                                               GPIOB
#define GPIO_SPI1_SCLK_PIN                                       DL_GPIO_PIN_18
#define GPIO_SPI1_IOMUX_SCLK                                    (IOMUX_PINCM44)
#define GPIO_SPI1_IOMUX_SCLK_FUNC                    IOMUX_PINCM44_PF_SPI0_SCLK



/* Defines for ADC12_0 */
#define ADC12_0_INST                                                        ADC0
#define ADC12_0_INST_IRQHandler                                  ADC0_IRQHandler
#define ADC12_0_INST_INT_IRQN                                    (ADC0_INT_IRQn)
#define ADC12_0_ADCMEM_0                                      DL_ADC12_MEM_IDX_0
#define ADC12_0_ADCMEM_0_REF                     DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define ADC12_0_ADCMEM_0_REF_VOLTAGE_V                                       3.3
#define ADC12_0_INST_SUB_CH                                                  (1)
#define GPIO_ADC12_0_C3_PORT                                               GPIOA
#define GPIO_ADC12_0_C3_PIN                                       DL_GPIO_PIN_24
#define GPIO_ADC12_0_IOMUX_C3                                    (IOMUX_PINCM54)
#define GPIO_ADC12_0_IOMUX_C3_FUNC                (IOMUX_PINCM54_PF_UNCONNECTED)



/* Port definition for Pin Group BUTTON_PORTA */
#define BUTTON_PORTA_PORT                                                (GPIOA)

/* Defines for S5: GPIOA.18 with pinCMx 40 on package pin 11 */
#define BUTTON_PORTA_S5_PIN                                     (DL_GPIO_PIN_18)
#define BUTTON_PORTA_S5_IOMUX                                    (IOMUX_PINCM40)
/* Port definition for Pin Group GPIO_LEDS */
#define GPIO_LEDS_PORT                                                   (GPIOB)

/* Defines for USER_TEST: GPIOB.16 with pinCMx 33 on package pin 4 */
#define GPIO_LEDS_USER_TEST_PIN                                 (DL_GPIO_PIN_16)
#define GPIO_LEDS_USER_TEST_IOMUX                                (IOMUX_PINCM33)
/* Port definition for Pin Group SPI1_GPIO */
#define SPI1_GPIO_PORT                                                   (GPIOB)

/* Defines for CS: GPIOB.20 with pinCMx 48 on package pin 19 */
#define SPI1_GPIO_CS_PIN                                        (DL_GPIO_PIN_20)
#define SPI1_GPIO_CS_IOMUX                                       (IOMUX_PINCM48)
/* Port definition for Pin Group BUTTONS */
#define BUTTONS_PORT                                                     (GPIOB)

/* Defines for B_1: GPIOB.21 with pinCMx 49 on package pin 20 */
// pins affected by this interrupt request:["B_1","B_2","B_3","B_4","B_5"]
#define BUTTONS_INT_IRQN                                        (GPIOB_INT_IRQn)
#define BUTTONS_INT_IIDX                        (DL_INTERRUPT_GROUP1_IIDX_GPIOB)
#define BUTTONS_B_1_IIDX                                    (DL_GPIO_IIDX_DIO21)
#define BUTTONS_B_1_PIN                                         (DL_GPIO_PIN_21)
#define BUTTONS_B_1_IOMUX                                        (IOMUX_PINCM49)
/* Defines for B_2: GPIOB.7 with pinCMx 24 on package pin 59 */
#define BUTTONS_B_2_IIDX                                     (DL_GPIO_IIDX_DIO7)
#define BUTTONS_B_2_PIN                                          (DL_GPIO_PIN_7)
#define BUTTONS_B_2_IOMUX                                        (IOMUX_PINCM24)
/* Defines for B_3: GPIOB.6 with pinCMx 23 on package pin 58 */
#define BUTTONS_B_3_IIDX                                     (DL_GPIO_IIDX_DIO6)
#define BUTTONS_B_3_PIN                                          (DL_GPIO_PIN_6)
#define BUTTONS_B_3_IOMUX                                        (IOMUX_PINCM23)
/* Defines for B_4: GPIOB.8 with pinCMx 25 on package pin 60 */
#define BUTTONS_B_4_IIDX                                     (DL_GPIO_IIDX_DIO8)
#define BUTTONS_B_4_PIN                                          (DL_GPIO_PIN_8)
#define BUTTONS_B_4_IOMUX                                        (IOMUX_PINCM25)
/* Defines for B_5: GPIOB.9 with pinCMx 26 on package pin 61 */
#define BUTTONS_B_5_IIDX                                     (DL_GPIO_IIDX_DIO9)
#define BUTTONS_B_5_PIN                                          (DL_GPIO_PIN_9)
#define BUTTONS_B_5_IOMUX                                        (IOMUX_PINCM26)



/* Defines for DAC12 */
#define DAC12_IRQHandler                                         DAC0_IRQHandler
#define DAC12_INT_IRQN                                           (DAC0_INT_IRQn)
#define GPIO_DAC12_OUT_PORT                                                GPIOA
#define GPIO_DAC12_OUT_PIN                                        DL_GPIO_PIN_15
#define GPIO_DAC12_IOMUX_OUT                                     (IOMUX_PINCM37)
#define GPIO_DAC12_IOMUX_OUT_FUNC                   IOMUX_PINCM37_PF_UNCONNECTED


/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_PWM_BUZZ_init(void);
void SYSCFG_DL_TIMER_0_init(void);
void SYSCFG_DL_TIMER_1_init(void);
void SYSCFG_DL_TIMER_2_init(void);
void SYSCFG_DL_UART_0_init(void);
void SYSCFG_DL_SPI1_init(void);
void SYSCFG_DL_ADC12_0_init(void);

void SYSCFG_DL_DAC12_init(void);

bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
