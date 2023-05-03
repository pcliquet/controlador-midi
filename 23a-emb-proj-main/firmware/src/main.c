/************************************************************************
* 5 semestre - Eng. da Computao - Insper
*
* 2021 - Exemplo com HC05 com RTOS
*
*/

#include <asf.h>
#include "conf_board.h"
#include <string.h>

/************************************************************************/
/* defines                                                              */
/************************************************************************/

// LEDs
#define LED_PIO      PIOC
#define LED_PIO_ID   ID_PIOC
#define LED_IDX      8
#define LED_IDX_MASK (1 << LED_IDX)

//AFEC-MOS
#define AFEC_POT AFEC0
#define AFEC_POT_ID ID_AFEC0
#define AFEC_POT_CHANNEL 5 // Canal do pino PB2

// Botões
#define BUT_PIO_0     PIOA
#define BUT_PIO_ID_0   ID_PIOA
#define BUT_IDX_0      19
#define BUT_IDX_MASK_0 (1 << BUT_IDX_0)

#define BUT_PIO_1      PIOD
#define BUT_PIO_ID_1   ID_PIOD
#define BUT_IDX_1      30
#define BUT_IDX_MASK_1 (1 << BUT_IDX_1)

#define BUT_PIO_2      PIOD
#define BUT_PIO_ID_2   ID_PIOD
#define BUT_IDX_2      11
#define BUT_IDX_MASK_2 (1 << BUT_IDX_2)

#define BUT_PIO_3      PIOA
#define BUT_PIO_ID_3   ID_PIOA
#define BUT_IDX_3      6
#define BUT_IDX_MASK_3 (1 << BUT_IDX_3)

#define BUT_PIO_4      PIOD
#define BUT_PIO_ID_4   ID_PIOD
#define BUT_IDX_4      26
#define BUT_IDX_MASK_4 (1 << BUT_IDX_4)

#define BUT_PIO_5     PIOC
#define BUT_PIO_ID_5   ID_PIOC
#define BUT_IDX_5      19
#define BUT_IDX_MASK_5 (1 << BUT_IDX_5)

#define BUT_PIO_6      PIOA
#define BUT_PIO_ID_6   ID_PIOA
#define BUT_IDX_6      24
#define BUT_IDX_MASK_6 (1 << BUT_IDX_6)

#define BUT_PIO_7      PIOA
#define BUT_PIO_ID_7   ID_PIOA
#define BUT_IDX_7      2
#define BUT_IDX_MASK_7 (1 << BUT_IDX_7)

// usart (bluetooth ou serial)
// Descomente para enviar dados
// pela serial debug

#define DEBUG_SERIAL

#ifdef DEBUG_SERIAL
#define USART_COM USART1
#define USART_COM_ID ID_USART1
#else
#define USART_COM USART0
#define USART_COM_ID ID_USART0
#endif

/************************************************************************/
/* RTOS                                                                 */
/************************************************************************/

#define TASK_BLUETOOTH_STACK_SIZE            (4096/sizeof(portSTACK_TYPE))
#define TASK_BLUETOOTH_STACK_PRIORITY        (tskIDLE_PRIORITY)

#define TASK_ADC_STACK_SIZE (1024 * 10 / sizeof(portSTACK_TYPE))
#define TASK_ADC_STACK_PRIORITY (tskIDLE_PRIORITY)

#define TASK_PROC_STACK_SIZE (1024 * 10 / sizeof(portSTACK_TYPE))
#define TASK_PROC_STACK_PRIORITY (tskIDLE_PRIORITY)

/************************************************************************/
/* prototypes                                                           */
/************************************************************************/

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,
signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);
static void USART1_init(void);
static void config_AFEC_pot(Afec *afec, uint32_t afec_id, uint32_t afec_channel,
afec_callback_t callback);
static void configure_console(void);

/************************************************************************/
/* constants                                                            */
/************************************************************************/

/************************************************************************/
/* variaveis globais                                                    */
/************************************************************************/

volatile char butflag = 0;
volatile char butflag1 = 0;
volatile char butflag2 = 0;
volatile char butflag3 = 0;
volatile char butflag4 = 0;
volatile char butflag5 = 0;
volatile char butflag6 = 0;
volatile char butflag7 = 0;

