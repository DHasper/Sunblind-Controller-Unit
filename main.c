#include <avr/io.h>
#include <avr/interrupt.h>
#include "AVR_TTC_scheduler.h"
#include "sensors.h"
#include "com.h"
#include "shutter.h"

void setup(){
	// do all inits and enable global interrupts
	sei();
	init_led();
	init_sonar();
	init_com();
	init_adc();
	init_shutter();
	SCH_Init_T1();
}

int main(void)
{
	setup();
	
	// add tasks to scheduler
    SCH_Add_Task(read_sonar, 0, 1000);
	SCH_Add_Task(send_interface_vals, 0, 1000);
	SCH_Add_Task(move_shutter, 0, 100);
	SCH_Add_Task(update_shutter, 0, 100);
	
	// read temperature or light depending on which type of device is configured
	switch(DEVICE_TYPE){
		case 0:
			SCH_Add_Task(read_temperature, 0, 1000);
			break;
		case 1:
			SCH_Add_Task(read_light_intensity, 0, 1000);
			break;
	}
	
	// do scheduler tasks
    while (1) {
	    SCH_Dispatch_Tasks();
    }

    return 0;
}

