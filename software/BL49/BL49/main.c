/*
 * BL49.c
 *
 * Created: 22.05.2022 20:35:45
 * Author : Heinrich
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

#include "helpers.h"
#include "adc/adc.h"
#include "spi/spi.h"
#include "dac/dac.h"
#include "cj125/cj125.h"
#include "timer/timer.h"
#include "sensor/sensor.h"
#include "board/board.h"
#include "can/can_network.h"

extern tSensor sensor1;
extern tBoard board;
extern volatile uint16_t ms_counter;

int main(void)
{
	uint8_t signature;
	uint16_t dac_value = 0;
	uint16_t tmp_voltage = 1500;
	uint8_t loopCounter = 0;

	adc_init();
	spi_init();
	// heater_init();
	can_network_init(1);

	board_init(&board);	
	sensor_init(&sensor1, 8);
	board_read_inputs(&board);
	
	sei();	

	/*
	There are three different types of CAN modules available:
	-> 2.0A - Considers 29 bit ID as an error
	-> 2.0B Passive - Ignores 29 bit ID messages
	-> 2.0B Active - Handles both 11 and 29 bit ID Messages
	*/
	
	/*	
	st_cmd_t message;
	
	message.id.ext = 0x180;
	// message.ctrl.ide = 0;			// standard CAN frame type (2.0A)
	
	message.ctrl.ide = 1;				// we are using extended ID, check can_lib.c:118
	message.ctrl.rtr = 0;				// this message object is not requesting a remote node to transmit data back
	message.dlc = 1;
	message.cmd = CMD_TX_DATA;
	// message.pt_data = 0x01;
	
	while(can_cmd(&message) != CAN_CMD_ACCEPTED);					// wait for MOb to configure
	while(can_get_status(&message) == CAN_STATUS_NOT_COMPLETED);	// wait for a transmit request to come in, and send a response
	*/
	
	uint16_t dty = 0;

	
	cj125_set_calibration_mode();
	timer_delay_ms(500);
	
	// Ur value:
	// lower value means hotter sensor...
	// Ur_ref is something round about 1v (1015)
	// Ua_ref is something round about 1.5 (1503)
	
	sensor1.Ua_ref = adc2voltage_millis(adc_read_UA());
	sensor1.Ur_ref = adc2voltage_millis(adc_read_UR());
	
	cj125_set_running_mode_v8();
	
	init_1ms_timer();
	
    /* Replace with your application code */
    while (1) 
    {		
		if (BIT_CHECK(TIMER_TASKS, BIT_TIMER_10ms))
		{
			BIT_CLEAR(TIMER_TASKS, BIT_TIMER_10ms);
			
			/*
			board_read_inputs(&board);
			// we are in running state, so :
			// check cj125 status, read Ur, adjust PID, read Ua, do calculation stuff....
			if (sensor1.State == SENSOR_RUNNING && board.input1_state == HIGH)
			{
				if (cj125_readStatus() == CJ125_STATUS_OKAY)
				{
					sensor1.Ua = adc2voltage_millis(adc_read_UA());
					calculate_ip(&sensor1);
					calculate_lambda(&sensor1);
					sensor1.Ur = adc2voltage_millis(adc_read_UR());
				}
				else
				{
					sensor1.State == SENSOR_ERROR;
					heater_shutdown();
				}
			}
			*/
			
			// TODO: send can message here
		}
		
		if (BIT_CHECK(TIMER_TASKS, BIT_TIMER_20ms))
		{
			BIT_CLEAR(TIMER_TASKS, BIT_TIMER_20ms);
			// do some 20ms stuff...
		}
		
		if (BIT_CHECK(TIMER_TASKS, BIT_TIMER_50ms))
		{
			BIT_CLEAR(TIMER_TASKS, BIT_TIMER_50ms);
			// do some 50ms stuff...
		}
		
		if (BIT_CHECK(TIMER_TASKS, BIT_TIMER_100ms))
		{
			BIT_CLEAR(TIMER_TASKS, BIT_TIMER_100ms);
			board_read_inputs(&board);
			
			sensor_update_ua (&sensor1, 2100);
			
			sensor1.Status = RUN;
			
			
			
			// do some 100ms stuff...
			PORTB ^= (1 << PINB5);
		}
		
		if (BIT_CHECK(TIMER_TASKS, BIT_TIMER_250ms))
		{
			BIT_CLEAR(TIMER_TASKS, BIT_TIMER_250ms);
			
			sensor1.Ur = adc2voltage_millis(adc_read_UR());		
			
			
			/*
			// do some 250ms stuff...
			board_read_inputs(&board);
			
			if (board.input1_state == HIGH)
			{
				// means: engine is running
				switch (sensor1.State)
				{
					case SENSOR_OFF:
						// means engine is started right now so we need to init the sensor and stuff...
						cj125_set_calibration_mode();
					
						if (cj125_readStatus() == CJ125_STATUS_OKAY)
						{
							// everything is fine, we are in calibration mode...
							sensor1.Ua_ref = adc2voltage_millis(adc_read_UA());
							sensor1.Ur_ref = adc2voltage_millis(adc_read_UR());
							
							cj125_set_running_mode_v8();
							
							if (cj125_readStatus() == CJ125_STATUS_OKAY)
							{
								// stuff stored, go for condensation...1.5v for 5 seconds
								// 5 s = 5000 ms = 20
								loopCounter = 19;
								sensor1.State = SENSOR_CONDENSATION;
							}
						}
						else
						{
							sensor1.State == SENSOR_ERROR;
							heater_shutdown();
						}
					break;
					
					case SENSOR_CONDENSATION:
						dty = target_voltage_duty_cycle(1500, board.vBatt);
						heater_setDuty(dty);
						loopCounter--;
						
						if (loopCounter == 0)
						{
							// condensation done, next step heating up the sensor
							sensor1.State = SENSOR_HEATING_UP;
							loopCounter = 0;
						}
						
					case SENSOR_HEATING_UP:
						// we heat up the sensor, start with 8.5v and goes up to 13v in 0.4v/s steps
						// start with 8.5v
						
						// first loop:
						// set voltage to 8.5v
						if (loopCounter == 0)
						{
							tmp_voltage = 8500;
						}
						
						// can we modulo 4? if so, a second is over and we hve to increase the voltage...
						if (loopCounter % 4 == 0 && loopCounter != 0)
						{
							tmp_voltage += 400;
							
							if (tmp_voltage > 13000)
							{
								tmp_voltage = 13000;
							}
						}
						
						// adjust heater voltage to supply voltage
						dty = target_voltage_duty_cycle(tmp_voltage, board.vBatt);
						heater_setDuty(dty);
						
						if (loopCounter == 48)
						{
							// heating up is done, switch to PID controller and go over to running...
							sensor1.State = SENSOR_RUNNING;
						}
						
						loopCounter++;
					
					break;
				}
				
				
				
			}
			*/

			PORTB ^= (1 << PINB6);
		}

    }
}