/************************************************************************/
/* recursos RTOS                                                        */
/************************************************************************/

TimerHandle_t xTimer;

/** Queue for msg log send data */
QueueHandle_t xQueueADC;

typedef struct {
	uint value;
} adcData;

/************************************************************************/
/* RTOS application HOOK                                                */
/************************************************************************/

/* Called if stack overflow during execution */
extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,
signed char *pcTaskName) {
/*	printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);*/
	/* If the parameters have been corrupted then inspect pxCurrentTCB to
	* identify which task has overflowed its stack.
	*/
	for (;;) {
	}
}

/* This function is called by FreeRTOS idle task */
extern void vApplicationIdleHook(void) {
	pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
}

/* This function is called by FreeRTOS each tick */
extern void vApplicationTickHook(void) { }

extern void vApplicationMallocFailedHook(void) {
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */

	/* Force an assert. */
	configASSERT( ( volatile void * ) NULL );
}

/************************************************************************/
/* handlers / callbacks                                                 */
/************************************************************************/

void btn1_callback(void) {
	butflag = 1;
}

void btn2_callback(void) {
	butflag1 = 1;
}

void btn3_callback(void) {
	butflag2 = 1;
}

void btn4_callback(void) {
	butflag3 = 1;
}

void btn5_callback(void) {
	butflag4 = 1;
}

void btn6_callback(void) {
	butflag5 = 1;
}

void btn7_callback(void) {
	butflag6 = 1;
}

void btn8_callback(void) {
	butflag7 = 1;
}


static void AFEC_pot_callback(void) {
	adcData adc;
	adc.value = afec_channel_get_value(AFEC_POT, AFEC_POT_CHANNEL);
	BaseType_t xHigherPriorityTaskWoken = pdTRUE;
	xQueueSendFromISR(xQueueADC, &adc, &xHigherPriorityTaskWoken);
}


/************************************************************************/
/* funcoes                                                              */
/************************************************************************/


static void config_AFEC_pot(Afec *afec, uint32_t afec_id, uint32_t afec_channel,
                            afec_callback_t callback) {
  /*************************************
   * Ativa e configura AFEC
   *************************************/
  /* Ativa AFEC - 0 */
  afec_enable(afec);

  /* struct de configuracao do AFEC */
  struct afec_config afec_cfg;

  /* Carrega parametros padrao */
  afec_get_config_defaults(&afec_cfg);

  /* Configura AFEC */
  afec_init(afec, &afec_cfg);

  /* Configura trigger por software */
  afec_set_trigger(afec, AFEC_TRIG_SW);

  /*** Configuracao específica do canal AFEC ***/
  struct afec_ch_config afec_ch_cfg;
  afec_ch_get_config_defaults(&afec_ch_cfg);
  afec_ch_cfg.gain = AFEC_GAINVALUE_0;
  afec_ch_set_config(afec, afec_channel, &afec_ch_cfg);

  /*
  * Calibracao:
  * Because the internal ADC offset is 0x200, it should cancel it and shift
  down to 0.
  */
  afec_channel_set_analog_offset(afec, afec_channel, 0x200);

  /***  Configura sensor de temperatura ***/
  struct afec_temp_sensor_config afec_temp_sensor_cfg;

  afec_temp_sensor_get_config_defaults(&afec_temp_sensor_cfg);
  afec_temp_sensor_set_config(afec, &afec_temp_sensor_cfg);

  /* configura IRQ */
  afec_set_callback(afec, afec_channel, callback, 1);
  NVIC_SetPriority(afec_id, 4);
  NVIC_EnableIRQ(afec_id);
}

