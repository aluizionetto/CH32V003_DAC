/*
 * usart_com.h
 *
 *  Created on: Nov 29, 2024
 *      Author: Aluizio d'Affonseca Netto
 */

#ifndef USER_USART_COM_H_
#define USER_USART_COM_H_

#include <ch32v00x.h>
void USARTx_CFG(void);   //configura porta USART
void USART_char(char c); //coloca amostra no buffer de TX
void USART_str(char *d);
void USART_int(int32_t num, int16_t pdiv);
void conv_int2str (char *bf,uint32_t num, uint8_t cat, int16_t pdiv) ;
void str_cat(char *bf, char *cat) ;
uint32_t str_len(char *bf) ;
int32_t conv_strtoint(char *bf);
char USART_get(void);  //retorna amostra disponível do buffer rx
int USART_available_rx (void);  //retorna numero de amostras disponíveis no buffer


#endif /* USER_USART_COM_H_ */
