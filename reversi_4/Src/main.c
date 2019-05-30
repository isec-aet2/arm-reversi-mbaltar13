/* USER CODE BEGIN Header */
//REVERSI - Marina Baltar
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32f769i_discovery.h"
#include "stm32f769i_discovery_lcd.h"
#include "stm32f769i_discovery_ts.h"
#include "stLogo.h"
#include <stdio.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TEMP_REFRESH_PERIOD   2000    /* Internal temperature refresh period */
#define MAX_CONVERTED_VALUE   4095    /* Max converted value */
#define AMBIENT_TEMP            25    /* Ambient Temperature */
#define VSENS_AT_AMBIENT_TEMP  760    /* VSENSE value (mv) at ambient temperature */
#define AVG_SLOPE               25    /* Avg_Solpe multiply by 10 */
#define VREF                  3300

#define COR_JOGADOR_1        LCD_COLOR_RED
#define PECA_JOGADOR_1       'X'
#define COR_JOGADOR_2        LCD_COLOR_GREEN
#define PECA_JOGADOR_2       'Y'
#define SEM_PECA	         'N'
#define JOGADA_POSSIVEL	     'P'
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

DMA2D_HandleTypeDef hdma2d;

DSI_HandleTypeDef hdsi;

LTDC_HandleTypeDef hltdc;

SD_HandleTypeDef hsd2;

TIM_HandleTypeDef htim6;

SDRAM_HandleTypeDef hsdram1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA2D_Init(void);
static void MX_FMC_Init(void);
static void MX_LTDC_Init(void);
static void MX_ADC1_Init(void);
static void MX_DSIHOST_DSI_Init(void);
static void MX_SDMMC2_SD_Init(void);
static void MX_TIM6_Init(void);
/* USER CODE BEGIN PFP */
static void LCD_Config();
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int adversario = 0;             // se 1 joga contra o ARM
int flag=0;                     // timer 6
int count = 0;                  // CONTA SEGUNDOS
int deadline = 20;              // segundos para fazer a jogada
uint32_t ConvertedValue;        // valor obtido da temperatura
long int JTemp;                 // valor convertido da temperatura
char desc[100];					// frases para impressão
int init_tick_led1 = 0;			// inicializa led
TS_StateTypeDef TS_State;		// verifica se o touch screen é utilizado
int ts_flag = 0;				// flag do touch screen
volatile int ver_quem_joga = 1; // começa no jogador 1
volatile char tabuleiro[8][8];	// tabuleiro
int passa_jogada_um = 0;		// conta vezes que o jogador 1 nao jogou seguidas
int passa_jogada_dois = 0;      // conta vezes que o jogador 2 nao jogou seguidas



void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){

	if(GPIO_Pin == GPIO_PIN_13){
		BSP_TS_GetState(&TS_State);
		ts_flag=1;
	    HAL_Delay(100);
	}

}

void HAL_TIM_PeriodElapsedCallback (TIM_HandleTypeDef *htim){
	if(htim->Instance == TIM6){
		flag=1;
		count++;
		deadline--;

		  if (count%2 == 0){
			  //ACTUALIZA O VALOR DA TEMPERATURA
			  ConvertedValue=HAL_ADC_GetValue(&hadc1); //get value
			  JTemp = ((((ConvertedValue * VREF)/MAX_CONVERTED_VALUE) - VSENS_AT_AMBIENT_TEMP) * 10 / AVG_SLOPE) + AMBIENT_TEMP;
		      if(HAL_GetTick() >= init_tick_led1 + 500)
		      {
		          init_tick_led1 = HAL_GetTick();
		          BSP_LED_Toggle(LED_GREEN); // cada vez que actualiza, pisca o led verde
		      }
		  }
	}
	flag=0;
}

void menu_inicial(){

	//BSP_LCD_DrawBitmap(25, 25, (uint8_t*)apostar_em_ti); //tentativa de meter um logotipo

	sprintf(desc, "REVERSI");
	BSP_LCD_SetFont(&Font24);
	BSP_LCD_SetTextColor(LCD_COLOR_RED);
	BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize()/2 - 50, (uint8_t *)desc, CENTER_MODE);

	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	sprintf(desc, "Escolha um adversario");
	BSP_LCD_SetFont(&Font20);
	BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize()/2 - 20, (uint8_t *)desc, CENTER_MODE);

	sprintf(desc, "Prima o botao azul para comecar o jogo");
	BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize()/2 + 10, (uint8_t *)desc, CENTER_MODE);

	sprintf(desc, "Humano");
	BSP_LCD_SetFont(&Font24);
	BSP_LCD_SetTextColor(LCD_COLOR_RED);
	BSP_LCD_DisplayStringAt(-250, BSP_LCD_GetYSize()/2 + 115, (uint8_t *)desc, CENTER_MODE);

	sprintf(desc, "ARM");
	BSP_LCD_DisplayStringAt(250, BSP_LCD_GetYSize()/2 + 115, (uint8_t *)desc, CENTER_MODE);
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);

}