void io_init(void) {

	// Ativa PIOs
	pmc_enable_periph_clk(LED_PIO_ID);
	pmc_enable_periph_clk(BUT_PIO_ID_0);
	pmc_enable_periph_clk(BUT_PIO_ID_1);
	pmc_enable_periph_clk(BUT_PIO_ID_2);
	pmc_enable_periph_clk(BUT_PIO_ID_3);
	pmc_enable_periph_clk(BUT_PIO_ID_4);
	pmc_enable_periph_clk(BUT_PIO_ID_5);
	pmc_enable_periph_clk(BUT_PIO_ID_6);
	pmc_enable_periph_clk(BUT_PIO_ID_7);

	// Configura Pinos
	pio_configure(LED_PIO, PIO_OUTPUT_0, LED_IDX_MASK, PIO_DEFAULT | PIO_DEBOUNCE);
	
	// Botões
	pio_configure(BUT_PIO_0, PIO_INPUT, BUT_IDX_MASK_0, PIO_PULLUP);
	pio_configure(BUT_PIO_1, PIO_INPUT, BUT_IDX_MASK_1, PIO_PULLUP);
	pio_configure(BUT_PIO_2, PIO_INPUT, BUT_IDX_MASK_2, PIO_PULLUP);
	pio_configure(BUT_PIO_3, PIO_INPUT, BUT_IDX_MASK_3, PIO_PULLUP);
	pio_configure(BUT_PIO_4, PIO_INPUT, BUT_IDX_MASK_4, PIO_PULLUP);
	pio_configure(BUT_PIO_5, PIO_INPUT, BUT_IDX_MASK_5, PIO_PULLUP);
	pio_configure(BUT_PIO_6, PIO_INPUT, BUT_IDX_MASK_6, PIO_PULLUP);
	pio_configure(BUT_PIO_7, PIO_INPUT, BUT_IDX_MASK_7, PIO_PULLUP);
	
	pio_handler_set(BUT_PIO_0,
	BUT_PIO_ID_0,
	BUT_IDX_MASK_0,
	PIO_IT_FALL_EDGE,
	btn1_callback);

	pio_enable_interrupt(BUT_PIO_0, BUT_IDX_MASK_0);
	pio_get_interrupt_status(BUT_PIO_0);
	NVIC_EnableIRQ(BUT_PIO_ID_0);
	NVIC_SetPriority(BUT_PIO_ID_0, 4);
	
	pio_handler_set(BUT_PIO_1,
	BUT_PIO_ID_1,
	BUT_IDX_MASK_1,
	PIO_IT_FALL_EDGE,
	btn2_callback);

	pio_enable_interrupt(BUT_PIO_1, BUT_IDX_MASK_1);
	pio_get_interrupt_status(BUT_PIO_1);
	NVIC_EnableIRQ(BUT_PIO_ID_1);
	NVIC_SetPriority(BUT_PIO_ID_1, 4);
	
	pio_handler_set(BUT_PIO_2,
	BUT_PIO_ID_2,
	BUT_IDX_MASK_2,
	PIO_IT_FALL_EDGE,
	btn3_callback);

	pio_enable_interrupt(BUT_PIO_2, BUT_IDX_MASK_2);
	pio_get_interrupt_status(BUT_PIO_2);
	NVIC_EnableIRQ(BUT_PIO_ID_2);
	NVIC_SetPriority(BUT_PIO_ID_2, 4);
	
	pio_handler_set(BUT_PIO_3,
	BUT_PIO_ID_3,
	BUT_IDX_MASK_3,
	PIO_IT_FALL_EDGE,
	btn4_callback);

	pio_enable_interrupt(BUT_PIO_3, BUT_IDX_MASK_3);
	pio_get_interrupt_status(BUT_PIO_3);
	NVIC_EnableIRQ(BUT_PIO_ID_3);
	NVIC_SetPriority(BUT_PIO_ID_3, 4);
	
	pio_handler_set(BUT_PIO_1,
	BUT_PIO_ID_1,
	BUT_IDX_MASK_1,
	PIO_IT_FALL_EDGE,
	btn5_callback);

	pio_enable_interrupt(BUT_PIO_4, BUT_IDX_MASK_4);
	pio_get_interrupt_status(BUT_PIO_4);
	NVIC_EnableIRQ(BUT_PIO_ID_4);
	NVIC_SetPriority(BUT_PIO_ID_4, 4);
	
	pio_handler_set(BUT_PIO_5,
	BUT_PIO_ID_5,
	BUT_IDX_MASK_5,
	PIO_IT_FALL_EDGE,
	btn6_callback);

	pio_enable_interrupt(BUT_PIO_5, BUT_IDX_MASK_5);
	pio_get_interrupt_status(BUT_PIO_5);
	NVIC_EnableIRQ(BUT_PIO_ID_5);
	NVIC_SetPriority(BUT_PIO_ID_5, 4);
	
	pio_handler_set(BUT_PIO_6,
	BUT_PIO_ID_6,
	BUT_IDX_MASK_6,
	PIO_IT_FALL_EDGE,
	btn7_callback);

	pio_enable_interrupt(BUT_PIO_6, BUT_IDX_MASK_6);
	pio_get_interrupt_status(BUT_PIO_6);
	NVIC_EnableIRQ(BUT_PIO_ID_6);
	NVIC_SetPriority(BUT_PIO_ID_6, 4);
	
	pio_handler_set(BUT_PIO_7,
	BUT_PIO_ID_7,
	BUT_IDX_MASK_7,
	PIO_IT_FALL_EDGE,
	btn8_callback);

	pio_enable_interrupt(BUT_PIO_7, BUT_IDX_MASK_7);
	pio_get_interrupt_status(BUT_PIO_7);
	NVIC_EnableIRQ(BUT_PIO_ID_7);
	NVIC_SetPriority(BUT_PIO_ID_7, 4);
}

