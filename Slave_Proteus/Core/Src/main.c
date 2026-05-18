/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : TPMS Slave - Tire Pressure Sensor Node (Proteus)
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TX_HEADER  0x5A
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  HAL_Init();
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  MX_GPIO_Init();
  MX_SPI1_Init();
  /* Reconfigure PA6 as GPIO output for bit-bang MISO */
  GPIO_InitTypeDef gpio = {0};
  gpio.Pin = GPIO_PIN_6;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &gpio);
  MX_USART1_UART_Init();
  huart1.Init.BaudRate = 9600;
  HAL_UART_Init(&huart1);
  MX_ADC1_Init();
  /* PA2 as analog for temperature potentiometer */
  { GPIO_InitTypeDef g={0}; g.Pin=GPIO_PIN_2; g.Mode=GPIO_MODE_ANALOG; HAL_GPIO_Init(GPIOA,&g); }
  uint8_t temperature = 25;
  /* USER CODE BEGIN 2 */
  HAL_GPIO_WritePin(LED_SYS_GPIO_Port, LED_SYS_Pin, GPIO_PIN_SET);
  HAL_Delay(500);
  HAL_GPIO_WritePin(LED_SYS_GPIO_Port, LED_SYS_Pin, GPIO_PIN_RESET);

  char msg[] = "Slave ready\r\n";
  HAL_UART_Transmit(&huart1, (uint8_t*)msg, sizeof(msg)-1, 100);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
    char dbg[64];
    int len;
    static uint16_t pressure = 0, loop_count = 0;

    /* Read pressure ADC (CH1) */
    ADC1->SQR3 = ADC_CHANNEL_1;
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    uint16_t adc_val = HAL_ADC_GetValue(&hadc1);
    pressure = (uint32_t)adc_val * 600 / 4095;

    /* Read temperature ADC (CH2) every 4th loop */
    if (loop_count % 4 == 0) {
      ADC1->SQR3 = ADC_CHANNEL_2;
      HAL_ADC_Start(&hadc1);
      HAL_ADC_PollForConversion(&hadc1, 10);
      uint16_t t_raw = HAL_ADC_GetValue(&hadc1);
      temperature = (uint8_t)((uint32_t)t_raw * 60 / 4095);
    }

    /* Build 6-byte tx: sync + header + P_hi + P_lo + temp + checksum */
    uint8_t tx[6];
    tx[0] = 0x00;
    tx[1] = TX_HEADER;
    tx[2] = pressure >> 8;
    tx[3] = pressure & 0xFF;
    tx[4] = temperature;
    tx[5] = tx[1] + tx[2] + tx[3] + tx[4];

    /* Wait for NSS low */
    while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_SET);

    /* Software SPI slave */
    int b, bit;
    for (b = 0; b < 6; b++) {
      uint8_t byte = tx[b];
      for (bit = 0; bit < 8; bit++) {
        while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) == GPIO_PIN_RESET);
        if (byte & 0x80) HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
        else             HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
        byte <<= 1;
        while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) == GPIO_PIN_SET);
      }
    }
    HAL_GPIO_TogglePin(LED_TX_GPIO_Port, LED_TX_Pin);

    /* Wait for NSS high */
    while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_RESET);

    /* Debug */
    len = sprintf(dbg, "P=%3ukPa T=%uC\r\n", pressure, temperature);
    HAL_UART_Transmit(&huart1, (uint8_t*)dbg, len, 100);
    loop_count++;
    HAL_UART_Transmit(&huart1, (uint8_t*)dbg, len, 100);
  }
  /* USER CODE END 3 */
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSEState = RCC_HSE_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) Error_Handler();
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
