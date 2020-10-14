#include <avr/io.h>
#include <stdbool.h>
#include <avr/eeprom.h>
#include "shutter.h"
#include "sensors.h"

#define F_CPU 16000000
#include <util/delay.h>

#define GREEN_LED	PORTD6
#define YELLOW_LED  PORTD5
#define RED_LED		PORTD4

#define MIN_TEMP_ADDRESS	0
#define MAX_TEMP_ADDRESS	2
#define MIN_LIGHT_ADDRESS	4
#define MAX_LIGHT_ADDRESS	6
#define MIN_DIST_ADDRESS	8
#define MAX_DIST_ADDRESS	10

// 0 = in 1 = out 2 = moving in 3 = moving out
int shutter_state = 0;

// if true the boundary values won't get used and the shutter can be controlled by external software
bool update_override = false;

// stores boundary values
uint16_t max_temp = 0;
uint16_t min_temp = 0;
uint16_t max_light = 0;
uint16_t min_light = 0;
uint16_t max_dist = 0;
uint16_t min_dist = 0;

void init_shutter(){
	// D4 to output for LED
	DDRD |= (1 << RED_LED);
	DDRD |= (1 << YELLOW_LED);
	DDRD |= (1 << GREEN_LED);
	
	// read user defined values from eeprom
	max_temp = eeprom_read_word((uint16_t*) MAX_TEMP_ADDRESS);
	min_temp = eeprom_read_word((uint16_t*) MIN_TEMP_ADDRESS);
	max_light = eeprom_read_word((uint16_t*) MAX_LIGHT_ADDRESS);
	min_light = eeprom_read_word((uint16_t*) MIN_LIGHT_ADDRESS);
	max_dist = eeprom_read_word((uint16_t*) MAX_DIST_ADDRESS);
	min_dist = eeprom_read_word((uint16_t*) MIN_DIST_ADDRESS);
}

void update_shutter(){
	write_to_display(max_light);
	if(!update_override){
		switch(DEVICE_TYPE){
			case 0:
			// if temp exceeds boundary values move shutter in
			if((get_temperature() <= min_temp || get_temperature() >= max_temp) && shutter_state == 1){
				shutter_state = 2;
			}
			// if its safe move shutter out
			else if (get_temperature() >= min_temp && get_temperature() <= max_temp && shutter_state == 0){
				shutter_state = 3;
			}
			break;
			
			case 1:
			// if light exceeds boundary values move shutter in
			if((get_light_intensity() <= min_light || get_light_intensity() >= max_light) && shutter_state == 1){
				shutter_state = 2;
			}
			// if its safe move shutter out
			else if(get_light_intensity() >= min_light && get_light_intensity() <= max_light && shutter_state == 0){
				shutter_state = 3;
			}
			break;
		}
	}
}

void move_shutter(){
	// control the shutter
	switch(shutter_state){
		case 0:
			PORTD = (1 << RED_LED);
			break;
		case 1:
			PORTD = (1 << GREEN_LED);
			break;
		case 2:
			move_shutter_in();
			break;
		case 3:
			move_shutter_out();
			break;
	}
}

void move_shutter_in(){	
	// if shutter is not fully moved in move in further
	if(get_sonar() >= min_dist){
		PORTD = (1 << YELLOW_LED);
		_delay_ms(50);
		PORTD &= ~(1 << YELLOW_LED);
		_delay_ms(50);
	}
	// done moving in, set state accordingly
	else {
		shutter_state = 0;
	}
}

void move_shutter_out(){
	// if shutter is not fully moved out move out further
	if(get_sonar() <= max_dist){
		PORTD = (1 << YELLOW_LED);
		_delay_ms(50);
		PORTD &= ~(1 << YELLOW_LED);
		_delay_ms(50);
	}
	// done moving out, set state accordingly
	else {
		shutter_state = 1;
	}
}

// setters for boundary values
void set_shutter_state(int state){
	shutter_state = state;
}

void set_min_light(uint16_t val){
	min_light = val;
	eeprom_write_word((uint16_t*) MIN_LIGHT_ADDRESS, val);
}

void set_max_light(uint16_t val){
	max_light = val;
	eeprom_write_word((uint16_t*) MAX_LIGHT_ADDRESS, val);
}

void set_min_temp(uint16_t val){
	min_temp = val;
	eeprom_write_word((uint16_t*) MIN_TEMP_ADDRESS, val);
}

void set_max_temp(uint16_t val){
	max_temp = val;
	eeprom_write_word((uint16_t*) MAX_TEMP_ADDRESS, val);
}

void set_min_dist(uint16_t val){
	min_dist = val;
	eeprom_write_word((uint16_t*) MIN_DIST_ADDRESS, val);
}

void set_max_dist(uint16_t val){
	max_dist = val;
	eeprom_write_word((uint16_t*) MAX_DIST_ADDRESS, val);
}

void set_update_override(int var){
	update_override = var;
}

// getters for boundary values
uint16_t get_max_temp(){
	return max_temp;
}

uint16_t get_min_temp(){
	return min_temp;
}

uint16_t get_max_light(){
	return max_light;
}

uint16_t get_min_light(){
	return min_light;
}

uint16_t get_max_dist(){
	return max_dist;
}

uint16_t get_min_dist(){
	return min_dist;
}

int get_update_override(){
	return update_override;
}

int get_shutter_state(){
	return shutter_state;
}
