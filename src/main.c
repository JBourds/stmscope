#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "defs.h"
#include "main.h"

#include "stm32f4xx_hal.h"

UART_HandleTypeDef huart2;
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
TIM_HandleTypeDef htim3;

#define SZ 1024
#define VOLTAGE_MAX 3.3
#define ADC_MAX 4095.0

#define LED_PIN GPIO_PIN_5

typedef struct {
    _Bool active_buf : 1;
    _Bool dma_complete : 1;
} Flags;

volatile Flags FLAGS;
volatile u16 ADC_SAMPLES[2 * SZ];

static void sysclock_init(void);
static void gpio_init(void);
static void uart2_init(void);
static void adc_init(void);
static void dma_init(void);
static void handle_error(void);

int _write(int file, char *ptr, int len);

double adc_to_voltage(u16 val) { return VOLTAGE_MAX * val / ADC_MAX; }

static void toggle_led() { HAL_GPIO_TogglePin(LD2_GPIO_Port, LED_PIN); }

int main(void) {
    HAL_Init();
    sysclock_init();
    gpio_init();
    uart2_init();
    dma_init();
    adc_init();
    setbuf(stdout, NULL);
    printf("starting!\n");
    HAL_ADC_Start_DMA(&hadc1, (u32 *)ADC_SAMPLES, 2 * SZ);

    while (1) {
        toggle_led();
        if (FLAGS.dma_complete) {
            u64 sum = 0;
            u16 *buf = ADC_SAMPLES + FLAGS.active_buf * SZ;
            for (usize i = 0; i < SZ; ++i) {
                sum += buf[i];
            }
            u16 avg = sum / SZ;
            double avg_voltage = adc_to_voltage(avg);
            printf("avg voltage: %lfV\n", avg_voltage);
        } else {
            printf("not yet\n");
        }
        HAL_Delay(500);
    }
}

void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (hadc->Instance == ADC1) {
        __HAL_RCC_ADC1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_0;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

static void adc_init() {
    ADC_ChannelConfTypeDef sConfig = {0};

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = ENABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = ENABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        printf("error initializing ADC\n");
        handle_error();
    }

    /** Configure for the selected ADC regular channel its corresponding rank in
     * the sequencer and its sample time. */
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        printf("error initializing ADC\n");
        handle_error();
    }
}

static void sysclock_init(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        printf("error initializing sysclock\n");
        handle_error();
    }

    /** Initializes the CPU, AHB and APB buses clocks */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        printf("error initializing sysclock\n");
        handle_error();
    }
}

static void uart2_init(void) {
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        handle_error();
    }
}

static void gpio_init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin : B1_Pin */
    GPIO_InitStruct.Pin = B1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : LD2_Pin */
    GPIO_InitStruct.Pin = LD2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);
}

int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

static void dma_init(void) {
    /* DMA controller clock enable */
    __HAL_RCC_DMA2_CLK_ENABLE();

    /* Configure DMA for ADC1 */
    hdma_adc1.Instance = DMA2_Stream0;
    hdma_adc1.Init.Channel = DMA_CHANNEL_0;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode = DMA_CIRCULAR;
    hdma_adc1.Init.Priority = DMA_PRIORITY_LOW;
    hdma_adc1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_adc1);

    /* Associate the DMA handle with the ADC handle */
    __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);

    /* DMA interrupt init */
    /* DMA2_Stream0_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
    if (hadc->Instance == ADC1) {
        FLAGS.dma_complete = 1;
    } else {
        handle_error();
    }
}

void DMA2_Stream0_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_adc1); }

static void handle_error(void) {
    __disable_irq();
    while (1) {
        toggle_led();
        HAL_Delay(500);
    }
}