void fim_do_jogo(int * jog_um, int * jog_dois, int * vencedor){
	int k = 0;
	int z = 0;


	for(k = 0; k < 8; k++){
		for(z = 0; z < 8; z++){
			if(tabuleiro[k][z] == PECA_JOGADOR_1){
				(*jog_um)++;     // conta quantas peças o jogador 1 ficaram no tabuleiro
			}
			else if(tabuleiro[k][z] == PECA_JOGADOR_2){
				(*jog_dois)++;   // conta quantas peças o jogador 2 ficaram no tabuleiro
			}
		}
	}

	if(*jog_um == *jog_dois){
		sprintf(desc, "Empate! Jogador 1: %d; Jogador 2: %d", *jog_um, *jog_dois);
		*vencedor = 0;
	}
	else if(*jog_um > *jog_dois){
		sprintf(desc, "Ganhou o Jogador 1! Jogador 1: %d; Jogador 2: %d", *jog_um, *jog_dois);
		*vencedor = 1;
	}
	else if(*jog_um < *jog_dois){
		sprintf(desc, "Ganhou o Jogador 2! Jogador 1: %d; Jogador 2: %d", *jog_um, *jog_dois);
		*vencedor = 2;
	}

	//mostra a mensagem do final do jogo
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE); //fundo
	BSP_LCD_FillRect(0, 50, BSP_LCD_GetXSize(), BSP_LCD_GetYSize()-50);
	BSP_LCD_SetFont(&Font20);
	BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
	BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize()/2 - 30, (uint8_t *)desc, CENTER_MODE);  // resultado
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	sprintf(desc, "Prima o botao azul para recomecar o jogo");
	BSP_LCD_SetFont(&Font16);
	BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize()/2 - 10, (uint8_t *)desc, CENTER_MODE);
}

void imprime_tabuleiro(){

	  BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);  // fundo da tabela
	  BSP_LCD_FillRect(BSP_LCD_GetXSize()/10, BSP_LCD_GetYSize()/10, 400, 400);
	  BSP_LCD_SetTextColor(LCD_COLOR_ORANGE);

	  int i=0;
	  for(i = 0; i<=8; i++){ // imprime colunas
		  BSP_LCD_DrawVLine(BSP_LCD_GetXSize()/10 + (BSP_LCD_GetXSize()/16)*i, BSP_LCD_GetYSize()/10, 400);
	  }

	  int j;
	  for(j = 0; j<=8; j++){ // imprime linhas
		  BSP_LCD_DrawHLine(BSP_LCD_GetXSize()/10, BSP_LCD_GetYSize()/10 + (BSP_LCD_GetYSize()/9.6)*j, 400);
	  }
	  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);

}

void mostra_temperatura(){
	  //Mostrar a temperatura interna
	  sprintf(desc, "Temperatura: %ld C", JTemp);
	  BSP_LCD_SetFont(&Font12);
	  BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize()/2 - 90, (uint8_t *)desc, RIGHT_MODE);
}

void mostra_tempo(){
	//Mostrar a duração do jogo
	sprintf(desc, "Tempo: %d segundos", count);
	BSP_LCD_SetFont(&Font12);
	BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize()/2 - 70, (uint8_t *)desc, RIGHT_MODE);
}

void mostra_deadline(){
	//Mostrar deadline - quanto tempo ainda sobra para poder jogar
	sprintf(desc, "   Faltam: %d segundos", deadline);
	BSP_LCD_SetFont(&Font20);
	BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize()/2 - 20, (uint8_t *)desc, RIGHT_MODE);
}

void mostra_quem_joga(){
	//Mostrar o jogador actual
    if (ver_quem_joga%2 == 1){
    	sprintf(desc, "Jogador 1");
    	BSP_LCD_SetFont(&Font16);
    	BSP_LCD_SetTextColor(COR_JOGADOR_1);
    	BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize()/2 - 50, (uint8_t *)desc, RIGHT_MODE);
    	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    }
    else{
    	sprintf(desc, "Jogador 2");
    	BSP_LCD_SetFont(&Font16);
    	BSP_LCD_SetTextColor(COR_JOGADOR_2);
    	BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize()/2 - 50, (uint8_t *)desc, RIGHT_MODE);
    	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    }
}

void imprime_pecas_iniciais(){

	//posiçoes iniciais do jogador 1
	BSP_LCD_SetTextColor(COR_JOGADOR_1);
	BSP_LCD_FillCircle(50*3 + 105, 50*3 + 75, 15);
	BSP_LCD_FillCircle(50*4 + 105, 50*4 + 75, 15);
	tabuleiro[3][3] = PECA_JOGADOR_1;
	tabuleiro[4][4] = PECA_JOGADOR_1;

	//posiçoes iniciais do jogador 2
	BSP_LCD_SetTextColor(COR_JOGADOR_2);
	BSP_LCD_FillCircle(50*3 + 105, 50*4 + 75, 15);
	BSP_LCD_FillCircle(50*4 + 105, 50*3 + 75, 15);
	tabuleiro[3][4] = PECA_JOGADOR_2;
	tabuleiro[4][3] = PECA_JOGADOR_2;

	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);

}

void imprime_jogada(float x, float y, int i, int j){
    if (ver_quem_joga%2 == 1){
    	tabuleiro[i][j] = PECA_JOGADOR_1;
		BSP_LCD_SetTextColor(COR_JOGADOR_1);
		BSP_LCD_FillCircle(x, y, 15);
    }
    else{
    	tabuleiro[i][j] = PECA_JOGADOR_2;
		BSP_LCD_SetTextColor(COR_JOGADOR_2);
		BSP_LCD_FillCircle(x, y, 15);
    }
}

void actualiza_pecas_tabuleiro(){
	int k = 0;
	int z = 0;


	for(k = 0; k < 8; k++){
		for(z = 0; z < 8; z++){
			if(tabuleiro[k][z] == PECA_JOGADOR_1){
				BSP_LCD_SetTextColor(COR_JOGADOR_1);
				BSP_LCD_FillCircle(50*k + 105, 50*z + 75, 15);
			}
			else if(tabuleiro[k][z]==PECA_JOGADOR_2){
				BSP_LCD_SetTextColor(COR_JOGADOR_2);
				BSP_LCD_FillCircle(50*k + 105, 50*z + 75, 15);
			}
			else if(tabuleiro[k][z]==SEM_PECA){
				BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
				BSP_LCD_FillCircle(50*k + 105, 50*z + 75, 15);
			}
			else if(tabuleiro[k][z]==JOGADA_POSSIVEL){
				BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
				BSP_LCD_FillCircle(50*k + 105, 50*z + 75, 5);
			}
		}
	}
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
}

