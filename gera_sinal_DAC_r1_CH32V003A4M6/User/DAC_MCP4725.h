/*
 * DAC_MCP4725.h
 *
 *  Created on: Oct 20, 2024
 *      Author: Aluizio d'Affonsêca Netto
 */

#ifndef USER_DAC_MCP4725_H_
#define USER_DAC_MCP4725_H_

#include <ch32v00x.h>


void MCP_Init(uint16_t A0);
void MCP_fast_start(uint16_t A0) ;
void MCP_fast_stop(void);
void MCP_fast_sample(uint16_t sample);
void MCP_fast_sample_single(uint16_t A0, uint16_t sample);



#endif /* USER_DAC_MCP4725_H_ */
