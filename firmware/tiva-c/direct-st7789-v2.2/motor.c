/*
 * Tiva Claw Arm Controller v2.2 - motor.c
 *
 * Bare-metal ADC, PWM, GPIO interrupt and SysTick implementation.
 * The system clock is configured to 80 MHz in main.c.
 *
 * Servo outputs:
 *   Servo A  -> PE4 / M1PWM2
 *   Servo B1 -> PE5 / M1PWM3
 *   Servo B2 -> PF3 / M1PWM7, opposite command to B1
 *   Servo C  -> PF2 / M1PWM6
 *
 * Diagnostic output:
 *   PF1 -> LaunchPad red LED heartbeat
 */
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "motor.h"

#define PWM_PERIOD      25000UL   /* 20 ms at 1.25 MHz */
#define PWM_PERIOD_MAX   3000UL   /* 2.4 ms */
#define PWM_PERIOD_MIN    750UL   /* 0.6 ms */
#define ADC_MAX_VALUE     4095UL

volatile uint32_t systick_ms = 0U;
volatile uint8_t button_flag = 0U;

static uint16_t ClampADC(uint16_t value)
{
    return (value > ADC_MAX_VALUE) ? (uint16_t)ADC_MAX_VALUE : value;
}

static uint32_t ADCToPulseTicks(uint16_t adc_value)
{
    uint32_t value = ClampADC(adc_value);

    return PWM_PERIOD_MIN +
        (((PWM_PERIOD_MAX - PWM_PERIOD_MIN) * value) / ADC_MAX_VALUE);
}

static void SysTick_Init(void)
{
    NVIC_ST_CTRL_R    = 0U;
    NVIC_ST_RELOAD_R  = 79999U;
    NVIC_ST_CURRENT_R = 0U;
    NVIC_ST_CTRL_R    = 0x07U;
}

void SysTick_Handler(void)
{
    systick_ms++;
}

void Delay_ms(uint32_t ms)
{
    uint32_t start = systick_ms;
    while ((systick_ms - start) < ms) {}
}

static void ADC_Init(void)
{
    SYSCTL_RCGCGPIO_R |= 0x00000008U;       /* Port D */
    while ((SYSCTL_PRGPIO_R & 0x08U) == 0U) {}

    GPIO_PORTD_DIR_R   &= ~0x03U;
    GPIO_PORTD_AFSEL_R |=  0x03U;
    GPIO_PORTD_DEN_R   &= ~0x03U;
    GPIO_PORTD_AMSEL_R |=  0x03U;

    SYSCTL_RCGCADC_R |= 0x01U;              /* ADC0 */
    while ((SYSCTL_PRADC_R & 0x01U) == 0U) {}

    ADC0_PC_R     = (ADC0_PC_R & ~0x0FU) | 0x01U; /* 125 ksps */
    ADC0_SSPRI_R  = 0x0123U;
    ADC0_ACTSS_R &= ~0x08U;
    ADC0_EMUX_R  &= ~0xF000U;
    ADC0_SSMUX3_R = 7U;
    ADC0_SSCTL3_R = 0x06U;
    ADC0_ACTSS_R |= 0x08U;
}

uint16_t ADC_Read(uint8_t channel)
{
    uint16_t result;

    ADC0_ACTSS_R  &= ~0x08U;
    ADC0_SSMUX3_R  = channel & 0x0FU;
    ADC0_ISC_R     = 0x08U;
    ADC0_ACTSS_R  |= 0x08U;
    ADC0_PSSI_R    = 0x08U;

    while ((ADC0_RIS_R & 0x08U) == 0U) {}

    result = (uint16_t)(ADC0_SSFIFO3_R & 0x0FFFU);
    ADC0_ISC_R = 0x08U;

    return result;
}

static void PortE_Init(void)
{
    SYSCTL_RCGCGPIO_R |= 0x00000010U;
    while ((SYSCTL_PRGPIO_R & 0x10U) == 0U) {}

    GPIO_PORTE_AFSEL_R |= 0x30U;
    GPIO_PORTE_PCTL_R =
        (GPIO_PORTE_PCTL_R & ~0x00FF0000U) | 0x00550000U;
    GPIO_PORTE_AMSEL_R &= ~0x30U;
    GPIO_PORTE_DEN_R   |=  0x30U;
    GPIO_PORTE_DIR_R   |=  0x30U;
}

