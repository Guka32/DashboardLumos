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
#include "telemetry.h"
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
CAN_HandleTypeDef hcan1;
CAN_HandleTypeDef hcan2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
UART_HandleTypeDef huart2;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN1_Init(void);
static void MX_CAN2_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t can1_active = 0;
uint8_t can2_active = 0;

void CAN_Start_Peripherals(void)
{
  CAN_FilterTypeDef filter;
  
  // Configure filter for CAN1 (Master)
  filter.FilterBank = 0;
  filter.FilterMode = CAN_FILTERMODE_IDMASK;
  filter.FilterScale = CAN_FILTERSCALE_32BIT;
  filter.FilterFIFOAssignment = CAN_RX_FIFO0;
  filter.FilterActivation = CAN_FILTER_ENABLE;
  filter.FilterIdHigh = 0;
  filter.FilterIdLow = 0;
  filter.FilterMaskIdHigh = 0;
  filter.FilterMaskIdLow = 0;
  filter.SlaveStartFilterBank = 14;
  
  if (HAL_CAN_ConfigFilter(&hcan1, &filter) != HAL_OK)
  {
    Error_Handler();
  }
  
  // Configure filter for CAN2 (Slave)
  filter.FilterBank = 14;
  filter.FilterFIFOAssignment = CAN_RX_FIFO0;
  if (HAL_CAN_ConfigFilter(&hcan2, &filter) != HAL_OK)
  {
    Error_Handler();
  }
  
  // Start CAN1 (if successful, flag active; if not, ignore gracefully)
  if (HAL_CAN_Start(&hcan1) == HAL_OK)
  {
    can1_active = 1;
    
    // Enable CAN1 RX Interrupt Notification
    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
    
    // Configure and enable CAN1 RX0 interrupt vector in NVIC
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
  }
  
  // Start CAN2 (if successful, flag active; if not, ignore gracefully)
  if (HAL_CAN_Start(&hcan2) == HAL_OK)
  {
    can2_active = 1;
  }
}

void CAN_Send_Heartbeat(void)
{
  CAN_TxHeaderTypeDef txHeader;
  uint32_t txMailbox;
  uint8_t payload[8] = {0};
  uint32_t tick = HAL_GetTick();
  
  // Pack uptime tick (uint32_t) into first 4 bytes of payload
  payload[0] = (uint8_t)(tick & 0xFF);
  payload[1] = (uint8_t)((tick >> 8) & 0xFF);
  payload[2] = (uint8_t)((tick >> 16) & 0xFF);
  payload[3] = (uint8_t)((tick >> 24) & 0xFF);
  
  // Configure Tx Header
  txHeader.StdId = 0x010;        // Telemetry Gateway Heartbeat Standard ID
  txHeader.RTR = CAN_RTR_DATA;
  txHeader.IDE = CAN_ID_STD;
  txHeader.DLC = 4;              // Sending 4 bytes (uptime tick)
  txHeader.TransmitGlobalTime = DISABLE;
  
  // Transmit on CAN1 (only if active and mailbox is free)
  if (can1_active && (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) > 0))
  {
    HAL_CAN_AddTxMessage(&hcan1, &txHeader, payload, &txMailbox);
  }
  
  // Transmit on CAN2 (only if active and mailbox is free)
  if (can2_active && (HAL_CAN_GetTxMailboxesFreeLevel(&hcan2) > 0))
  {
    HAL_CAN_AddTxMessage(&hcan2, &txHeader, payload, &txMailbox);
  }
}


