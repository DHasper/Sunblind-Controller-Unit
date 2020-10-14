#ifndef SHUTTER_H_
#define SHUTTER_H_

// type 0 has temperature sensor, type 1 has light sensor
#define DEVICE_TYPE 1
#define DEVICE_NAME "Light Unit 1"

void init_shutter();
void update_shutter();
void move_shutter();
void move_shutter_in();
void move_shutter_out();
void set_shutter_state(int state);

// boundary setters
void set_min_light(uint16_t val);
void set_max_light(uint16_t val);
void set_min_temp(uint16_t val);
void set_max_temp(uint16_t val);
void set_min_dist(uint16_t val);
void set_max_dist(uint16_t val);
void set_update_override(int var);

// boundary getters
uint16_t get_max_temp();
uint16_t get_min_temp();
uint16_t get_max_light();
uint16_t get_min_light();
uint16_t get_max_dist();
uint16_t get_min_dist();
int get_update_override();
int get_shutter_state();

void toggle_update_override();

#endif