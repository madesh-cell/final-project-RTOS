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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <stdbool.h>
#include "mw_gpio.h"
#include "mw_time.h"
#include "mw_uart.h"
#include "mw_adc.h"
#include "mw_pwm.h"
#include "../../middleware/os/mw_os.h"
#include "lib_fem.h"
#include "drv_hx710b.h"
#include "drv_pump.h"
#include "drv_valve.h"
#include "drv_pot.h"
#include "app_pad.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart2;

/* Definitions for SafetyTask */
osThreadId_t SafetyTaskHandle;
const osThreadAttr_t SafetyTask_attributes = {
  .name = "SafetyTask",
  .priority = (osPriority_t) osPriorityHigh,
  .stack_size = 256 * 4
};
/* Definitions for SensorTask */
osThreadId_t SensorTaskHandle;
const osThreadAttr_t SensorTask_attributes = {
  .name = "SensorTask",
  .priority = (osPriority_t) osPriorityAboveNormal,
  .stack_size = 256 * 4
};
/* Definitions for ButtonTask */
osThreadId_t ButtonTaskHandle;
const osThreadAttr_t ButtonTask_attributes = {
  .name = "ButtonTask",
  .priority = (osPriority_t) osPriorityBelowNormal,
  .stack_size = 128 * 4
};
/* Definitions for ControlTask */
osThreadId_t ControlTaskHandle;
const osThreadAttr_t ControlTask_attributes = {
  .name = "ControlTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 384 * 4
};
/* Definitions for DisplayTask */
osThreadId_t DisplayTaskHandle;
const osThreadAttr_t DisplayTask_attributes = {
  .name = "DisplayTask",
  .priority = (osPriority_t) osPriorityLow,
  .stack_size = 384 * 4
};
/* USER CODE BEGIN PV */
static DRV_HX710B_Handle_t MAIN_sensor1_handle;
static DRV_HX710B_Handle_t MAIN_sensor2_handle;
static DRV_PUMP_Handle_t   MAIN_pump_handle;
static DRV_VALVE_Handle_t  MAIN_valve1_handle;
static DRV_VALVE_Handle_t  MAIN_valve2_handle;
static DRV_POT_Handle_t    MAIN_pot_handle;
static const char         *MAIN_wiring_step = "start";
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM3_Init(void);
void StartSafetyTask(void *argument);
void StartSensorTask(void *argument);
void StartButtonTask(void *argument);
void StartControlTask(void *argument);
void StartDisplayTask(void *argument);

