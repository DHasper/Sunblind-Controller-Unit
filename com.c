#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "com.h"
#include "shutter.h"
#include "sensors.h"

#define F_CPU					16000000
#include <util/delay.h>
#define BAUD					57600
#define BRC						((F_CPU/16/BAUD) - 1)

#define TX_BUFFER_SIZE			128
#define INSTRUCTION_BUFFER_SIZE	50
#define DATA_STRING_LENGTH		100

char transmit_buffer[TX_BUFFER_SIZE];
uint8_t transmit_read_pos = 0;
uint8_t transmit_write_pos = 0;

char instruction_buffer[INSTRUCTION_BUFFER_SIZE];
uint8_t instruction_read_pos = 0;
bool is_reading_instruction = false;

// instructions must consist of 4 characters
// some examples for instructions:
char hs_instruction[4] = "HAND";
char shutter_in_instruction[4] = "shin";
char shutter_out_instruction[4] = "shou";
char set_min_light_instruction[4] = "minl";
char set_max_light_instruction[4] = "maxl";
char set_min_temp_instruction[4] = "mint";
char set_max_temp_instruction[4] = "maxt";
char set_min_dist_instruction[4] = "mind";
char set_max_dist_instruction[4] = "maxd";
char set_update_override_instruction[4] = "tupo";

// stores instruction argument
char arg[INSTRUCTION_BUFFER_SIZE-4];

void init_com(void){
	// set USART baud rate
	UBRR0H = (BRC >> 8);
	UBRR0L = BRC;
	
	// enable USART tx and rx + interrupts
	UCSR0B = (1 << TXEN0)	| (1 << TXCIE0) | (1 << RXEN0) | (1 << RXCIE0);
	// set data size to 8-bit
	UCSR0C = (1 << UCSZ01)	| (1 << UCSZ00);
}

void send_interface_vals(){
	// sonar data
	char sonar_data[] = "US";
	char sonar_string[50];
	dtostrf(get_sonar(), sizeof get_sonar(), 2, sonar_string);
	strcat(sonar_data, sonar_string);
	
	// shutter state data
	char shutter_state_data[] = "ST";
	char shutter_state_string[5];
	sprintf(shutter_state_string, "%d", get_shutter_state());
	strcat(shutter_state_data, shutter_state_string);
	
	// override state data
	char override_state_data[] = "OS";
	char override_state_string[5];
	sprintf(override_state_string, "%d", get_update_override());
	strcat(override_state_data, override_state_string);
	
	// min dist boundary
	char min_dist_data[] = "D0";
	char min_dist_string[50];
	sprintf(min_dist_string, "%u", get_min_dist());
	strcat(min_dist_data, min_dist_string);
	
	// max dist boundary
	char max_dist_data[] = "D1";
	char max_dist_string[50];
	sprintf(max_dist_string, "%u", get_max_dist());
	strcat(max_dist_data, max_dist_string);
	
	// min sensor boundary
	char min_sensor_data[] = "S0";
	char min_sensor_string[50];
		
	// max sensor boundary
	char max_sensor_data[] = "S1";
	char max_sensor_string[50];	
	
	// sensor data
	char sensor_data[] = "";
	char temp_code[] = "DT0&TS";
	char light_code[] = "DT1&LS";
	char sensor_string[50];
	
	switch(DEVICE_TYPE){
		case 0:
			// temperature data
			strcat(sensor_data, temp_code);
			dtostrf(get_temperature(), sizeof get_temperature(), 2, sensor_string);
			strcat(sensor_data, sensor_string);	
			
			sprintf(max_sensor_string, "%u", get_max_temp());
			strcat(max_sensor_data, max_sensor_string);
			
			sprintf(min_sensor_string, "%u", get_min_temp());
			strcat(min_sensor_data, min_sensor_string);
			break;
		case 1:
			// light intensity data
			strcat(sensor_data, light_code);
			dtostrf(get_light_intensity(), sizeof get_light_intensity(), 2, sensor_string);
			strcat(sensor_data, sensor_string);
			
			sprintf(max_sensor_string, "%u", get_max_light());
			strcat(max_sensor_data, max_sensor_string);
			
			sprintf(min_sensor_string, "%u", get_min_light());
			strcat(min_sensor_data, min_sensor_string);
			break;
	}
	
	// concat data string
	char datastr[DATA_STRING_LENGTH] = "";
	strcat(datastr, "DN");
	strcat(datastr, DEVICE_NAME);
	strcat(datastr, "&");
	strcat(datastr, sonar_data);
	strcat(datastr, "&");
	strcat(datastr, sensor_data);
	strcat(datastr, "&");
	strcat(datastr, min_dist_data);
	strcat(datastr, "&");
	strcat(datastr, max_dist_data);
	strcat(datastr, "&");
	strcat(datastr, min_sensor_data);
	strcat(datastr, "&");
	strcat(datastr, max_sensor_data);
	strcat(datastr, "&");
	strcat(datastr, override_state_data);
	strcat(datastr, "&");
	strcat(datastr, shutter_state_data);
	strcat(datastr, "\n");
	
	// send data string
	sendData(datastr);
}

