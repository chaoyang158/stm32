/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
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
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
volatile uint16_t command_index = 0;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
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
void uart3_send(const char *data)
{
  HAL_UART_Transmit(&huart3, (uint8_t *)data, strlen(data), HAL_MAX_DELAY);
}

void handle_uart3_response()
{
  static uint16_t last_head = 0;
  static char temp_buffer[256];
  static int temp_pos = 0;

  static int expected_length = -1; // -1表示没开始接收
  static char tcp_payload[128];
  static int tcp_payload_pos = 0;

  while (last_head != uart3_rx_head)
  {
    char ch = uart3_rx_buffer[last_head++];
    if (last_head >= UART3_RX_BUFFER_SIZE)
      last_head = 0;

    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 10); // 原样转发

    // 收集到临时缓存
    if (temp_pos < sizeof(temp_buffer) - 1)
    {
      temp_buffer[temp_pos++] = ch;
      temp_buffer[temp_pos] = '\0';
    }
    else
    {
      temp_pos = 0;
    }

    // 如果还没识别出+IPD
    if (expected_length == -1)
    {
      char *ipd_ptr = strstr(temp_buffer, "+IPD,");
      if (ipd_ptr)
      {
        // 查找冒号
        char *colon = strchr(ipd_ptr, ':');
        if (colon)
        {
          // 解析长度字段
          int len = 0;
          sscanf(ipd_ptr + 5, "%d", &len);

          expected_length = len;
          tcp_payload_pos = 0;

          // 如果冒号后已经带了部分数据
          // int already_have = temp_pos - (colon - temp_buffer + 1);
          // if (already_have > 0)
          // {
          //   for (int i = 0; i < already_have && tcp_payload_pos < sizeof(tcp_payload) - 1; i++)
          //   {
          //     tcp_payload[tcp_payload_pos++] = colon[1 + i];
          //   }
          // }
        }
      }
    }
    else
    {
      // 正在接收数据正文
      if (tcp_payload_pos < expected_length)
      {
        tcp_payload[tcp_payload_pos++] = ch;
      }

      // 收满了
      if (tcp_payload_pos >= expected_length)
      {
        tcp_payload[tcp_payload_pos] = '\0';

        // 显示到OLED
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t *)tcp_payload, strlen(tcp_payload));

        // 清理
        expected_length = -1;
        temp_pos = 0;
        memset(temp_buffer, 0, sizeof(temp_buffer));
      }
    }
  }
}

int send_tcp_data(const char *data)
{
  char cmd[128];

  sprintf(cmd, "AT+CIPSEND=%d", strlen(data));

  // 先发送长度指令
  send_AT_command(cmd);

  // 等待 '>' 提示符
  if (!wait_for_response(">", 1000))
    return 0;

  HAL_Delay(100);
  // 发送实际数据
  HAL_UART_Transmit(&huart3, (uint8_t *)data, strlen(data), HAL_MAX_DELAY);

  // 等待发送完成
  if (!wait_for_response("SEND OK", 2000))
    return 0;

  return 1;
}

void send_AT_command(const char *command)
{
  HAL_UART_Transmit(&huart3, (uint8_t *)command, strlen(command), 1000);
  HAL_UART_Transmit(&huart3, (uint8_t *)"\r\n", 2, 1000);
}

int wait_for_response(const char *expected_response, uint32_t timeout)
{
  uint32_t tickstart = HAL_GetTick();
  uint16_t check_pos = 0; // 上次缓冲区检查到的位置

  while ((HAL_GetTick() - tickstart) < timeout)
  {
    // 判断缓冲区是否有新数
    if (check_pos != uart3_rx_head)
    {

      char temp_buffer[UART3_RX_BUFFER_SIZE];
      uint16_t temp_idx = 0;
      uint16_t i = check_pos;

      while (i != uart3_rx_head && temp_idx < sizeof(temp_buffer) - 1)
      {
        temp_buffer[temp_idx++] = uart3_rx_buffer[i++];
        if (i >= UART3_RX_BUFFER_SIZE)
          i = 0; // 环形缓冲处理
      }
      temp_buffer[temp_idx] = '\0';

      if (strstr(temp_buffer, expected_response) != NULL)
      {
        return 1; // 找到预期响应
      }

      check_pos = uart3_rx_head; // 更新位置继续等待
    }
  }

  return 0;
}

void handle_uart1_input()
{
  static char input_line[UART1_RX_BUFFER_SIZE];
  static int input_pos = 0;
  static int oled_current_line = 0;
  while (command_index != uart1_rx_head)
  {
    char ch = uart1_rx_buffer[command_index++];
    if (command_index >= UART1_RX_BUFFER_SIZE)
      command_index = 0;

    if (ch == '\r' || ch == '\n')
    {
      if (input_pos > 0)
      {
        input_line[input_pos] = '\0';

        // 直接发送输入的内容，不用判断前缀
        send_tcp_data(input_line);
        input_pos = 0;
      }
    }
    else if (input_pos < sizeof(input_line) - 1)
    {
      input_line[input_pos++] = ch;
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
  __HAL_RCC_AFIO_CLK_ENABLE();
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_I2C1_Init();
  uint8_t A[] = "TCP connected!!";
  // 初始化oled屏幕
  OLED_Init();
  // 开启OLED显示
  OLED_Display_On();
  // 清屏
  OLED_Clear();

  /* USER CODE BEGIN 2 */

  send_AT_command("ATE0");
  send_AT_command("AT+CWJAP=\"WHLD\",\"WHLDADMIN\"");
  HAL_Delay(2000);

  send_AT_command("AT+CWAUTOCONN=1");
  HAL_Delay(2000);

  int connected = 0;
  while (!connected)
  {
    send_AT_command("AT+CIPSTART=\"TCP\",\"192.168.50.187\",81");
    connected = wait_for_response("OK", 5000);
    if (connected)
    {
      HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET);
      OLED_ShowString(0, 0, A, sizeof(A));
      HAL_Delay(5000);
      OLED_Clear();
    }
    else
    {
      for (int i = 0; i < 5; i++)
      {
        HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
        HAL_Delay(500);
      }
      HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET);
      HAL_Delay(1000);
    }
  }

  send_AT_command("AT+CIPDINFO=1");
  HAL_Delay(500);

  const char *data = "Hello\r\n"; // 7字节数据
  char send_cmd[20];
  sprintf(send_cmd, "AT+CIPSEND=%d", strlen(data));
  send_AT_command(send_cmd);
  if (wait_for_response(">", 1000))
  {
    HAL_UART_Transmit(&huart3, (uint8_t *)data, strlen(data), 1000);
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    handle_uart1_input();    // 处理上位机命令
    handle_uart3_response(); // 处理ESP8266响应
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
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
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