/* USER CODE BEGIN PFP */
static bool APP_Main_WireModules(void);
static void APP_Main_ReportWiringFailure(void);
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

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  if (!APP_Main_WireModules())
  {
    APP_Main_ReportWiringFailure();
    Error_Handler();
  }
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  (void)MW_OS_Init();
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of SafetyTask */
  SafetyTaskHandle = osThreadNew(StartSafetyTask, NULL, &SafetyTask_attributes);

  /* creation of SensorTask */
  SensorTaskHandle = osThreadNew(StartSensorTask, NULL, &SensorTask_attributes);

  /* creation of ButtonTask */
  ButtonTaskHandle = osThreadNew(StartButtonTask, NULL, &ButtonTask_attributes);

  /* creation of ControlTask */
  ControlTaskHandle = osThreadNew(StartControlTask, NULL, &ControlTask_attributes);

  /* creation of DisplayTask */
  DisplayTaskHandle = osThreadNew(StartDisplayTask, NULL, &DisplayTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.LowPowerAutoPowerOff = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_160CYCLES_5;
  hadc1.Init.SamplingTimeCommon2 = ADC_SAMPLETIME_1CYCLE_5;
  hadc1.Init.OversamplingMode = DISABLE;
  hadc1.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_GREEN_Pin */
  GPIO_InitStruct.Pin = LED_GREEN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LED_GREEN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PC7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PB4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/**
 * @brief  Wire middleware, drivers and application together. The only place this happens.
 * @return true if every module initialised, false if a wiring step failed.
 * @note   A sensor or valve failure inside APP_Pad_Init is reported through LIB_FEM and
 *         leaves the application in FAULT. It is not a wiring error, so it does not make
 *         this function return false.
 */
static bool APP_Main_WireModules(void)
{
  bool wiring_ok = false;

  const LIB_FEM_Config_t fem_cfg =
  {
    .get_ms   = MW_TIME_If.get_ms,
    .on_event = APP_Pad_OnFemEvent
  };

  const DRV_HX710B_Config_t sensor1_cfg =
  {
    .gpio_set    = MW_GPIO_If.set,
    .gpio_read   = MW_GPIO_If.read,
    .delay_us    = MW_TIME_If.delay_us,
    .dout_pin_id = MW_GPIO_PIN_S1_DOUT,
    .sclk_pin_id = MW_GPIO_PIN_S1_SCLK
  };

  const DRV_HX710B_Config_t sensor2_cfg =
  {
    .gpio_set    = MW_GPIO_If.set,
    .gpio_read   = MW_GPIO_If.read,
    .delay_us    = MW_TIME_If.delay_us,
    .dout_pin_id = MW_GPIO_PIN_S2_DOUT,
    .sclk_pin_id = MW_GPIO_PIN_S2_SCLK
  };

  const DRV_PUMP_Config_t pump_cfg =
  {
    .gpio_set       = MW_GPIO_If.set,
    .enable_pin_id  = MW_GPIO_PIN_PUMP,
    .pwm_set        = MW_PWM_If.set_duty,
    .pwm_channel_id = MW_PWM_CH_PUMP
  };

  const DRV_VALVE_Config_t valve1_cfg =
  {
    .gpio_set = MW_GPIO_If.set,
    .pin_id   = MW_GPIO_PIN_VALVE1
  };

  const DRV_VALVE_Config_t valve2_cfg =
  {
    .gpio_set = MW_GPIO_If.set,
    .pin_id   = MW_GPIO_PIN_VALVE2
  };

  const DRV_POT_Config_t pot_cfg =
  {
    .adc_read      = MW_ADC_If.read_voltage,
    .channel_id    = MW_ADC_CH_POT,
    .full_scale_mv = MW_ADC_VREF_MV
  };

  const APP_Pad_Config_t app_cfg =
  {
    .sensor1         = &MAIN_sensor1_handle,
    .sensor2         = &MAIN_sensor2_handle,
    .pump            = &MAIN_pump_handle,
    .valve1          = &MAIN_valve1_handle,
    .valve2          = &MAIN_valve2_handle,
    .pot             = &MAIN_pot_handle,
    .gpio_set        = MW_GPIO_If.set,
    .gpio_read       = MW_GPIO_If.read,
    .get_ms          = MW_TIME_If.get_ms,
    .uart_write      = MW_UART_If.write,
    .os_lock         = MW_OS_If.lock,
    .os_unlock       = MW_OS_If.unlock,
    .os_sched_lock   = MW_OS_If.sched_lock,
    .os_sched_unlock = MW_OS_If.sched_unlock,
    .button_pin_id   = MW_GPIO_PIN_BUTTON,
    .led_pin_id      = MW_GPIO_PIN_LED_GREEN
  };

  MW_GPIO_Init();

  MAIN_wiring_step = "uart";
  wiring_ok = MW_UART_Init(&huart2);

  if (wiring_ok)
  {
    MAIN_wiring_step = "adc";
    wiring_ok = MW_ADC_Init();
  }

  if (wiring_ok)
  {
    MAIN_wiring_step = "pwm";
    wiring_ok = MW_PWM_Init();
  }

  if (wiring_ok)
  {
    MAIN_wiring_step = "fem";
    wiring_ok = LIB_FEM_Init(&fem_cfg);
  }

  if (wiring_ok)
  {
    MAIN_wiring_step = "sensor1";
    wiring_ok = DRV_HX710B_Init(&MAIN_sensor1_handle, &sensor1_cfg);
  }

  if (wiring_ok)
  {
    MAIN_wiring_step = "sensor2";
    wiring_ok = DRV_HX710B_Init(&MAIN_sensor2_handle, &sensor2_cfg);
  }

  if (wiring_ok)
  {
    MAIN_wiring_step = "pump";
    wiring_ok = DRV_PUMP_Init(&MAIN_pump_handle, &pump_cfg);
  }

  if (wiring_ok)
  {
    MAIN_wiring_step = "valve1";
    wiring_ok = DRV_VALVE_Init(&MAIN_valve1_handle, &valve1_cfg);
  }

  if (wiring_ok)
  {
    MAIN_wiring_step = "valve2";
    wiring_ok = DRV_VALVE_Init(&MAIN_valve2_handle, &valve2_cfg);
  }

  if (wiring_ok)
  {
    MAIN_wiring_step = "pot";
    wiring_ok = DRV_POT_Init(&MAIN_pot_handle, &pot_cfg);
  }

  if (wiring_ok)
  {
    /* The cause of a failure is reported through LIB_FEM inside APP_Pad_Init. */
    (void)APP_Pad_Init(&app_cfg);
  }

  return wiring_ok;
}

/**
 * @brief  Print which wiring step failed, so a start-up failure is never silent.
 * @note   Uses the UART directly. If the UART itself failed, nothing can be printed and
 *         the green LED stays off.
 */
static void APP_Main_ReportWiringFailure(void)
{
  static const char prefix[] = "WIRING FAILED at step: ";
  uint16_t          length   = 0U;

  while (MAIN_wiring_step[length] != '\0')
  {
    length++;
  }

  (void)HAL_UART_Transmit(&huart2, (uint8_t *)prefix, (uint16_t)(sizeof(prefix) - 1U), 100U);
  (void)HAL_UART_Transmit(&huart2, (uint8_t *)MAIN_wiring_step, length, 100U);
  (void)HAL_UART_Transmit(&huart2, (uint8_t *)"\r\n", 2U, 100U);
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartSafetyTask */
/**
  * @brief  Function implementing the SafetyTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartSafetyTask */
void StartSafetyTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    APP_Pad_RunSafety();
    osDelay(APP_PAD_SAFETY_TASK_PERIOD_MS);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartSensorTask */
/**
* @brief Function implementing the SensorTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSensorTask */
void StartSensorTask(void *argument)
{
  /* USER CODE BEGIN StartSensorTask */
  /* Infinite loop */
  for(;;)
  {
    APP_Pad_RunSensors();
    osDelay(APP_PAD_SENSOR_TASK_PERIOD_MS);
  }
  /* USER CODE END StartSensorTask */
}

/* USER CODE BEGIN Header_StartButtonTask */
/**
* @brief Function implementing the ButtonTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartButtonTask */
void StartButtonTask(void *argument)
{
  /* USER CODE BEGIN StartButtonTask */
  /* Infinite loop */
  for(;;)
  {
    APP_Pad_RunButton();
    osDelay(APP_PAD_BUTTON_TASK_PERIOD_MS);
  }
  /* USER CODE END StartButtonTask */
}

/* USER CODE BEGIN Header_StartControlTask */
/**
* @brief Function implementing the ControlTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartControlTask */
void StartControlTask(void *argument)
{
  /* USER CODE BEGIN StartControlTask */
  /* Infinite loop */
  for(;;)
  {
    APP_Pad_RunControl();
    osDelay(APP_PAD_CONTROL_TASK_PERIOD_MS);
  }
  /* USER CODE END StartControlTask */
}

/* USER CODE BEGIN Header_StartDisplayTask */
/**
* @brief Function implementing the DisplayTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDisplayTask */
void StartDisplayTask(void *argument)
{
  /* USER CODE BEGIN StartDisplayTask */
  /* Infinite loop */
  for(;;)
  {
    APP_Pad_RunDisplay();
    osDelay(APP_PAD_DISPLAY_TASK_PERIOD_MS);
  }
  /* USER CODE END StartDisplayTask */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
