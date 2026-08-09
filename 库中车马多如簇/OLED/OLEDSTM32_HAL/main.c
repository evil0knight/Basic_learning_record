/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ssd1306.h"
#include "ssd1306_fonts.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DEMO_PAGE_TIME_MS  1500U
#define SIGNAL_BAR_COUNT   5U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan;

I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
static void OLED_ShowSignalBars(uint8_t level);
static void OLED_ShowIconDemo(void);
static void OLED_ShowWaveDemo(uint8_t phase);
static void OLED_ShowMenuDemo(uint8_t selected);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void OLED_ShowSignalBars(uint8_t level)
{
  uint8_t i;
  uint8_t x = 12;
  uint8_t base_y = 58;
  uint8_t bar_w = 12;
  uint8_t gap = 6;
  uint8_t heights[SIGNAL_BAR_COUNT] = {8, 14, 20, 26, 32};

  ssd1306_Fill(Black);
  ssd1306_SetCursor(0, 0);
  ssd1306_WriteString("Signal", Font_7x10, White);

  for (i = 0; i < SIGNAL_BAR_COUNT; i++) {
    uint8_t x1 = x + i * (bar_w + gap);
    uint8_t y1 = base_y - heights[i];
    uint8_t x2 = x1 + bar_w;
    uint8_t y2 = base_y;

    ssd1306_DrawRectangle(x1, y1, x2, y2, White);
    if (i < level) {
      ssd1306_FillRectangle((uint8_t)(x1 + 2U), (uint8_t)(y1 + 2U), (uint8_t)(x2 - 2U), (uint8_t)(y2 - 2U), White);
    }
  }

  ssd1306_SetCursor(92, 0);
  ssd1306_WriteString("4/5", Font_7x10, White);
  ssd1306_UpdateScreen();
}

static void OLED_ShowIconDemo(void)
{
  ssd1306_Fill(Black);
  ssd1306_SetCursor(0, 0);
  ssd1306_WriteString("Icons", Font_7x10, White);

  ssd1306_DrawCircle(20, 24, 10, White);
  ssd1306_Line(20, 10, 20, 14, White);
  ssd1306_Line(20, 34, 20, 38, White);
  ssd1306_Line(6, 24, 10, 24, White);
  ssd1306_Line(30, 24, 34, 24, White);
  ssd1306_Line(11, 15, 14, 18, White);
  ssd1306_Line(26, 30, 29, 33, White);
  ssd1306_Line(11, 33, 14, 30, White);
  ssd1306_Line(26, 18, 29, 15, White);

  ssd1306_DrawRectangle(45, 14, 75, 34, White);
  ssd1306_FillRectangle(77, 20, 80, 28, White);
  ssd1306_FillRectangle(48, 17, 60, 31, White);

  ssd1306_DrawCircle(102, 24, 10, White);
  ssd1306_DrawCircle(102, 24, 4, White);
  ssd1306_Line(102, 14, 102, 10, White);
  ssd1306_Line(102, 34, 102, 38, White);
  ssd1306_Line(92, 24, 88, 24, White);
  ssd1306_Line(112, 24, 116, 24, White);
  ssd1306_Line(95, 17, 91, 13, White);
  ssd1306_Line(109, 31, 113, 35, White);
  ssd1306_Line(95, 31, 91, 35, White);
  ssd1306_Line(109, 17, 113, 13, White);

  ssd1306_SetCursor(6, 44);
  ssd1306_WriteString("Sun", Font_6x8, White);
  ssd1306_SetCursor(48, 44);
  ssd1306_WriteString("Batt", Font_6x8, White);
  ssd1306_SetCursor(93, 44);
  ssd1306_WriteString("Gear", Font_6x8, White);
  ssd1306_UpdateScreen();
}

static void OLED_ShowWaveDemo(uint8_t phase)
{
  uint8_t x;
  uint8_t prev_x = 5;
  uint8_t prev_y = 34;

  ssd1306_Fill(Black);
  ssd1306_SetCursor(0, 0);
  ssd1306_WriteString("Trend", Font_7x10, White);
  ssd1306_SetCursor(88, 0);
  ssd1306_WriteString("72%", Font_7x10, White);

  ssd1306_Line(4, 54, 124, 54, White);
  ssd1306_Line(4, 14, 4, 54, White);

  for (x = 0; x < 116; x++) {
    uint8_t local = (uint8_t)((x + phase) % 24U);
    uint8_t y;

    if (local < 12U) {
      y = (uint8_t)(48U - local);
    } else {
      y = (uint8_t)(24U + (local - 12U));
    }

    if (x > 0U) {
      ssd1306_Line(prev_x, prev_y, (uint8_t)(5U + x), y, White);
    }

    prev_x = (uint8_t)(5U + x);
    prev_y = y;
  }

  ssd1306_UpdateScreen();
}

static void OLED_ShowMenuDemo(uint8_t selected)
{
  uint8_t item_y[3] = {16, 30, 44};
  char *items[3] = {"CAN View", "Settings", "About"};
  uint8_t i;

  ssd1306_Fill(Black);
  ssd1306_SetCursor(0, 0);
  ssd1306_WriteString("Menu", Font_7x10, White);

  for (i = 0; i < 3; i++) {
    if (i == selected) {
      ssd1306_DrawRectangle(0, (uint8_t)(item_y[i] - 2U), 127, (uint8_t)(item_y[i] + 10U), White);
      ssd1306_FillRectangle(3, (uint8_t)(item_y[i] + 2U), 7, (uint8_t)(item_y[i] + 6U), White);
    }
    ssd1306_SetCursor(14, (uint8_t)(item_y[i] - 1U));
    ssd1306_WriteString(items[i], Font_7x10, White);
  }

  ssd1306_UpdateScreen();
}
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

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CAN_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  ssd1306_Init();
  ssd1306_Fill(Black);
  ssd1306_UpdateScreen();
  HAL_Delay(50);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint8_t level;
    uint8_t selected;

    for (level = 1; level <= SIGNAL_BAR_COUNT; level++) {
      OLED_ShowSignalBars(level);
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
      HAL_Delay(400);
    }

    OLED_ShowIconDemo();
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(DEMO_PAGE_TIME_MS);

    OLED_ShowWaveDemo(0);
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(450);
    OLED_ShowWaveDemo(8);
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(450);
    OLED_ShowWaveDemo(16);
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(450);

    for (selected = 0; selected < 3; selected++) {
      OLED_ShowMenuDemo(selected);
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
      HAL_Delay(550);
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
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
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CAN Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN_Init(void)
{

  /* USER CODE BEGIN CAN_Init 0 */

  /* USER CODE END CAN_Init 0 */

  /* USER CODE BEGIN CAN_Init 1 */

  /* USER CODE END CAN_Init 1 */
  hcan.Instance = CAN1;
  hcan.Init.Prescaler = 4;
  hcan.Init.Mode = CAN_MODE_NORMAL;
  hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan.Init.TimeSeg1 = CAN_BS1_9TQ;
  hcan.Init.TimeSeg2 = CAN_BS2_8TQ;
  hcan.Init.TimeTriggeredMode = DISABLE;
  hcan.Init.AutoBusOff = ENABLE;
  hcan.Init.AutoWakeUp = DISABLE;
  hcan.Init.AutoRetransmission = ENABLE;
  hcan.Init.ReceiveFifoLocked = DISABLE;
  hcan.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN_Init 2 */

  /* USER CODE END CAN_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