void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  CAN_RxHeaderTypeDef rxHeader;
  uint8_t rxData[8];
  
  if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, rxData) == HAL_OK)
  {
    // Parse ID 0x200 (Battery Control PCB)
    if (rxHeader.StdId == 0x200)
    {
      if (rxHeader.DLC >= 8)
      {
        float volt, curr;
        uint8_t *v_ptr = (uint8_t*)&volt;
        uint8_t *c_ptr = (uint8_t*)&curr;
        for(int i = 0; i < 4; i++) {
          v_ptr[i] = rxData[i];
          c_ptr[i] = rxData[i+4];
        }
        status.battery_voltage = volt;
        status.battery_current = curr;
      }
      status.battery_pcb_last_seen = HAL_GetTick();
    }
    // Parse ID 0x150 (Actuator Control PCB)
    else if (rxHeader.StdId == 0x150)
    {
      if (rxHeader.DLC >= 2)
      {
        status.pump_active = rxData[0];
        status.actuator_active = rxData[1];
      }
      status.actuator_pcb_last_seen = HAL_GetTick();
    }
    // Parse ID 0x100 (Main PCB Heartbeat)
    else if (rxHeader.StdId == 0x100)
    {
      status.main_pcb_last_seen = HAL_GetTick();
    }
  }
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
  MX_CAN1_Init();
  MX_CAN2_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  CAN_Start_Peripherals();
  uint32_t last_hb_tick = 0;
  uint32_t last_telemetry_tick = 0;
  
  // Initialize mock GPS positions
  status.gps1_lat = 25.67142;
  status.gps1_lon = -100.30971;
  status.gps1_fix = 4; // RTK Fixed
  status.gps2_lat = 25.67145;
  status.gps2_lon = -100.30973;
  status.gps2_fix = 2; // 3D Fix
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // 1. Send CAN Heartbeat (1 Hz)
    if (HAL_GetTick() - last_hb_tick >= 1000)
    {
      HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5); // Toggle LED on transmission
      CAN_Send_Heartbeat();
      last_hb_tick = HAL_GetTick();
    }
    
    // 2. Broadcast NMEA Telemetry over USART2 (5 Hz / 200 ms)
    if (HAL_GetTick() - last_telemetry_tick >= 200)
    {
      last_telemetry_tick = HAL_GetTick();
      status.gateway_uptime = last_telemetry_tick;
      
      // Simulate minor GPS drift to demonstrate motion on Grafana
      status.gps1_lat += 0.00001;
      status.gps1_lon += 0.000005;
      status.gps2_lat += 0.00001;
      status.gps2_lon += 0.000005;
      
      // Read local ADC backup battery (mock 3.28V)
      status.local_battery_voltage = 3.28f;
      
      // Format and send $PSTAT sentence
      char tx_buf[128];
      int len = generate_nmea_sentence(tx_buf, sizeof(tx_buf), "PSTAT", "%lu,%.2f,%.2f,%u,%u,%.2f",
                                       status.gateway_uptime,
                                       status.battery_voltage,
                                       status.battery_current,
                                       status.pump_active,
                                       status.actuator_active,
                                       status.local_battery_voltage);
      if (len > 0)
      {
        HAL_UART_Transmit(&huart2, (uint8_t*)tx_buf, len, 100);
      }
      
      // Format and send $PGPS sentence
      len = generate_nmea_sentence(tx_buf, sizeof(tx_buf), "PGPS", "%.6f,%.6f,%u,%.6f,%.6f,%u",
                                   status.gps1_lat, status.gps1_lon, status.gps1_fix,
                                   status.gps2_lat, status.gps2_lon, status.gps2_fix);
      if (len > 0)
      {
        HAL_UART_Transmit(&huart2, (uint8_t*)tx_buf, len, 100);
      }
    }
    
    HAL_Delay(10); // Yield CPU, preventing debugger halt timeouts
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
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

/**
  * @brief CAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 8;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_13TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */

  /* USER CODE END CAN1_Init 2 */

}

/**
  * @brief CAN2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN2_Init(void)
{

  /* USER CODE BEGIN CAN2_Init 0 */

  /* USER CODE END CAN2_Init 0 */

  /* USER CODE BEGIN CAN2_Init 1 */

  /* USER CODE END CAN2_Init 1 */
  hcan2.Instance = CAN2;
  hcan2.Init.Prescaler = 8;
  hcan2.Init.Mode = CAN_MODE_NORMAL;
  hcan2.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan2.Init.TimeSeg1 = CAN_BS1_13TQ;
  hcan2.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan2.Init.TimeTriggeredMode = DISABLE;
  hcan2.Init.AutoBusOff = DISABLE;
  hcan2.Init.AutoWakeUp = DISABLE;
  hcan2.Init.AutoRetransmission = DISABLE;
  hcan2.Init.ReceiveFifoLocked = DISABLE;
  hcan2.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN2_Init 2 */

  /* USER CODE END CAN2_Init 2 */

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
  if (HAL_UART_Init(&huart2) != HAL_OK)
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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

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
