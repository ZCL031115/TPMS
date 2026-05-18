/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : TPMS Master - Dashboard Receiver & OLED Display (Proteus)
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "ssd1306.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SYNC_HEADER     0x5A
#define PRESS_MIN       180
#define PRESS_MAX       350
#define PRESS_CRITICAL  150
#define TEMP_LOW        0
#define TEMP_HIGH       45
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

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  MX_GPIO_Init();
  MX_USART1_UART_Init();
  huart1.Init.BaudRate = 9600;
  HAL_UART_Init(&huart1);
  MX_SPI1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  HAL_GPIO_WritePin(SPI_SS_GPIO_Port, SPI_SS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LED_OK_GPIO_Port, LED_OK_Pin, GPIO_PIN_SET);
  HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
  { GPIO_InitTypeDef g = {0}; g.Pin=GPIO_PIN_1; g.Mode=GPIO_MODE_ANALOG; HAL_GPIO_Init(GPIOA,&g); }

  char dbg[64];
  int len;
  len = sprintf(dbg, "OLED init...\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)dbg, len, 100);
  SSD1306_Init();
  len = sprintf(dbg, "OLED done\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)dbg, len, 100);
  HAL_Delay(500);

  len = sprintf(dbg, "Master ready\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)dbg, len, 100);

  uint8_t  display_mode = 0;
  uint8_t  alarm_test   = 0;
  uint16_t pressure     = 0;
  uint8_t  temperature  = 25;
  uint8_t  alarm_state  = 0;
  uint8_t  last_alarm   = 0;
  uint8_t  loop_count   = 0;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* --- Software SPI master --- */
    /* Reconfigure SPI pins as GPIO */
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_5 | GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
    gpio.Pin = GPIO_PIN_6;
    gpio.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(GPIOA, &gpio);

    HAL_GPIO_WritePin(SPI_SS_GPIO_Port, SPI_SS_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_Delay(1);

    int b, bit;
    uint8_t rx[6] = {0};
    for (b = 0; b < 6; b++) {
      for (bit = 0; bit < 8; bit++) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        HAL_Delay(1);
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6))
          rx[b] |= (0x80 >> bit);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
        HAL_Delay(1);
      }
    }
    HAL_GPIO_WritePin(SPI_SS_GPIO_Port, SPI_SS_Pin, GPIO_PIN_SET);

    uint8_t cksum = (uint8_t)(rx[1]+rx[2]+rx[3]+rx[4]);
    if (rx[1] == SYNC_HEADER && rx[5] == cksum) {
      pressure    = ((uint16_t)rx[2] << 8) | rx[3];
      temperature = rx[4];
    }
    else {
      len = sprintf(dbg, "SPI raw: %02X %02X %02X %02X %02X %02X\r\n",
                    rx[0],rx[1],rx[2],rx[3],rx[4],rx[5]);
      HAL_UART_Transmit(&huart1, (uint8_t*)dbg, len, 100);
    }

    /* --- Buttons --- */
    if (HAL_GPIO_ReadPin(KEY_MENU_GPIO_Port, KEY_MENU_Pin) == GPIO_PIN_RESET)
    {
      HAL_Delay(30);
      if (HAL_GPIO_ReadPin(KEY_MENU_GPIO_Port, KEY_MENU_Pin) == GPIO_PIN_RESET)
      {
        display_mode = !display_mode;
        while (HAL_GPIO_ReadPin(KEY_MENU_GPIO_Port, KEY_MENU_Pin) == GPIO_PIN_RESET);
      }
    }

    if (HAL_GPIO_ReadPin(KEY_ALARM_GPIO_Port, KEY_ALARM_Pin) == GPIO_PIN_RESET)
    {
      HAL_Delay(30);
      if (HAL_GPIO_ReadPin(KEY_ALARM_GPIO_Port, KEY_ALARM_Pin) == GPIO_PIN_RESET)
      {
        alarm_test = !alarm_test;
        while (HAL_GPIO_ReadPin(KEY_ALARM_GPIO_Port, KEY_ALARM_Pin) == GPIO_PIN_RESET);
      }
    }

    /* --- Alarm logic --- */
    alarm_state = (pressure > 0 && (pressure < PRESS_MIN || pressure > PRESS_MAX))
               || (temperature < TEMP_LOW || temperature > TEMP_HIGH)
               || alarm_test;

    if (alarm_state)
    {
      HAL_GPIO_WritePin(LED_ALARM_GPIO_Port, LED_ALARM_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LED_OK_GPIO_Port,    LED_OK_Pin,    GPIO_PIN_RESET);
      if (!last_alarm) {
        /* Restore PA1 to AF_PP for PWM */
        GPIO_InitTypeDef g = {0};
        g.Pin = GPIO_PIN_1; g.Mode = GPIO_MODE_AF_PP;
        HAL_GPIO_Init(GPIOA, &g);
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
      }
    }
    else
    {
      HAL_GPIO_WritePin(LED_ALARM_GPIO_Port, LED_ALARM_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(LED_OK_GPIO_Port,    LED_OK_Pin,    GPIO_PIN_SET);
      if (last_alarm) {
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
        /* Switch PA1 to analog to kill crosstalk */
        GPIO_InitTypeDef g = {0};
        g.Pin = GPIO_PIN_1; g.Mode = GPIO_MODE_ANALOG;
        HAL_GPIO_Init(GPIOA, &g);
      }
    }
    last_alarm = alarm_state;

    /* --- OLED Display (every 2s) --- */
    if (loop_count % 4 == 0) {
    len = sprintf(dbg, "OLED upd %d\r\n", display_mode);
    HAL_UART_Transmit(&huart1, (uint8_t*)dbg, len, 100);
    if (display_mode == 0) {
      SSD1306_ShowStr(1, 1, "TPMS Monitor");
      char buf[16];
      sprintf(buf, "P:%3ukPa T:%2uC", pressure, temperature);
      SSD1306_ShowStr(2, 1, buf);
      if (alarm_state) {
        if (pressure < PRESS_CRITICAL)     SSD1306_ShowStr(3, 1, "PRES DANGER!");
        else if (pressure < PRESS_MIN || pressure > PRESS_MAX)
                                           SSD1306_ShowStr(3, 1, "PRES ALARM");
        else if (temperature < TEMP_LOW || temperature > TEMP_HIGH)
                                           SSD1306_ShowStr(3, 1, "TEMP ALARM");
      } else SSD1306_ShowStr(3, 1, "Status: OK");
      SSD1306_ShowStr(4, 1, "M:Mode A:Test");
    } else {
      SSD1306_ShowStr(1, 1, "Tire #1 Detail");
      char buf[16];
      sprintf(buf, "Press: %ukPa", pressure);
      SSD1306_ShowStr(2, 1, buf);
      sprintf(buf, "Temp: %u C", temperature);
      SSD1306_ShowStr(3, 1, buf);
      if (alarm_state) SSD1306_ShowStr(4, 1, "ALARM ACTIVE");
      else SSD1306_ShowStr(4, 1, "Normal");
    }
    } /* end if (loop_count % 4 == 0) */

    /* --- Debug UART --- */
    len = sprintf(dbg, "P=%ukPa T=%uC M=%d A=%d\r\n", pressure, temperature, display_mode, alarm_state);
    HAL_UART_Transmit(&huart1, (uint8_t*)dbg, len, 100);

    HAL_GPIO_TogglePin(LED_SYS_GPIO_Port, LED_SYS_Pin);
    loop_count++;
    HAL_Delay(500);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSEState = RCC_HSE_OFF;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
