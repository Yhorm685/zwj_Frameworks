/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Application/rtos_messages.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticQueue_t osStaticMessageQDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for comm */
osThreadId_t commHandle;
const osThreadAttr_t comm_attributes = {
  .name = "comm",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for sensor */
osThreadId_t sensorHandle;
const osThreadAttr_t sensor_attributes = {
  .name = "sensor",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for monitor */
osThreadId_t monitorHandle;
const osThreadAttr_t monitor_attributes = {
  .name = "monitor",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for requestQueue */
osMessageQueueId_t requestQueueHandle;
uint8_t requestBuffer[ 4 * sizeof( uint16_t ) ];
osStaticMessageQDef_t requestControl;
const osMessageQueueAttr_t requestQueue_attributes = {
  .name = "requestQueue",
  .cb_mem = &requestControl,
  .cb_size = sizeof(requestControl),
  .mq_mem = &requestBuffer,
  .mq_size = sizeof(requestBuffer)
};
/* Definitions for eventQueue */
osMessageQueueId_t eventQueueHandle;
uint8_t eventBuffer[ 8 * sizeof( FrameworkSensorEvent ) ];
osStaticMessageQDef_t eventControl;
const osMessageQueueAttr_t eventQueue_attributes = {
  .name = "eventQueue",
  .cb_mem = &eventControl,
  .cb_size = sizeof(eventControl),
  .mq_mem = &eventBuffer,
  .mq_size = sizeof(eventBuffer)
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void Framework_Run(void);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
extern void StartSensor(void *argument);
extern void StartMonitor(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  if (osKernelGetState() != osKernelReady)
  {
    Error_Handler();
  }
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of requestQueue */
  requestQueueHandle = osMessageQueueNew (4, sizeof(uint16_t), &requestQueue_attributes);

  /* creation of eventQueue */
  eventQueueHandle = osMessageQueueNew (8, sizeof(FrameworkSensorEvent), &eventQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of comm */
  commHandle = osThreadNew(StartDefaultTask, NULL, &comm_attributes);

  /* creation of sensor */
  sensorHandle = osThreadNew(StartSensor, NULL, &sensor_attributes);

  /* creation of monitor */
  monitorHandle = osThreadNew(StartMonitor, NULL, &monitor_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  if (commHandle == NULL || sensorHandle == NULL || monitorHandle == NULL ||
      requestQueueHandle == NULL || eventQueueHandle == NULL)
  {
    Error_Handler();
  }
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  (void)argument;
  Framework_Run();
  Error_Handler();
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