static void configure_console(void) {
	const usart_serial_options_t uart_serial_options = {
		.baudrate = CONF_UART_BAUDRATE,
		#if (defined CONF_UART_CHAR_LENGTH)
		.charlength = CONF_UART_CHAR_LENGTH,
		#endif
		.paritytype = CONF_UART_PARITY,
		#if (defined CONF_UART_STOP_BITS)
		.stopbits = CONF_UART_STOP_BITS,
		#endif
	};

	/* Configure console UART. */
	stdio_serial_init(CONF_UART, &uart_serial_options);

	/* Specify that stdout should not be buffered. */
	#if defined(__GNUC__)
	setbuf(stdout, NULL);
	#else
	/* Already the case in IAR's Normal DLIB default configuration: printf()
	* emits one character at a time.
	*/
	#endif
}

uint32_t usart_puts(uint8_t *pstring) {
	uint32_t i ;

	while(*(pstring + i))
	if(uart_is_tx_empty(USART_COM))
	usart_serial_putchar(USART_COM, *(pstring+i++));
}


void usart_put_string(Usart *usart, char str[]) {
	usart_serial_write_packet(usart, str, strlen(str));
}

int usart_get_string(Usart *usart, char buffer[], int bufferlen, uint timeout_ms) {
	uint timecounter = timeout_ms;
	uint32_t rx;
	uint32_t counter = 0;

	while( (timecounter > 0) && (counter < bufferlen - 1)) {
		if(usart_read(usart, &rx) == 0) {
			buffer[counter++] = rx;
		}
		else{
			timecounter--;
			vTaskDelay(1);
		}
	}
	buffer[counter] = 0x00;
	return counter;
}

void usart_send_command(Usart *usart, char buffer_rx[], int bufferlen,
char buffer_tx[], int timeout) {
	usart_put_string(usart, buffer_tx);
	usart_get_string(usart, buffer_rx, bufferlen, timeout);
}

void config_usart0(void) {
	sysclk_enable_peripheral_clock(ID_USART0);
	usart_serial_options_t config;
	config.baudrate = CONF_UART_BAUDRATE;
	config.charlength = US_MR_CHRL_8_BIT;
	config.paritytype = US_MR_PAR_NO;
	config.stopbits = false;
	usart_serial_init(USART0, &config);
	usart_enable_tx(USART0);
	usart_enable_rx(USART0);

	// RX - PB0  TX - PB1
	pio_configure(PIOB, PIO_PERIPH_C, (1 << 0), PIO_DEFAULT);
	pio_configure(PIOB, PIO_PERIPH_C, (1 << 1), PIO_DEFAULT);
}

int hc05_init(void) {
	char buffer_rx[128];
	usart_send_command(USART_COM, buffer_rx, 1000, "AT", 100);
	vTaskDelay( 20 / portTICK_PERIOD_MS);
	usart_send_command(USART_COM, buffer_rx, 1000, "AT", 100);
	vTaskDelay( 20 / portTICK_PERIOD_MS);
	usart_send_command(USART_COM, buffer_rx, 1000, "AT+NAMEagoravai", 100);
	vTaskDelay( 20 / portTICK_PERIOD_MS);
	usart_send_command(USART_COM, buffer_rx, 1000, "AT", 100);
	vTaskDelay( 20 / portTICK_PERIOD_MS);
	usart_send_command(USART_COM, buffer_rx, 1000, "AT+PIN0000", 100);
}

