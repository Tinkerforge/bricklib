/* bricklib
 * Copyright (C) 2013 Olaf Lüke <olaf@tinkerforge.com>
 *
 * pid.c: PID controlling functions
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

// This PID controller is based on the Arduino PID library by
// Brett Beauregard. See here for details:
// http://brettbeauregard.com/blog/2011/04/improving-the-beginners-pid-introduction/

#include "pid.h"

#include <stdint.h>
#include <stdbool.h>

void pid_init(PID *pid, float k_p, float k_i, float k_d, uint8_t sample_time, int32_t out_max, int32_t out_min) {
	pid->out_max = out_max;
	pid->out_min = out_min;
	pid->sample_time = sample_time;
	pid->sample_time_counter = 0;

	pid_set_tuning(pid, k_p, k_i, k_d);

	pid->i_term = 0.0;
	pid->last_input = 0.0;
	// TODO: i_term = output, last_input = input;
}

void pid_set_tuning(PID *pid, float k_p, float k_i, float k_d) {
	pid->k_p_orig = k_p;
	pid->k_i_orig = k_i;
	pid->k_d_orig = k_d;

	pid->k_p = k_p;
	pid->k_i = k_i * (((float)pid->sample_time)/1000.0);
	pid->k_d = k_d / (((float)pid->sample_time)/1000.0);

//	if(controllerDirection ==REVERSE) {
//		kp = (0 - kp);
//		ki = (0 - ki);
//		kd = (0 - kd);
//	}
}

bool pid_compute(PID *pid, float setpoint, float input, float *output) {
	pid->sample_time_counter++;
//	if(pid->sample_time_counter >= pid->sample_time) {
		pid->sample_time_counter = 0;

		float error = setpoint - input;
		pid->i_term += pid->k_i * error;
		if(pid->i_term > pid->out_max) {
			pid->i_term = pid->out_max;
		} else if(pid->i_term < pid->out_min) {
			pid->i_term = pid->out_min;
		}

		float d_input = (input - pid->last_input);

		*output = pid->k_p * error + pid->i_term - pid->k_d * d_input;

		if(*output > pid->out_max) {
			*output = pid->out_max;
		} else if(*output < pid->out_min) {
			*output = pid->out_min;
		}


		pid->last_input = input;

		return true;
//	}

	return false;
}

void pid_print(PID *pid) {
	logpidd("i_term=%d, last_input=%d, out_max=%d, out_min=%d\n\r",
			(int32_t)pid->i_term, (int32_t)pid->last_input, pid->out_max, pid->out_min);
}
