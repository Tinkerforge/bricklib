/* bricklib
 * Copyright (C) 2013 Olaf Lüke <olaf@tinkerforge.com>
 *
 * pid.h: PID controlling functions
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


#ifndef PID_H
#define PID_H

#include <stdint.h>
#include <stdbool.h>
#include "bricklib/logging/logging.h"

#define DEBUG_PID 1

// ************** PID DEBUGGING **************
#if(DEBUG_PID)
#define logpidd(str, ...) do{logd("pid: " str, ##__VA_ARGS__);}while(0)
#define logpidi(str, ...) do{logi("pid: " str, ##__VA_ARGS__);}while(0)
#define logpidw(str, ...) do{logw("pid: " str, ##__VA_ARGS__);}while(0)
#define logpide(str, ...) do{loge("pid: " str, ##__VA_ARGS__);}while(0)
#define logpidf(str, ...) do{logf("pid: " str, ##__VA_ARGS__);}while(0)
#else
#define logpidd(str, ...) {}
#define logpidi(str, ...) {}
#define logpidw(str, ...) {}
#define logpide(str, ...) {}
#define logpidf(str, ...) {}
#endif

typedef struct {
	float k_p;
	float k_i;
	float k_d;
	float k_p_orig;
	float k_i_orig;
	float k_d_orig;
	float i_term;
	float last_input;
	int32_t out_max;
	int32_t out_min;
	uint8_t sample_time;
	uint8_t sample_time_counter;
} PID;

void pid_init(PID *pid, float k_p, float k_i, float k_d, uint8_t sample_time, int32_t out_max, int32_t out_min);
void pid_set_tuning(PID *pid, float k_p, float k_i, float k_d);
bool pid_compute(PID *pid, float setpoint, float input, float *output);
void pid_print(PID *pid);

#endif