/************************************************************************/
/* TASKS                                                                */
/************************************************************************/

void vTimerCallback(TimerHandle_t xTimer) {
	/* Selecina canal e inicializa conversão */
	afec_channel_enable(AFEC_POT, AFEC_POT_CHANNEL);
	afec_start_software_conversion(AFEC_POT);
}

static void task_proc(void *pvParameters) {

  // configura ADC e TC para controlar a leitura
  config_AFEC_pot(AFEC_POT, AFEC_POT_ID, AFEC_POT_CHANNEL, AFEC_pot_callback);

  xTimer = xTimerCreate(/* Just a text name, not used by the RTOS
                        kernel. */
                        "Timer",
                        /* The timer period in ticks, must be
                        greater than 0. */
                        100,
                        /* The timers will auto-reload themselves
                        when they expire. */
                        pdTRUE,
                        /* The ID is used to store a count of the
                        number of times the timer has expired, which
                        is initialised to 0. */
                        (void *)0,
                        /* Timer callback */
                        vTimerCallback);
  xTimerStart(xTimer, 0);

  // variável para recever dados da fila
  adcData adc;

  while (1) {
  }
}

void task_bluetooth(void) {
// 	printf("Task Bluetooth started \n");
// 	
// 	printf("Inicializando HC05 \n");
	config_usart0();
	hc05_init();
	
	// configura LEDs e Botões
	io_init();
	
	int valoradc;
	adcData adc;
	//BOTOES
	char button0 = '0';
	char button1 = '0';
	char button2 = '0';
	char button3 = '0';
	char button4 = '0';
	char button5 = '0';
	char button6 = '0';
	char button7 = '0';
	
	//AFEC - POTENCIOMETRO
	char button8 = '0';
	char button9 = '0';
	char button10 = '0';
	
	char eof = 'X';
	int i = 0; 
	// Task não deve retornar.
	while(1) {
		// atualiza valor do botão
		if (xQueueReceive(xQueueADC, &(adc), 1000)) {
			valoradc = adc.value;
		}
		if (butflag) {
			button0 = '1';
		} 
		if (butflag1){
			button1 = '1';
		}
		if (butflag2){
			button2 = '1';
		}
		if (butflag3){
			button3 = '1';
		}
		if (butflag4){
			button4 = '1';
		}
		if (butflag5){
			button5 = '1';
		}
		if (butflag6){
			button6 = '1';
		}
		if (butflag7){
			button7 = '1';
		}
		if (valoradc <= 504){
			button8 = '0';
			button9 = '0';
			button10 = '0';
		}
		if (valoradc > 504 && valoradc <= 1007){
			button8 = '0';
			button9 = '0';
			button10 = '1';
		}
		if (valoradc > 1007 && valoradc <= 1511){
			button8 = '0';
			button9 = '1';
			button10 = '0';
		}
		if (valoradc > 1511 && valoradc <= 2016){
			button8 = '0';
			button9 = '1';
			button10 = '1';
		}
		if (valoradc > 2016 && valoradc <= 2521){
			button8 = '1';
			button9 = '0';
			button10 = '0';
		}
		if (valoradc > 2521 && valoradc <= 3026){
			button8 = '1';
			button9 = '0';
			button10 = '1';
		}
		if (valoradc > 3026 && valoradc <= 3532){
			button8 = '1';
			button9 = '1';
			button10 = '0';
		}
		if (valoradc > 3532){
			button8 = '1';
			button9 = '1';
			button10 = '1';
		}

		// envia status botão
		
		while(!usart_is_tx_ready(USART_COM)) {
			vTaskDelay(1 / portTICK_PERIOD_MS);
		}
		usart_write(USART_COM, button0);
		if(pio_get(BUT_PIO_0, PIO_INPUT, BUT_IDX_MASK_0) == 0){
			butflag = 1;
		}
		else{
			butflag = 0;
			button0 = '0';
		}
		
		while(!usart_is_tx_ready(USART_COM)) {
			vTaskDelay(1 / portTICK_PERIOD_MS);
		}
		usart_write(USART_COM, button1);
		if(pio_get(BUT_PIO_1, PIO_INPUT, BUT_IDX_MASK_1) == 0){
			butflag1 = 1;
		}
		else{
			butflag1 = 0;
			button1 = '0';
		}
		
		while(!usart_is_tx_ready(USART_COM)) {
			vTaskDelay(1 / portTICK_PERIOD_MS);
		}
		usart_write(USART_COM, button2);
		if(pio_get(BUT_PIO_2, PIO_INPUT, BUT_IDX_MASK_2) == 0){
			butflag2 = 1;
		}
		else{
			butflag2 = 0;
			button2 = '0';
		}
		
		while(!usart_is_tx_ready(USART_COM)) {
			vTaskDelay(1 / portTICK_PERIOD_MS);
		}
		usart_write(USART_COM, button3);
		if(pio_get(BUT_PIO_3, PIO_INPUT, BUT_IDX_MASK_3) == 0){
			butflag3 = 1;
		}
		else{
			butflag3 = 0;
			button3 = '0';
		}
		
		while(!usart_is_tx_ready(USART_COM)) {
			vTaskDelay(1 / portTICK_PERIOD_MS);
		}
		usart_write(USART_COM, button4);
		if(pio_get(BUT_PIO_4, PIO_INPUT, BUT_IDX_MASK_4) == 0){
			butflag4 = 1;
		}
		else{
			butflag4 = 0;
			button4 = '0';
		}
		
		while(!usart_is_tx_ready(USART_COM)) {
			vTaskDelay(1 / portTICK_PERIOD_MS);
		}
		usart_write(USART_COM, button5);
		if(pio_get(BUT_PIO_5, PIO_INPUT, BUT_IDX_MASK_5) == 0){
			butflag5 = 1;
		}
		else{
			butflag5 = 0;
			button5 = '0';
		}
		
		while(!usart_is_tx_ready(USART_COM)) {
			vTaskDelay(1 / portTICK_PERIOD_MS);
		}
		usart_write(USART_COM, button6);
		if(pio_get(BUT_PIO_6, PIO_INPUT, BUT_IDX_MASK_6) == 0){
			butflag6 = 1;
		}
		else{
			butflag6 = 0;
			button6 = '0';
		}
		
		while(!usart_is_tx_ready(USART_COM)) {
			vTaskDelay(1 / portTICK_PERIOD_MS);
		}
		usart_write(USART_COM, button7);
		if(pio_get(BUT_PIO_7, PIO_INPUT, BUT_IDX_MASK_7) == 0){
			butflag7 = 1;
		}
		else{
			butflag7 = 0;
			button7 = '0';
		}
		
		while(!usart_is_tx_ready(USART_COM)) {
			vTaskDelay(1 / portTICK_PERIOD_MS);
		}
		
		usart_write(USART_COM, button8);
		
		while(!usart_is_tx_ready(USART_COM)) {
			vTaskDelay(1 / portTICK_PERIOD_MS);
		}
		
		usart_write(USART_COM, button9);
		
		while(!usart_is_tx_ready(USART_COM)) {
			vTaskDelay(1 / portTICK_PERIOD_MS);
		}
		
		usart_write(USART_COM, button10);
		
		// envia fim de pacote
		while(!usart_is_tx_ready(USART_COM)) {
			vTaskDelay(1 / portTICK_PERIOD_MS);
		}
		usart_write(USART_COM, eof);

		// dorme por 500 ms
		vTaskDelay(50 / portTICK_PERIOD_MS);
		
	}
}

/************************************************************************/
/* main                                                                 */
/************************************************************************/

int main(void) {
	/* Initialize the SAM system */
	sysclk_init();
	board_init();
	configure_console();
	
	xQueueADC = xQueueCreate(100, sizeof(adcData));
	if (xQueueADC == NULL)
	printf("falha em criar a queue xQueueADC \n");

	/* Create task to make led blink */
	xTaskCreate(task_bluetooth, "BLT", TASK_BLUETOOTH_STACK_SIZE, NULL,	TASK_BLUETOOTH_STACK_PRIORITY, NULL);
	
	if (xTaskCreate(task_proc, "ADC", TASK_ADC_STACK_SIZE, NULL,
	TASK_ADC_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create test ADC task\r\n");
	}

	/* Start the scheduler. */
	vTaskStartScheduler();

	while(1){}

	/* Will only get here if there was insufficient memory to create the idle task. */
	return 0;
}
