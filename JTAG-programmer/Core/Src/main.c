/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
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
#include "stdio.h" // for printf
#include "JTAGfunc430.h"
#include "LowLevelFunc430.h"
#include "Devices430.h"

//mine
#include "jtag_programmer.h"
#include "stdlib.h" // for malloc
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
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#define GETCHAR_PROTOTYPE int __io_getchar(void)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#define GETCHAR_PROTOTYPE int fgetc(FILE *f)
#endif

PUTCHAR_PROTOTYPE
{
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}

GETCHAR_PROTOTYPE
{
  uint8_t ch = 0;

  /* Clear the Overrun flag just before receiving the first character */
  __HAL_UART_CLEAR_OREFLAG(&huart2);

  /* Wait for reception of a character on the USART RX line and echo this
   * character on console */
  HAL_UART_Receive(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
static void GPIO_DeInit(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// TODO -- TESTING
word StartJtag(void);

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
  /* USER CODE BEGIN 2 */

  // -------------- TESTING --------------
//  ClrRST();
//  ClrTCK();
//  ClrTDI();
//  ClrTMS();
//  ClrTST();
//  volatile word x = StartJtag();
//  ClrRST();
//  ClrTCK();
//  ClrTDI();
//  ClrTMS();
//  ClrTST();
//  printf("got jtag id is 0x%04x\n", x);
//  printf("Exit\n");
//
//  while(1)
//  {}

  // -------------- END TESTING -------------


	printf("starting...\r\n");
	HAL_Delay(1000);

	uint8_t ret = 0;
	node_t *program = NULL;

	printf("Ready\n");

	uint8_t *c = NULL;
	uint16_t bytes_received = 0;

	uint16_t file_len = 0;

	if (HAL_UART_Receive(&huart2, (uint8_t*) &file_len, 2, 4000) != HAL_OK)	// receive 2 bytes, LSB first
	{
		printf("Critical Error! Failed to receive file length\n");
		goto error;
	}

	if (file_len < 12) // the smallest possible .hex file is 12 bytes. ":00000001FF" + EOF
	{
		printf("Critical Error! File is too short (<12 bytes)\n");
		goto error;
	}

	printf("-vSuccessfully received file length %u bytes\n", file_len);

	c = (uint8_t*) malloc(file_len);

	{
		uint8_t *cptr = c;
		while ((ret = HAL_UART_Receive(&huart2, cptr, 1, 4000)) == HAL_OK) // 4 second timeout
		{
			++cptr;

			if (cptr >= c + file_len) {
				break;
			}
			continue;
		}

		bytes_received = (uint16_t) (cptr - c);
		if (ret != HAL_OK)
		{
			printf("Critical Error! Timed out while waiting for file. Received %u/%u ret = %u\n", cptr - c, file_len, ret);
			goto error;
		}
	}

	printf("-vReceived file of length %u bytes\n", bytes_received);

	if (bytes_received < 12) // the smallest possible .hex file is 12 bytes. ":00000001FF" + EOF
	{
		printf("Critical Error! File is too short (<12 bytes)\n");
		goto error;
	}

	printf("Verifying .hex file...\n");
	ret = verify_hexfile_and_return_program_linklist(c, bytes_received, &program);
	if (ret != 0)
	{
		printf("Aborting...\n");
		goto error;
	}
	printf("-vSuccessfully verified .hex file\n");

	printf("Connecting to target...\n");
	uint16_t deviceID;
	word status = GetDevice(&deviceID);
	if (status != STATUS_OK)         // Set DeviceId
	{
		printf("Get device failed. status=0x%x\n\r", status);
		goto error;
	}                                    // time-out. (error: red LED is ON)
	printf("-vSuccessfully connected to target 0x%04x\n", deviceID);

	printf("Erasing target flash...\n");

	uint8_t main_segments[64] = { 0 };// 64 segments of main memory 512 bytes each 0x8000 to 0xffff (in reality only 4 segments are used in 2k flash but this is easier
	uint8_t info_segments[4] = { 0 }; // 4 segments of info memory 64 bytes each 0x1000 to 0x10ff. [0] is segment A and [3] is segment D

	// iterate through program linked list to find out which segments need to be erased
	for (node_t *iter = program; iter != NULL; iter = iter->next)// traverse the linked list
	{
		if (iter->address >= 0x8000) // technically 32kb flash region starts at 0x8000
		{
			//main
			uint8_t seg_index = (0xffff - iter->address) / 0x0200;
			uint8_t seg_index_of_last_byte = (0xffff - iter->address - 2*iter->length) / 0x0200;	// the segment that the last byte lies in. (*2 because length is number of words!)
			if (main_segments[seg_index] == 0)
			{
				printf("-vPlan to flash main segment %u (0x%04x)\n", seg_index, iter->address);
				main_segments[seg_index] = 1;
			}
			if (main_segments[seg_index_of_last_byte] == 0)
			{
				printf("-vCode overlaps into next segment!\n");
				printf("-vPlan to flash main segment %u (0x%04x)\n", seg_index_of_last_byte, iter->address);
				main_segments[seg_index_of_last_byte] = 1;
			}
		}
		else if (iter->address >= 0x1000 && iter->address <= 0x10ff)
		{
			//info
			uint8_t seg_index = (0x10ff - iter->address) / 0x40;
			if (info_segments[seg_index] == 0)
			{
				info_segments[seg_index] = 1;
				printf("-vNeed to flash info segment %c (0x%04x)\n", seg_index + 'A', iter->address);
			}
		}
//		else
//		{
//			should never reach here because the adress range is checked in verifyhexFile.
//			TODO verify the range someplace else.
//		}
	}

	uint32_t now = HAL_GetTick();
	for (int i = 0; i < 64; i++)
	{
		if (main_segments[i])
		{
			uint16_t address = 0xFE00 - 0x200 * i;

			EraseFLASH(ERASE_SGMT, address);
			if (EraseCheck(address, 0x0100) != STATUS_OK) // Check main memory erasure (Fxx2..9)
			{
				printf("Critical Error! Failed to erase main flash segment %u (begins 0x%04x)\n", i, address);
				goto error;
			}
			printf("-vSuccessfully erased flash segment %u (begins 0x%04x)\n", i, address);
		}
	}
	for (int i = 0; i < 4; i++)
	{
		if (info_segments[i])
		{
			uint16_t address = 0x10C0 - 0x40 * i;

			EraseFLASH(ERASE_SGMT, address);
			if (EraseCheck(address, 0x0020) != STATUS_OK) // Check main memory erasure (Fxx2..9)
			{
				if (i == 0) // segment A
				{
					printf("Error! Failed to erase info flash segment A (begins 0x10C0). This is a special segment which contains configuration data\n");
					continue;
				}

				printf("Critical Error! Failed to erase info flash segment %c (begins 0x%04x)\n", i + 'A', address);
				goto error;
			}
			printf("-vSuccessfully erased flash segment %c (begins 0x%04x)\n", i = 'A', address);
		}
	}
	uint32_t delta = HAL_GetTick() - now;
	printf("-vSuccessfully erased target flash\n");

	printf("Writing target flash...\n");
	now = HAL_GetTick();

	for (node_t *iter = program; iter != NULL; iter = iter->next)// traverse the linked list
	{
		WriteFLASH(iter->address, iter->length, iter->data);

		uint16_t *temp = (uint16_t*) malloc(iter->length * sizeof(uint16_t));
		ReadMemQuick(iter->address, iter->length, temp);

		printf("-v0x%04x: ", iter->address);
		for (int i = 0; i < iter->length; i++)
		{
			printf("%04x", temp[i]);
		}
		printf("\n");

		free(temp);

		if (VerifyMem(iter->address, iter->length, iter->data) != STATUS_OK)
		{
			printf("Critical Error! Verification of memory block 0x%04x failed!\n", iter->address);
			goto error;
		}
	}

	delta = HAL_GetTick() - now;
	printf("Successfully wrote target flash. Time elapsed %01lu:%02lu.%03lu\n", delta / 60000, (delta / 1000) % 60, delta % 1000);

error:
	printf("Shutting down JTAG connection...\n");
	ReleaseDevice(V_RESET);

	// cleanup
	free(c);
	free_linkedlist(program);

	GPIO_DeInit();

	printf("Exit\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {

		HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
		HAL_Delay(500);
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

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
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
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
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
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
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOH);

  /**/
  LL_GPIO_ResetOutputPin(GPIOA, TARGET_RESET_Pin|TMS_Pin|TCK_Pin|TDI_Pin
                          |TEST_Pin);

  /**/
  LL_GPIO_ResetOutputPin(LD3_GPIO_Port, LD3_Pin);

  /**/
  GPIO_InitStruct.Pin = TARGET_RESET_Pin|TMS_Pin|TCK_Pin|TDI_Pin
                          |TEST_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = TDO_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(TDO_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LL_GPIO_PIN_0|LL_GPIO_PIN_1|LL_GPIO_PIN_4|LL_GPIO_PIN_5
                          |LL_GPIO_PIN_6|LL_GPIO_PIN_7;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LL_GPIO_PIN_8|LL_GPIO_PIN_9|LL_GPIO_PIN_10|LL_GPIO_PIN_11
                          |LL_GPIO_PIN_12;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LD3_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LD3_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LL_GPIO_PIN_3;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOH, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

// deinits all JTAG pins
static void GPIO_DeInit(void)
{
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  GPIO_InitStruct.Pin = TMS_Pin|TCK_Pin|TDI_Pin|TEST_Pin|TDO_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = TARGET_RESET_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}
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
	while (1) {
	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