void sendData(char c[]){
	// put all chars in array in transmit buffer
	for(uint8_t i = 0; i < strlen(c); i++){
		transmit_buffer[transmit_write_pos] = c[i];
		transmit_write_pos++;
		
		// if end of buffer is reached return to pos 0
		if(transmit_write_pos >= TX_BUFFER_SIZE){
			transmit_write_pos = 0;
		}
	}
	
	// this triggers the first interrupt
	if(UCSR0A & (1 << UDRE0)){
		UDR0 = 0;
	}
}

void get_arg(){
	// clear arg var first
	memset(arg, 0, sizeof arg);
	
	// get chars after first 4 from instruction
	for(volatile int i = 4; i <= INSTRUCTION_BUFFER_SIZE; i++){
		arg[i-4] = instruction_buffer[i];
	}
}

// interrupt on transmit finished
ISR(USART_TX_vect){
	// send next char in buffer if there are any
	if(transmit_read_pos != transmit_write_pos){
		UDR0 = transmit_buffer[transmit_read_pos];	
		transmit_read_pos++;
		
		// return to pos 0 when end of buffer is reached
		if(transmit_read_pos >= TX_BUFFER_SIZE){
			transmit_read_pos = 0;
		}
	}
}

// interrupt on receiving finished
ISR(USART_RX_vect){
	// get latest char from UDR0
	char c = UDR0;
	
	// start reading instruction
	if(c == ':'){
		is_reading_instruction = true;
	}
	// stop reading instruction, execute instruction and reset
	else if(c == '!'){
		is_reading_instruction = false;
		
		// handshake instruction
		if(memcmp(instruction_buffer, hs_instruction, 4) == 0){
			sendData("SHAKE\n");
		}
		// roll in shutter
		else if(memcmp(instruction_buffer, shutter_in_instruction, 4) == 0){
			set_shutter_state(2);
		}
		// roll out shutter
		else if(memcmp(instruction_buffer, shutter_out_instruction, 4) == 0){
			set_shutter_state(3);
		}
		// change minimum light boundary
		else if(memcmp(instruction_buffer, set_min_light_instruction, 4) == 0){
			get_arg();		
			set_min_light(atoi(arg));
		}
		// change maximum light boundary
		else if(memcmp(instruction_buffer, set_max_light_instruction, 4) == 0){
			get_arg();
			set_max_light(atoi(arg));
		}
		// change minimum temp boundary
		else if(memcmp(instruction_buffer, set_min_temp_instruction, 4) == 0){
			get_arg();
			set_min_temp(atoi(arg));
		}
		// change maximum temp boundary
		else if(memcmp(instruction_buffer, set_max_temp_instruction, 4) == 0){
			get_arg();
			set_max_temp(atoi(arg));
		}
		// change minimum dist boundary
		else if(memcmp(instruction_buffer, set_min_dist_instruction, 4) == 0){
			get_arg();
			set_min_dist(atoi(arg));
		}
		// change maximum dist boundary
		else if(memcmp(instruction_buffer, set_max_dist_instruction, 4) == 0){
			get_arg();
			set_max_dist(atoi(arg));
		}
		// toggle update_override
		else if(memcmp(instruction_buffer, set_update_override_instruction, 4) == 0){
			get_arg();
			set_update_override(atoi(arg));
		}
		
		// reset instruction buffer
		instruction_read_pos = 0;
		memset(instruction_buffer, 0, sizeof instruction_buffer);
	}
	// when reading a instruction and it's not a start or stop character write it to instruction buffer
	else if(is_reading_instruction){
		instruction_buffer[instruction_read_pos] = c;
		instruction_read_pos++;
	}
}