void limpa_possibilidades(){
	int k = 0;
	int z = 0;


	for(k = 0; k < 8; k++){
		for(z = 0; z < 8; z++){
			if(tabuleiro[k][z] == JOGADA_POSSIVEL){
				tabuleiro[k][z] = SEM_PECA;
			}
		}
	}
}

int validar_com_self(int linsel, int colsel){
    char self, adv;
    int i = 0;
    int j = 0;

    if (ver_quem_joga%2 == 1){
    	self = PECA_JOGADOR_1;
    	adv = PECA_JOGADOR_2;
    }
    else{
    	self = PECA_JOGADOR_2;
    	adv = PECA_JOGADOR_1;
    }


    //ver relaçoes com as peças vizinhas
    //ESQUERDA
    if(tabuleiro[linsel][colsel-1] == adv){
        for(j = colsel-2; j >= 0 ;  j-- ){
            if(tabuleiro[linsel][j] == self){
                return 1;
            }
        }
    }


    //DIREITA
    if(tabuleiro[linsel][colsel+1] == adv){
        for(j = colsel + 2; j < 8 ;  j++ ){
            if(tabuleiro[linsel][j] == self){
                return 1;
            }
        }
    }


    //CIMA
    if(tabuleiro[linsel+1][colsel] == adv){
        for(i = linsel + 2; i < 8 ;  i++ ){
            if(tabuleiro[i][colsel] == self){
                return 1;
            }
        }
    }


    //BAIXO
    if(tabuleiro[linsel-1][colsel] == adv){
        for(i = linsel-2; i >= 0 ;  i-- ){
            if(tabuleiro[i][colsel] == self){
                return 1;
            }
        }
    }


   //DIAGONAL SUPERIOR ESQUERDA
   if(tabuleiro[linsel-1][colsel+1] == adv){
       for(i=linsel-2, j=colsel+2; i>=0 && j< 8; i--, j++){
           if(tabuleiro[i][j] == self){
               return 1;
           }
       }
   }


   // DIAGONAL INFERIOR DIREITA
     if(tabuleiro[linsel+1][colsel-1] == adv){ //verfica se há adversário junto à casa onde pretendemos jogar
       for(i=linsel+2, j=colsel-2; i<8 && j>= 0; i++, j--){
           if(tabuleiro[i][j] == self){ // ve se tem self
               return 1;
           }
       }
   }



     // DIAGONAL INFERIOR ESQUERDA
     if(tabuleiro[linsel+1][colsel+1] == adv){ //verfica se ha adversario junto
       for(i=linsel+2, j=colsel+2; i<8 && j< 8; i++, j++){
           if(tabuleiro[i][j] == self){ //verifica se ha self a seguir
               return 1;
           }
       }
   }


     // DIAGONAL SUPERIOR DIREITA
     if(tabuleiro[linsel-1][colsel-1] == adv){ //ve se ha adversario
       for(i=linsel-2, j=colsel-2; i>=0 && j>= 0; i--, j--){
           if(tabuleiro[i][j] == self){ //ve se ha self
               return 1;
           }
       }
   }


     // DIAGONAL SUPERIOR ESQUERDA
     if(tabuleiro[linsel-1][colsel+1] == adv){ //adversario ao lado
       for(i=linsel-2, j=colsel+2; i>=0 && j< 8; i--, j++){
           if(tabuleiro[i][j] == self){ //self a seguir ao adversario
               return 1;
           }
       }
   }

     return 0;

}

