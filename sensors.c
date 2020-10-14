#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>
#include "sensors.h"
#include "com.h"

#define HIGH 0x1
#define LOW  0x0

#define F_CPU 16000000
#include <util/delay.h>

#define MAX_SONAR_RANGE 5
#define SONAR_TIMEOUT ((F_CPU*MAX_SONAR_RANGE)/343)
#define TEMPERATURE_BUFFER_SIZE 40
#define LIGHT_BUFFER_SIZE 30

#define DATA_PIN 0
#define CLOCK_PIN 1
#define STROBE_PIN 2
#define ECHO_PIN 3
#define TRIG_PIN 2

// used for calculating distance
bool sonar_reading = false;
float over_flow_counter = 0;

//array with light intensity values
float light_buffer[LIGHT_BUFFER_SIZE];
uint16_t light_buffer_pos = 0;

// array with temperature values used for calculating average
float temperature_buffer[TEMPERATURE_BUFFER_SIZE];
uint16_t temperature_buffer_pos = 0;

// store sensor data
float light_intensity_result = 0.0;
float temperature_result = 0.0;
float sonar_result = 0.0;

void init_led(){
	reset();
	DDRB = 0xFF;
	sendCommand(0x89);
}

void init_sonar(){
	// set trig and echo pin
	DDRD |= (1<< TRIG_PIN);
	DDRD &= ~(1<< ECHO_PIN);
}

void init_adc()
{
	DDRC = 0x00;
	// select channel 0 (PC0 = input)
	ADMUX = (1<<REFS0);
	// enable the ADC & prescale = 128
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}

void reset()
{
	// clear memory - all 16 addresses
	sendCommand(0x40); // set auto increment mode
	write(STROBE_PIN, LOW);
	shiftOut(0xc0);   // set starting address to 0
	for(uint8_t i = 0; i < 16; i++)
	{
		shiftOut(0x00);
	}
	write(STROBE_PIN, HIGH);
}

void write_to_display(uint64_t input){
	reset();
	uint8_t addresses[] = { 0xce, 0xcc, 0xca, 0xc8, 0xc6, 0xc4, 0xc2, 0xc0 };
	uint8_t digits[] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f };
	uint8_t addr = 0;
	uint8_t temp = 0;
	
	sendCommand(0x44);
	shiftOut(0xc0);
	
	uint64_t number = input;
	if(number == 0)
	{
		write(STROBE_PIN, LOW);
		shiftOut(addresses[addr]);
		shiftOut(digits[0]);
		write(STROBE_PIN, HIGH);
	}
	
	while(number != 0)
	{
		write(STROBE_PIN, LOW);
		temp = number % 10;
		number /= 10;
		shiftOut(addresses[addr]);
		shiftOut(digits[temp]);
		addr += 1;
		write(STROBE_PIN, HIGH);
	}
	
}

// write value to pin
void write(uint8_t pin, uint8_t val)
{
	if (val == LOW) {
		PORTB &= ~(_BV(pin)); // clear bit
		} else {
		PORTB |= _BV(pin); // set bit
	}
}

// shift out value to data
void shiftOut (uint8_t val)
{
	uint8_t i;
	for(i = 0; i < 8; i++)  {
		write(CLOCK_PIN, LOW);   // bit valid on rising edge
		write(DATA_PIN, val & 1 ? HIGH : LOW); // lsb first
		val = val >> 1;
		write(CLOCK_PIN, HIGH);
	}
}

void sendCommand(uint8_t value)
{
	write(STROBE_PIN, LOW);
	shiftOut(value);
	write(STROBE_PIN, HIGH);
}

// Source: https://veerobot.com/learn/ultrasonic-sensor-with-draco/
// Their pseudo-code was used as a reference to make this algorithm.
void read_sonar(){
	// make sure we don't start reading again until reading is finished
	if(!sonar_reading){
		sonar_reading = true;
	
		// send a 10us high pulse
		float trig_counter = 0;
		PORTD &= ~(1<<TRIG_PIN);
		_delay_us(2);
		PORTD |=(1<<TRIG_PIN);
		_delay_us(10);
		PORTD &= ~(1<<TRIG_PIN);
		_delay_us(2);         
		
		float result;         
		// while pulse has not come back, make sure we don't wait to long
		while(!(PIND & (1<<ECHO_PIN))){
			trig_counter++;
			if(trig_counter > SONAR_TIMEOUT){
				result = -1;
				break;
			}
		}
	
		// start 16 bit timer with no prescaler and enable overflow interrupts
		TCNT1 = 0;
		TCCR1B |= (1<<CS10);
		TIMSK1 |= (1<<TOIE1);
		over_flow_counter=0;
	
		// while echo pin is high, make sure the distance isnt unrealistic
		while((PIND & (1<<ECHO_PIN))){
			if(((over_flow_counter*65535)+TCNT1) > SONAR_TIMEOUT){
				result = -1;
				break;
			}
		};
	
		// stop timer
		TCCR1B = 0x00;
		// calculate distance
		result = (((over_flow_counter*65535)+TCNT1)/(58*16)); 
		// reset counter count
		TCNT1 = 0; 
		
		// set sonar_result if result is a valid distance
		if(result != -1 && result <= MAX_SONAR_RANGE * 100){
			sonar_result = result;
		}
		
		// make sure the sonar can be read again
		sonar_reading = false;
	}
}

void read_light_intensity(){
	// read the light intensity from ADC
	ADCSRA |= (1<<ADSC); 
	
	float volt = ADC;
	
	light_buffer[light_buffer_pos] = volt;
	light_buffer_pos++;
	
	// go back to start of the array if array is full
	if(light_buffer_pos >= LIGHT_BUFFER_SIZE){
		light_buffer_pos = 0;
	}
	// calculate average
	float sum = 0.0;
	for(int i = 0; i < LIGHT_BUFFER_SIZE; i++){
		sum += light_buffer[i];
	}
	
	light_intensity_result = sum / LIGHT_BUFFER_SIZE;
}

void read_temperature(){
	// get temperature in degrees c
	ADCSRA |= (1<<ADSC); 
	float temp = ADC;
	temp = ((temp / 1024.0) * 5.0 - 0.5) * 100.0;
	
	// put temperature in array
	temperature_buffer[temperature_buffer_pos] = temp;
	temperature_buffer_pos++;
	
	// go back to start of the array if array is full
	if(temperature_buffer_pos >= TEMPERATURE_BUFFER_SIZE){
		temperature_buffer_pos = 0;
	}
	
	// calculate average
	float sum = 0.0;
	for(int i = 0; i < TEMPERATURE_BUFFER_SIZE; i++){
		sum += temperature_buffer[i];
	}
	temperature_result = sum / TEMPERATURE_BUFFER_SIZE;
}

// getters for sensor data
float get_light_intensity(){
	return light_intensity_result;
}

float get_temperature(){
	return temperature_result;
}

float get_sonar(){
	return sonar_result;
}

// Timer1 overflow interrupt
ISR(TIMER1_OVF_vect){
	over_flow_counter++;
	TCNT1=0;
}