static void PortF_Init(void)
{
    SYSCTL_RCGCGPIO_R |= 0x00000020U;
    while ((SYSCTL_PRGPIO_R & 0x20U) == 0U) {}

    GPIO_PORTF_LOCK_R = 0x4C4F434BU;
    GPIO_PORTF_CR_R  |= 0x1FU;

    /* PF2/PF3: PWM outputs. */
    GPIO_PORTF_AFSEL_R |= 0x0CU;
    GPIO_PORTF_PCTL_R =
        (GPIO_PORTF_PCTL_R & ~0x0000FF00U) | 0x00005500U;
    GPIO_PORTF_AMSEL_R &= ~0x0CU;
    GPIO_PORTF_DEN_R   |=  0x0CU;
    GPIO_PORTF_DIR_R   |=  0x0CU;

    /* PF1: red status LED. */
    GPIO_PORTF_AFSEL_R &= ~0x02U;
    GPIO_PORTF_PCTL_R  &= ~0x000000F0U;
    GPIO_PORTF_AMSEL_R &= ~0x02U;
    GPIO_PORTF_DEN_R   |=  0x02U;
    GPIO_PORTF_DIR_R   |=  0x02U;
    GPIO_PORTF_DATA_R  &= ~0x02U;

    /* PF4: active-low button with internal pull-up. */
    GPIO_PORTF_AFSEL_R &= ~0x10U;
    GPIO_PORTF_PCTL_R  &= ~0x000F0000U;
    GPIO_PORTF_AMSEL_R &= ~0x10U;
    GPIO_PORTF_DIR_R   &= ~0x10U;
    GPIO_PORTF_PUR_R   |=  0x10U;
    GPIO_PORTF_DEN_R   |=  0x10U;

    GPIO_PORTF_IS_R  &= ~0x10U;
    GPIO_PORTF_IBE_R &= ~0x10U;
    GPIO_PORTF_IEV_R &= ~0x10U;
    GPIO_PORTF_ICR_R  =  0x10U;
    GPIO_PORTF_IM_R  |=  0x10U;

    NVIC_PRI7_R = (NVIC_PRI7_R & 0xFF00FFFFU) | 0x00A00000U;
    NVIC_EN0_R |= (1UL << 30);
}

static void PWM_Init(void)
{
    SYSCTL_RCGCPWM_R |= 0x02U;
    while ((SYSCTL_PRPWM_R & 0x02U) == 0U) {}

    SYSCTL_RCC_R = (SYSCTL_RCC_R & ~SYSCTL_RCC_PWMDIV_M)
                 | SYSCTL_RCC_PWMDIV_64
                 | SYSCTL_RCC_USEPWMDIV;

    PWM1_1_CTL_R  = 0U;
    PWM1_1_GENA_R = 0x0000008CU;
    PWM1_1_GENB_R = 0x0000080CU;
    PWM1_1_LOAD_R = PWM_PERIOD - 1U;
    PWM1_1_CMPA_R = PWM_PERIOD - 1U;
    PWM1_1_CMPB_R = PWM_PERIOD - 1U;
    PWM1_1_CTL_R  = 0x00000001U;

    PWM1_3_CTL_R  = 0U;
    PWM1_3_GENA_R = 0x0000008CU;
    PWM1_3_GENB_R = 0x0000080CU;
    PWM1_3_LOAD_R = PWM_PERIOD - 1U;
    PWM1_3_CMPA_R = PWM_PERIOD - 1U;
    PWM1_3_CMPB_R = PWM_PERIOD - 1U;
    PWM1_3_CTL_R  = 0x00000001U;

    PWM1_ENABLE_R |= 0xCCU;
}

void GPIOPortF_Handler(void)
{
    static uint32_t last_press = 0U;

    GPIO_PORTF_ICR_R = 0x10U;

    if ((systick_ms - last_press) < 50U) return;

    last_press = systick_ms;
    button_flag ^= 1U;
}

void PWM_SetADC_A(uint16_t adc_value)
{
    uint32_t pulse_ticks = ADCToPulseTicks(adc_value);
    PWM1_1_CMPA_R = (PWM_PERIOD - 1U) - pulse_ticks;
}

void PWM_SetADC_BPair(uint16_t adc_value)
{
    uint16_t value = ClampADC(adc_value);
    uint32_t b1_ticks = ADCToPulseTicks(value);
    uint32_t b2_ticks = ADCToPulseTicks((uint16_t)(ADC_MAX_VALUE - value));

    PWM1_1_CMPB_R = (PWM_PERIOD - 1U) - b1_ticks;
    PWM1_3_CMPB_R = (PWM_PERIOD - 1U) - b2_ticks;
}

void PWM_SetDuty_C(uint8_t state)
{
    uint32_t pulse_ticks =
        (state != 0U) ? PWM_PERIOD_MAX : PWM_PERIOD_MIN;

    PWM1_3_CMPA_R = (PWM_PERIOD - 1U) - pulse_ticks;
}

void StatusLED_Set(uint8_t on)
{
    if (on != 0U) {
        GPIO_PORTF_DATA_R |= 0x02U;
    } else {
        GPIO_PORTF_DATA_R &= ~0x02U;
    }
}

void StatusLED_Toggle(void)
{
    GPIO_PORTF_DATA_R ^= 0x02U;
}

void Motor_Init(void)
{
    SysTick_Init();
    PortE_Init();
    PortF_Init();
    PWM_Init();
    ADC_Init();

    PWM_SetADC_A(2048U);
    PWM_SetADC_BPair(2048U);
    PWM_SetDuty_C(0U);

    __asm(" CPSIE I");
}