void jogadas_possiveis(){
    char adv;
    int i = 0;
    int j = 0;


    if (ver_quem_joga%2 == 1){
        adv = PECA_JOGADOR_2;
    }
    else{
    	adv = PECA_JOGADOR_1;
    }


    	for(i = 0; i < 8; i++){
    		for(j = 0; j < 8; j++){
    			if(i==0 && j==0){ //canto superior esquerdo
    				if((tabuleiro[0][1] == adv ||
    					tabuleiro[1][1] == adv ||
						tabuleiro[1][0] == adv)
    					&& tabuleiro[0][0]==SEM_PECA && validar_com_self(i, j)){

    						tabuleiro[i][j] = JOGADA_POSSIVEL;
    				}
    			}
    			else if(i==0 && j==7){ //canto superior direito
    				if((tabuleiro[0][6] == adv ||
    					tabuleiro[1][6] == adv ||
						tabuleiro[1][7] == adv)
    					&& tabuleiro[0][7]==SEM_PECA && validar_com_self(i, j)){

    						tabuleiro[i][j] = JOGADA_POSSIVEL;
    				}
    			}
    			else if(i==7 && j==0){ //canto inferior esquerdo
    				if((tabuleiro[6][0] == adv ||
    					tabuleiro[6][1] == adv ||
						tabuleiro[7][1] == adv)
    					&& tabuleiro[7][0]==SEM_PECA && validar_com_self(i, j)){

    						tabuleiro[i][j] = JOGADA_POSSIVEL;
    				}
    			}
    			else if(i==7 && j==7){ //canto inferior direito
    				if((tabuleiro[6][7] == adv ||
    					tabuleiro[6][6] == adv ||
						tabuleiro[7][6] == adv)
    					&& tabuleiro[7][7]==SEM_PECA && validar_com_self(i, j)){

    						tabuleiro[i][j] = JOGADA_POSSIVEL;
    				}
    			}
    			else if(i==0 && j!=0 && j!=7){ //linha de cima
    				if((tabuleiro[0][j-1] == adv ||
    					tabuleiro[0][j+1] == adv ||
						tabuleiro[1][j] == adv ||
						tabuleiro[1][j+1] == adv ||
						tabuleiro[1][j-1] == adv)
    					&& tabuleiro[0][j]==SEM_PECA && validar_com_self(i, j)){

    						tabuleiro[i][j] = JOGADA_POSSIVEL;
    				}
    			}
    			else if(j==0 && i!=0 && i!=7){ //linha esquerda
    				if((tabuleiro[i-1][0] == adv ||
    					tabuleiro[i+1][0] == adv ||
						tabuleiro[1][i-1] == adv ||
						tabuleiro[1][i+1] == adv ||
						tabuleiro[1][i] == adv)
    					&& tabuleiro[i][0]==SEM_PECA && validar_com_self(i, j)){

    						tabuleiro[i][j] = JOGADA_POSSIVEL;
    				}
    			}
    			else if(i==7 && j!=0 && j!=7){ //linha de baixo
    				if((tabuleiro[7][j-1] == adv ||
    					tabuleiro[7][j+1] == adv ||
						tabuleiro[6][j] == adv ||
						tabuleiro[6][j-1] == adv ||
						tabuleiro[6][j+1] == adv)
    					&& tabuleiro[7][j]==SEM_PECA && validar_com_self(i, j)){

    						tabuleiro[i][j] = JOGADA_POSSIVEL;
    				}
    			}
    			else if(j==7 && i!=0 && i!=7){ //linha direita
    				if((tabuleiro[i-1][7] == adv ||
    					tabuleiro[i+1][7] == adv ||
						tabuleiro[i][6] == adv ||
						tabuleiro[i-1][6] == adv||
						tabuleiro[i+1][6] == adv)
    					&& tabuleiro[i][7]==SEM_PECA && validar_com_self(i, j)){

    						tabuleiro[i][j] = JOGADA_POSSIVEL;
    				}
    			}
    			//regra geral:
    			else if((tabuleiro[i-1][j-1] == adv ||
    					tabuleiro[i-1][j] == adv ||
						tabuleiro[i-1][j+1] == adv ||
						tabuleiro[i+1][j-1] == adv ||
						tabuleiro[i+1][j] == adv ||
						tabuleiro[i+1][j+1] == adv ||
						tabuleiro[i][j-1] == adv ||
						tabuleiro[i][j+1] == adv)
    					&& tabuleiro[i][j]==SEM_PECA && validar_com_self(i, j)){

    						tabuleiro[i][j] = JOGADA_POSSIVEL;
    				}
    			}

    }
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
}


void vira_pecas(int linsel, int colsel){
	char self;
	char adv;
	int j = 0;
	int i = 0;
	int ok = 0;

    if (ver_quem_joga%2 == 1){
    	self = PECA_JOGADOR_1;
        adv = PECA_JOGADOR_2;
    }
    else{
    	self = PECA_JOGADOR_2;
    	adv = PECA_JOGADOR_1;
    }



    //ver relaçoes com as peças vizinhas
    //ESQUERDA
    if(tabuleiro[linsel][colsel-1] == adv){
        for(j = colsel-2; j >= 0 ;  j-- ){
            if(tabuleiro[linsel][j] == self){
                ok = 1;
                break;
            }
            else{
                ok = 0;
            }
        }
    }

    if(ok){
        for(j = colsel-1; j >= 0 && tabuleiro[linsel][j] == adv; j--){
        	tabuleiro[linsel][j] = self;
        }
    }

    ok = 0;


    //DIREITA
    if(tabuleiro[linsel][colsel+1] == adv){
        for(j = colsel + 2; j < 8 ;  j++ ){
            if(tabuleiro[linsel][j] == self){
                ok = 1;
                break;
            }
            else{
                ok = 0;
            }
        }
    }

    if(ok){
        for(j = colsel + 1; j < 8 && tabuleiro[linsel][j] == adv; j++){
        	tabuleiro[linsel][j] = self;
        }
    }

    ok = 0;


    //CIMA
    if(tabuleiro[linsel+1][colsel] == adv){
        for(i = linsel + 2; i < 8 ;  i++ ){
            if(tabuleiro[i][colsel] == self){
                ok = 1;
                break;
            }
            else{
                ok = 0;
            }
        }
    }


    if(ok){
        for(i = linsel + 1; i < 8 && tabuleiro[i][colsel] == adv; i++){
        	tabuleiro[i][colsel] = self;
        }
    }

    ok = 0;


    //BAIXO
    if(tabuleiro[linsel-1][colsel] == adv){
        for(i = linsel-2; i >= 0 ;  i-- ){
            if(tabuleiro[i][colsel] == self){
                ok = 1;
                break;
            }
            else{
                ok = 0;
            }
        }
    }

    if(ok){
        for(i = linsel-1; i >= 0 && tabuleiro[i][colsel] == adv; i--){
        	tabuleiro[i][colsel] = self;
        }
    }

    ok = 0;


    //DIAGONAL SUPERIOR ESQUERDA
    if(tabuleiro[linsel-1][colsel+1] == adv){
       for(i=linsel-2, j=colsel+2; i>=0 && j< 8; i--, j++){
           if(tabuleiro[i][j] == self){
               ok = 1;
               break;
           }
           else{
               ok = 0;
           }
       }
    }

    if(ok){
       for(i=linsel-1, j=colsel+1; i>=0 && j< 8 && tabuleiro[i][j] == adv; i--, j++){
    	  tabuleiro[i][j] = self;
       }
    }

    ok = 0;


    //DIAGONAL INFERIOR ESQUERDA
    if(tabuleiro[linsel+1][colsel-1] == adv){
        for(i=linsel+2, j=colsel-2; i<8 && j>= 0; i++, j--){
            if(tabuleiro[i][j] == self){
                ok = 1;
                break;
            }
            else{
                ok = 0;
            }
        }
    }

    if(ok){
       for(i=linsel+1, j=colsel-1; i<8 && j>= 0 && tabuleiro[i][j] == adv; i++, j--){
     	  tabuleiro[i][j] = self;
       }
    }

    ok = 0;


    //DIAGONAL SUPERIOR DIREITA
    if(tabuleiro[linsel+1][colsel+1] == adv){
        for(i=linsel+2, j=colsel+2; i<8 && j< 8; i++, j++){
            if(tabuleiro[i][j] == self){
               ok = 1;
               break;
            }
            else{
               ok = 0;
            }
        }
    }

    if(ok){
       for(int i=linsel+1, j=colsel+1; i<8 && j < 8 && tabuleiro[i][j] == adv; i++, j++){
     	  tabuleiro[i][j] = self;
        }
    }

    ok = 0;


    //DIAGONAL INFERIOR ESQUERDA
    if(tabuleiro[linsel-1][colsel-1] == adv){
       for(i=linsel-2, j=colsel-2; i>=0 && j>= 0; i--, j--){
            if(tabuleiro[i][j] == self){
                ok = 1;
                break;
            }
            else{
                ok = 0;
            }
        }
    }

    if(ok){
       for(i=linsel-1, j=colsel-1; i>=0 && j>= 0 && tabuleiro[i][j] == adv; i--, j--){
     	  tabuleiro[i][j] = self;
       }
    }

    ok = 0;


}

