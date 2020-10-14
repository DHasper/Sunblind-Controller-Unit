#ifndef SENSORS_H_
#define SENSORS_H_

void reset(void);
void write_to_display(uint64_t input);
void write(uint8_t pin, uint8_t val);
void shiftOut (uint8_t val);
void sendCommand(uint8_t value);
void init_led();
void init_sonar();
void init_adc();
void read_sonar();
void read_temperature();
void read_light_intensity();
float get_light_intensity();
float get_temperature();
float get_sonar();

#endif