#include <stdio.h>
#include <string.h>

#include "defs.h"
#include "main.h"

#include "display.h"
#include "probe.h"
#include "serial.h"
#include "stm32f4xx_hal.h"

#define SZ 1024
#define VOLTAGE_MAX 3.3
#define ADC_MAX 4095.0

#define LED_PIN GPIO_PIN_5

static struct {
    _Bool dma_complete : 1;
    u16 *buf;
    usize sz;
} STATE;

volatile u16 ADC_SAMPLES[SZ];

static void sysclock_init(void);
static void gpio_init(void);
static void handle_error(void);

double adc_to_voltage(u16 val) { return VOLTAGE_MAX * val / ADC_MAX; }

static void toggle_led() { HAL_GPIO_TogglePin(LD2_GPIO_Port, LED_PIN); }

int main(void) {
    RC rc;
    HAL_Init();
    sysclock_init();
    gpio_init();

    setbuf(stdout, NULL);
    if (serial_init() != RC_OK) {
        handle_error();
    }
    printf("initialized serial\n");

    rc = probe_init();
    if (rc != RC_OK) {
        printf("error during probe initialization\n");
        handle_error();
    }
    printf("initialized probe\n");
    rc = probe_start((u16 *)ADC_SAMPLES, SZ);
    if (rc != RC_OK) {
        printf("error during probe initialization\n");
        handle_error();
    }
    printf("started probe\n");

    DisplayFile *display = NULL;
    ChannelHandle ch1_hdl, ch2_hdl;
    rc = display_open(TERMINAL_DISPLAY, &display);
    if (rc != RC_OK) {
        printf("error opening display\n");
        handle_error();
    }
    rc = display_add_channel(display, "Channel 1", &ch1_hdl);
    if (rc != RC_OK) {
        printf("error adding channel 1\n");
        handle_error();
    }
    rc = display_add_channel(display, "Channel 2", &ch2_hdl);
    if (rc != RC_OK) {
        printf("error adding channel 2\n");
        handle_error();
    }
    rc = display_set_x(display, 80);
    if (rc != RC_OK) {
        printf("error setting x dimension on display\n");
        handle_error();
    }
    rc = display_set_y(display, 25);
    if (rc != RC_OK) {
        printf("error setting y dimension on display\n");
        handle_error();
    }
    rc = display_set_scale(display, VOLTAGE_MAX);
    if (rc != RC_OK) {
        printf("error setting display scale\n");
        handle_error();
    }
    rc = display_redraw(display);
    if (rc != RC_OK) {
        printf("error drawing display\n");
        handle_error();
    }

    while (1) {
        toggle_led();
        if (STATE.dma_complete) {
            u64 sum = 0;
            for (usize i = 0; i < SZ / 2; ++i) {
                sum += STATE.buf[i];
            }
            u16 avg = sum / SZ;
            double avg_voltage = adc_to_voltage(avg);
            printf("avg voltage: %lfV\n", avg_voltage);
        } else {
            printf("not yet\n");
        }
        HAL_Delay(500);
        display_redraw(display);
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

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
    if (hadc->Instance == ADC1) {
        if (probe_fetch(&STATE.buf, &STATE.sz) != RC_OK) {
            handle_error();
        } else {
            STATE.dma_complete = 1;
        }
    } else {
        handle_error();
    }
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc) {
    if (hadc->Instance == ADC1) {
        if (probe_fetch(&STATE.buf, &STATE.sz) != RC_OK) {
            handle_error();
        } else {
            STATE.dma_complete = 1;
        }
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