void jogada_automatica(){

    int i = 0;
    int j = 0;
    int conta_valida = 0;

    HAL_Delay(500);

    for(i=0; i<8; i++){
    	for(j=0; j<8; j++){
    		if(tabuleiro[i][j] == JOGADA_POSSIVEL){
    			conta_valida++;
    				if(conta_valida == 1){
    					tabuleiro[i][j] = PECA_JOGADOR_2;
    					vira_pecas(i, j);
    				}
    				else if(conta_valida > 1){
    					tabuleiro[i][j] = SEM_PECA;
    				}
    		}
    	}
    }

    ver_quem_joga++;
	limpa_possibilidades();
	jogadas_possiveis();
	actualiza_pecas_tabuleiro();

}

void tocar_ecran_menu_inicial(){

	if(ts_flag==1){
		ts_flag=0;
			if(TS_State.touchX[0]>500 && TS_State.touchY[0]>=250){
				BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
				BSP_LCD_FillCircle(TS_State.touchX[0], TS_State.touchY[0], 20);
				adversario = 1;
			}
			if(TS_State.touchX[0]<300 && TS_State.touchY[0]>=250){
				BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
				BSP_LCD_FillCircle(TS_State.touchX[0], TS_State.touchY[0], 20);
				adversario = 0;
			}
	}

}

void dinamica_de_jogo(float x, float y, int i, int j){
	if(tabuleiro[i][j]==JOGADA_POSSIVEL && deadline >= 0){
		deadline = 20;
		if (ver_quem_joga%2 == 1){
			  passa_jogada_um = 0;
		}
		else{
			  passa_jogada_dois = 0;
		}

		imprime_jogada(x, y, i, j);
		vira_pecas(i, j);
		ver_quem_joga++;
		limpa_possibilidades();
		jogadas_possiveis();
		actualiza_pecas_tabuleiro();
	}
}

void tocar_ecran(){

	int i=0;
	int j=0;
	float x = 0.0;
	float y = 0.0;



	if(ts_flag==1){
		ts_flag=0;
			if(TS_State.touchX[0]>=(BSP_LCD_GetXSize()/10+15) && TS_State.touchY[0]>=(BSP_LCD_GetYSize()/10+15) && TS_State.touchX[0]<=475 && TS_State.touchY[0]<=450){

				for(i=0; i<8; i++){
					if((TS_State.touchX[0]) >= 50*i + 80 && (TS_State.touchX[0]) < 50*i + 130){
						x = 50*i + 105; // converte para o ponto medio dos quadrados da tabela
						break;
					}
				}

				for(j=0; j<8; j++){
					if((TS_State.touchY[0]) >= (50*j) && (TS_State.touchY[0]) < (50*j+100)){
						y = 50*j + 75;  // converte para o ponto medio dos quadrados da tabela
						break;
					}
				}


				dinamica_de_jogo(x, y, i, j);


		}
	}
}


int nao_e_possivel_continuar_jogo(){
	int i = 0;
	int j = 0;
	int conta_self = 0;
	int conta_possiveis = 0;
	int conta_vazias = 0;

	char self;

    if (ver_quem_joga%2 == 1){
        self = PECA_JOGADOR_1;
    }
    else{
    	self = PECA_JOGADOR_2;
    }

	for (i = 0; i < 8; i++){
		for (j = 0; j < 8; j++){
			if(tabuleiro[i][j] == self){
				conta_self++;
			}
			else if(tabuleiro[i][j] == SEM_PECA){
				conta_vazias++;
			}
			else if(tabuleiro[i][j] == JOGADA_POSSIVEL){
				conta_possiveis++;
			}
		}
	}
	if(conta_self == 0){          // sem peças do proprio
		return 1;                 // acabou o jogo
	}
	else if(conta_vazias == 0){   // sem posições vagas
		if(conta_possiveis == 0){ // nem jogadas possiveis
			return 1;			  // acabou o jogo
		}
		else{
			return 0;
		}
	}
	else{
		return 0;
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
	unsigned int nBytes; // feedback da passagem para o cartao
	int i = 0;			 // clear do tabuleiro antes de um novo jogo
	int j = 0;
	int jog_um = 0;		 // pontuação do jogador 1
	int jog_dois = 0;    // pontuação do jogador 2
	int vencedor = 0;    // quem ganhou

  /* USER CODE END 1 */
  

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

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
  MX_DMA2D_Init();
  MX_FMC_Init();
  MX_LTDC_Init();
  MX_ADC1_Init();
  MX_DSIHOST_DSI_Init();
  MX_SDMMC2_SD_Init();
  MX_TIM6_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_Base_Start_IT(&htim6);
  LCD_Config();
  BSP_LED_Init(LED_GREEN);
  BSP_LED_Init(LED_RED);
  BSP_PB_Init(BUTTON_WAKEUP, BUTTON_MODE_GPIO);
//start do adc
  HAL_ADC_Start(&hadc1);


  BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
  BSP_TS_ITConfig();


  jump:

  HAL_Delay(250);
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_FillRect(0, 50, BSP_LCD_GetXSize(), BSP_LCD_GetYSize()-50);
  adversario = 0;

  while(BSP_PB_GetState(BUTTON_WAKEUP)!=1){
	  menu_inicial();
	  tocar_ecran_menu_inicial();
	  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  }
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_FillRect(0, 50, BSP_LCD_GetXSize(), BSP_LCD_GetYSize()-50);

  ver_quem_joga = 1;
  count = 0;
  deadline = 20;
  passa_jogada_um = 0;
  passa_jogada_dois = 0;

  imprime_tabuleiro();

  for (i = 0; i < 8; i++){
	  for (j = 0; j < 8; j++){
		  tabuleiro[i][j] = SEM_PECA;
	  }
  }


  imprime_pecas_iniciais();

  jogadas_possiveis();
  actualiza_pecas_tabuleiro();


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	  mostra_temperatura();
	  mostra_tempo();
	  mostra_deadline();
	  mostra_quem_joga();
	  tocar_ecran();

	  if(BSP_PB_GetState(BUTTON_WAKEUP)==1 && count>1){
		  goto jump;
	  }


	  if(adversario==1){
		  if(ver_quem_joga%2==0){
				limpa_possibilidades();
				jogadas_possiveis();
				actualiza_pecas_tabuleiro();
				passa_jogada_dois = 0;
				jogada_automatica();
		  }
	  }

	  if(deadline < 0){
		  deadline = 20;

		  if (ver_quem_joga%2 == 1){
			  passa_jogada_um++;
		  }
		  else{
			  passa_jogada_dois++;
		  }

		  ver_quem_joga++;
		  limpa_possibilidades();
		  jogadas_possiveis();
		  actualiza_pecas_tabuleiro();
		  tocar_ecran();
	  }

	  if(nao_e_possivel_continuar_jogo() || passa_jogada_um >= 3 || passa_jogada_dois >= 3){
		  fim_do_jogo(&jog_um, &jog_dois, &vencedor);

		  if (f_mount(&SDFatFS, SDPath, 0) != FR_OK){
		          Error_Handler();
		      }

		      if (f_open(&SDFile, "reversi.txt", FA_CREATE_ALWAYS | FA_WRITE ) != FR_OK){
		          Error_Handler();
		      }

			  if(vencedor == 0){
				  sprintf(desc, "Empate a %d! A partida durou %d segundos!\n", jog_um, count);
			  }
			  else if(vencedor == 1){
				  sprintf(desc, "Ganhou o jogador 1! Jog 1: %d; Jog 2: %d - A partida durou %d segundos!\n", jog_um, jog_dois, count);
			  }
			  else if(vencedor == 2){
				  sprintf(desc, "Ganhou o jogador 2! Jog 1: %d; Jog 2: %d - A partida durou %d segundos!\n", jog_um, jog_dois, count);
			  }

		          f_write(&SDFile, desc, strlen(desc), &nBytes);
		          f_close(&SDFile);

		          jog_um = 0;
		          jog_dois = 0;
		          vencedor = 0;

  		  while(BSP_PB_GetState(BUTTON_WAKEUP)!=1){

  		  }
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
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 400;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Activate the Over-Drive mode 
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC|RCC_PERIPHCLK_SDMMC2
                              |RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
  PeriphClkInitStruct.PLLSAI.PLLSAIR = 2;
  PeriphClkInitStruct.PLLSAI.PLLSAIQ = 2;
  PeriphClkInitStruct.PLLSAI.PLLSAIP = RCC_PLLSAIP_DIV2;
  PeriphClkInitStruct.PLLSAIDivQ = 1;
  PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_2;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLL;
  PeriphClkInitStruct.Sdmmc2ClockSelection = RCC_SDMMC2CLKSOURCE_CLK48;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
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
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
  */
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_56CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief DMA2D Initialization Function
  * @param None
  * @retval None
  */
static void MX_DMA2D_Init(void)
{

  /* USER CODE BEGIN DMA2D_Init 0 */

  /* USER CODE END DMA2D_Init 0 */

  /* USER CODE BEGIN DMA2D_Init 1 */

  /* USER CODE END DMA2D_Init 1 */
  hdma2d.Instance = DMA2D;
  hdma2d.Init.Mode = DMA2D_M2M;
  hdma2d.Init.ColorMode = DMA2D_OUTPUT_ARGB8888;
  hdma2d.Init.OutputOffset = 0;
  hdma2d.LayerCfg[1].InputOffset = 0;
  hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_ARGB8888;
  hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hdma2d.LayerCfg[1].InputAlpha = 0;
  hdma2d.LayerCfg[1].AlphaInverted = DMA2D_REGULAR_ALPHA;
  hdma2d.LayerCfg[1].RedBlueSwap = DMA2D_RB_REGULAR;
  if (HAL_DMA2D_Init(&hdma2d) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMA2D_ConfigLayer(&hdma2d, 1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DMA2D_Init 2 */

  /* USER CODE END DMA2D_Init 2 */

}

/**
  * @brief DSIHOST Initialization Function
  * @param None
  * @retval None
  */
static void MX_DSIHOST_DSI_Init(void)
{

  /* USER CODE BEGIN DSIHOST_Init 0 */

  /* USER CODE END DSIHOST_Init 0 */

  DSI_PLLInitTypeDef PLLInit = {0};
  DSI_HOST_TimeoutTypeDef HostTimeouts = {0};
  DSI_PHY_TimerTypeDef PhyTimings = {0};
  DSI_LPCmdTypeDef LPCmd = {0};
  DSI_CmdCfgTypeDef CmdCfg = {0};

  /* USER CODE BEGIN DSIHOST_Init 1 */

  /* USER CODE END DSIHOST_Init 1 */
  hdsi.Instance = DSI;
  hdsi.Init.AutomaticClockLaneControl = DSI_AUTO_CLK_LANE_CTRL_DISABLE;
  hdsi.Init.TXEscapeCkdiv = 4;
  hdsi.Init.NumberOfLanes = DSI_ONE_DATA_LANE;
  PLLInit.PLLNDIV = 20;
  PLLInit.PLLIDF = DSI_PLL_IN_DIV1;
  PLLInit.PLLODF = DSI_PLL_OUT_DIV1;
  if (HAL_DSI_Init(&hdsi, &PLLInit) != HAL_OK)
  {
    Error_Handler();
  }
  HostTimeouts.TimeoutCkdiv = 1;
  HostTimeouts.HighSpeedTransmissionTimeout = 0;
  HostTimeouts.LowPowerReceptionTimeout = 0;
  HostTimeouts.HighSpeedReadTimeout = 0;
  HostTimeouts.LowPowerReadTimeout = 0;
  HostTimeouts.HighSpeedWriteTimeout = 0;
  HostTimeouts.HighSpeedWritePrespMode = DSI_HS_PM_DISABLE;
  HostTimeouts.LowPowerWriteTimeout = 0;
  HostTimeouts.BTATimeout = 0;
  if (HAL_DSI_ConfigHostTimeouts(&hdsi, &HostTimeouts) != HAL_OK)
  {
    Error_Handler();
  }
  PhyTimings.ClockLaneHS2LPTime = 28;
  PhyTimings.ClockLaneLP2HSTime = 33;
  PhyTimings.DataLaneHS2LPTime = 15;
  PhyTimings.DataLaneLP2HSTime = 25;
  PhyTimings.DataLaneMaxReadTime = 0;
  PhyTimings.StopWaitTime = 0;
  if (HAL_DSI_ConfigPhyTimer(&hdsi, &PhyTimings) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DSI_ConfigFlowControl(&hdsi, DSI_FLOW_CONTROL_BTA) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DSI_SetLowPowerRXFilter(&hdsi, 10000) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DSI_ConfigErrorMonitor(&hdsi, HAL_DSI_ERROR_NONE) != HAL_OK)
  {
    Error_Handler();
  }
  LPCmd.LPGenShortWriteNoP = DSI_LP_GSW0P_DISABLE;
  LPCmd.LPGenShortWriteOneP = DSI_LP_GSW1P_DISABLE;
  LPCmd.LPGenShortWriteTwoP = DSI_LP_GSW2P_DISABLE;
  LPCmd.LPGenShortReadNoP = DSI_LP_GSR0P_DISABLE;
  LPCmd.LPGenShortReadOneP = DSI_LP_GSR1P_DISABLE;
  LPCmd.LPGenShortReadTwoP = DSI_LP_GSR2P_DISABLE;
  LPCmd.LPGenLongWrite = DSI_LP_GLW_DISABLE;
  LPCmd.LPDcsShortWriteNoP = DSI_LP_DSW0P_DISABLE;
  LPCmd.LPDcsShortWriteOneP = DSI_LP_DSW1P_DISABLE;
  LPCmd.LPDcsShortReadNoP = DSI_LP_DSR0P_DISABLE;
  LPCmd.LPDcsLongWrite = DSI_LP_DLW_DISABLE;
  LPCmd.LPMaxReadPacket = DSI_LP_MRDP_DISABLE;
  LPCmd.AcknowledgeRequest = DSI_ACKNOWLEDGE_DISABLE;
  if (HAL_DSI_ConfigCommand(&hdsi, &LPCmd) != HAL_OK)
  {
    Error_Handler();
  }
  CmdCfg.VirtualChannelID = 0;
  CmdCfg.ColorCoding = DSI_RGB888;
  CmdCfg.CommandSize = 640;
  CmdCfg.TearingEffectSource = DSI_TE_EXTERNAL;
  CmdCfg.TearingEffectPolarity = DSI_TE_RISING_EDGE;
  CmdCfg.HSPolarity = DSI_HSYNC_ACTIVE_LOW;
  CmdCfg.VSPolarity = DSI_VSYNC_ACTIVE_LOW;
  CmdCfg.DEPolarity = DSI_DATA_ENABLE_ACTIVE_HIGH;
  CmdCfg.VSyncPol = DSI_VSYNC_FALLING;
  CmdCfg.AutomaticRefresh = DSI_AR_ENABLE;
  CmdCfg.TEAcknowledgeRequest = DSI_TE_ACKNOWLEDGE_DISABLE;
  if (HAL_DSI_ConfigAdaptedCommandMode(&hdsi, &CmdCfg) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DSI_SetGenericVCID(&hdsi, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DSIHOST_Init 2 */

  /* USER CODE END DSIHOST_Init 2 */

}

/**
  * @brief LTDC Initialization Function
  * @param None
  * @retval None
  */
static void MX_LTDC_Init(void)
{

  /* USER CODE BEGIN LTDC_Init 0 */

  /* USER CODE END LTDC_Init 0 */

  LTDC_LayerCfgTypeDef pLayerCfg = {0};
  LTDC_LayerCfgTypeDef pLayerCfg1 = {0};

  /* USER CODE BEGIN LTDC_Init 1 */

  /* USER CODE END LTDC_Init 1 */
  hltdc.Instance = LTDC;
  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  hltdc.Init.HorizontalSync = 7;
  hltdc.Init.VerticalSync = 3;
  hltdc.Init.AccumulatedHBP = 14;
  hltdc.Init.AccumulatedVBP = 5;
  hltdc.Init.AccumulatedActiveW = 654;
  hltdc.Init.AccumulatedActiveH = 485;
  hltdc.Init.TotalWidth = 660;
  hltdc.Init.TotalHeigh = 487;
  hltdc.Init.Backcolor.Blue = 0;
  hltdc.Init.Backcolor.Green = 0;
  hltdc.Init.Backcolor.Red = 0;
  if (HAL_LTDC_Init(&hltdc) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg.WindowX0 = 0;
  pLayerCfg.WindowX1 = 0;
  pLayerCfg.WindowY0 = 0;
  pLayerCfg.WindowY1 = 0;
  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_ARGB8888;
  pLayerCfg.Alpha = 0;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
  pLayerCfg.FBStartAdress = 0;
  pLayerCfg.ImageWidth = 0;
  pLayerCfg.ImageHeight = 0;
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg1.WindowX0 = 0;
  pLayerCfg1.WindowX1 = 0;
  pLayerCfg1.WindowY0 = 0;
  pLayerCfg1.WindowY1 = 0;
  pLayerCfg1.PixelFormat = LTDC_PIXEL_FORMAT_ARGB8888;
  pLayerCfg1.Alpha = 0;
  pLayerCfg1.Alpha0 = 0;
  pLayerCfg1.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  pLayerCfg1.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
  pLayerCfg1.FBStartAdress = 0;
  pLayerCfg1.ImageWidth = 0;
  pLayerCfg1.ImageHeight = 0;
  pLayerCfg1.Backcolor.Blue = 0;
  pLayerCfg1.Backcolor.Green = 0;
  pLayerCfg1.Backcolor.Red = 0;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg1, 1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LTDC_Init 2 */

  /* USER CODE END LTDC_Init 2 */

}

/**
  * @brief SDMMC2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDMMC2_SD_Init(void)
{

  /* USER CODE BEGIN SDMMC2_Init 0 */

  /* USER CODE END SDMMC2_Init 0 */

  /* USER CODE BEGIN SDMMC2_Init 1 */

  /* USER CODE END SDMMC2_Init 1 */
  hsd2.Instance = SDMMC2;
  hsd2.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
  hsd2.Init.ClockBypass = SDMMC_CLOCK_BYPASS_DISABLE;
  hsd2.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd2.Init.BusWide = SDMMC_BUS_WIDE_1B;
  hsd2.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd2.Init.ClockDiv = 0;
  /* USER CODE BEGIN SDMMC2_Init 2 */

  /* USER CODE END SDMMC2_Init 2 */

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 9999;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 9999;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/* FMC initialization function */
static void MX_FMC_Init(void)
{

  /* USER CODE BEGIN FMC_Init 0 */

  /* USER CODE END FMC_Init 0 */

  FMC_SDRAM_TimingTypeDef SdramTiming = {0};

  /* USER CODE BEGIN FMC_Init 1 */

  /* USER CODE END FMC_Init 1 */

  /** Perform the SDRAM1 memory initialization sequence
  */
  hsdram1.Instance = FMC_SDRAM_DEVICE;
  /* hsdram1.Init */
  hsdram1.Init.SDBank = FMC_SDRAM_BANK2;
  hsdram1.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_8;
  hsdram1.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_13;
  hsdram1.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_32;
  hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
  hsdram1.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_1;
  hsdram1.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
  hsdram1.Init.SDClockPeriod = FMC_SDRAM_CLOCK_DISABLE;
  hsdram1.Init.ReadBurst = FMC_SDRAM_RBURST_DISABLE;
  hsdram1.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_0;
  /* SdramTiming */
  SdramTiming.LoadToActiveDelay = 16;
  SdramTiming.ExitSelfRefreshDelay = 16;
  SdramTiming.SelfRefreshTime = 16;
  SdramTiming.RowCycleDelay = 16;
  SdramTiming.WriteRecoveryTime = 16;
  SdramTiming.RPDelay = 16;
  SdramTiming.RCDDelay = 16;

  if (HAL_SDRAM_Init(&hsdram1, &SdramTiming) != HAL_OK)
  {
    Error_Handler( );
  }

  /* USER CODE BEGIN FMC_Init 2 */

  /* USER CODE END FMC_Init 2 */
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOJ_CLK_ENABLE();

  /*Configure GPIO pin : PI13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  /*Configure GPIO pin : PI15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */
static void LCD_Config(void)
{
  uint32_t  lcd_status = LCD_OK;

  /* Initialize the LCD */
  lcd_status = BSP_LCD_Init();
  while(lcd_status != LCD_OK);

  BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);

  /* Clear the LCD */
  BSP_LCD_Clear(LCD_COLOR_WHITE);

  /* Set LCD Example description */
  BSP_LCD_SetTextColor(LCD_COLOR_DARKBLUE);
  BSP_LCD_SetFont(&Font12);
  BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize()- 20, (uint8_t *)"Copyright (c) Laufeyson 2019", CENTER_MODE);
  BSP_LCD_SetTextColor(LCD_COLOR_ORANGE);
  BSP_LCD_FillRect(0, 0, BSP_LCD_GetXSize(), 50);
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_SetBackColor(LCD_COLOR_ORANGE);
  BSP_LCD_SetFont(&Font24);
  BSP_LCD_DisplayStringAt(0, 10, (uint8_t *)"REVERSI", CENTER_MODE);
  BSP_LCD_SetFont(&Font16);

  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
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
	while(1){
		BSP_LED_Toggle(LED_RED);
		HAL_Delay(500);
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
