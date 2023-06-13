/*
 * Copyright (C) 2017,2018,2020 Konsulko Group
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * NOTE: Some locking around frequency and scanning state may be
 *       required if this null implementation ever needs to be used
 *       beyond basic functionality testing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <errno.h>
#include <glib.h>

#include "radio_impl.h"

// Structure to describe FM band plans, all values in Hz.
typedef struct {
	char *name;
	uint32_t min;
	uint32_t max;
	uint32_t step;
} fm_band_plan_t;

static fm_band_plan_t known_fm_band_plans[5] = {
	{ .name = "US", .min = 87900000, .max = 107900000, .step = 200000 },
	{ .name = "JP", .min = 76000000, .max = 95000000, .step = 100000 },
	{ .name = "EU", .min = 87500000, .max = 108000000, .step = 50000 },
	{ .name = "ITU-1", .min = 87500000, .max = 108000000, .step = 50000 },
	{ .name = "ITU-2", .min = 87900000, .max = 107900000, .step = 50000 }
};

static unsigned int bandplan;
static bool present;
static bool initialized;
static bool active;
static bool scanning;
static uint32_t current_frequency;
static radio_freq_callback_t freq_callback;
static void *freq_callback_data;

static uint32_t null_get_min_frequency(radio_band_t band);
static void null_set_frequency(uint32_t frequency);

static int null_probe(void)
{
	present = true;
	return 0;
}

static int null_init(void)
{
	GKeyFile *conf_file;
	char *value_str;

	if(!present)
		return -1;

	if(initialized)
		return 0;

	// Load settings from configuration file if it exists
	conf_file = g_key_file_new();
	if(conf_file &&
	   g_key_file_load_from_dirs(conf_file,
				     "AGL.conf",
				     (const gchar**) g_get_system_config_dirs(),
				     NULL,
				     G_KEY_FILE_KEEP_COMMENTS,
				     NULL) == TRUE) {

		// Set band plan if it is specified
		value_str = g_key_file_get_string(conf_file,
						  "radio",
						  "fmbandplan",
						  NULL);
		if(value_str) {
			unsigned int i;
			for(i = 0;
			    i < sizeof(known_fm_band_plans) / sizeof(fm_band_plan_t);
			    i++) {
				if(!strcasecmp(value_str, known_fm_band_plans[i].name)) {
					bandplan = i;
					break;
				}
			}
		}
	}

	// Start off with minimum bandplan frequency
	current_frequency = null_get_min_frequency(RADIO_BAND_FM);

	initialized = true;
	null_set_frequency(current_frequency);

	return 0;
}

static void null_set_output(const char *output)
{
}

static uint32_t null_get_frequency(void)
{
	return current_frequency;
}

static void null_set_frequency(uint32_t frequency)
{
	if(frequency < known_fm_band_plans[bandplan].min ||
	   frequency > known_fm_band_plans[bandplan].max)
		return;

	current_frequency = frequency;

	if(freq_callback)
		freq_callback(current_frequency, freq_callback_data);
}

static void null_set_frequency_callback(radio_freq_callback_t callback,
					void *data)
{
	freq_callback = callback;
	freq_callback_data = data;
}

static radio_band_t null_get_band(void)
{
	// We only support FM
	return RADIO_BAND_FM;
}

static void null_set_band(radio_band_t band)
{
	// We only support FM, so do nothing
}

static int null_band_supported(radio_band_t band)
{
	if(band == RADIO_BAND_FM)
		return 1;
	return 0;
}

static uint32_t null_get_min_frequency(radio_band_t band)
{
	return known_fm_band_plans[bandplan].min;
}

static uint32_t null_get_max_frequency(radio_band_t band)
{
	return known_fm_band_plans[bandplan].max;
}

static uint32_t null_get_frequency_step(radio_band_t band)
{
	uint32_t ret = 0;

	switch (band) {
	case RADIO_BAND_AM:
		ret = 1000; // 1 kHz
		break;
	case RADIO_BAND_FM:
		ret = known_fm_band_plans[bandplan].step;
		break;
	default:
		break;
	}
	return ret;
}

static void null_start(void)
{
	if(!initialized)
		return;

	if(active)
		return;

	active = true;
}

static void null_stop(void)
{
	if(!initialized)
		return;

	if (!active)
		return;

	active = false;
}

static void null_scan_start(radio_scan_direction_t direction,
			    radio_scan_callback_t callback,
			    void *data)
{
	int frequency;

	if(!active || scanning)
		return;

	scanning = true;

	// Just go to the next frequency step up or down
	frequency = current_frequency;
	if(direction == RADIO_SCAN_FORWARD) {
		frequency += known_fm_band_plans[bandplan].step;
	} else {
		frequency -= known_fm_band_plans[bandplan].step;
	}
	if(frequency < known_fm_band_plans[bandplan].min)
		frequency = known_fm_band_plans[bandplan].max;
	else if(frequency > known_fm_band_plans[bandplan].max)
		frequency = known_fm_band_plans[bandplan].min;

	scanning = false;
	null_set_frequency(frequency);
	if(callback)
		callback(frequency, data);
}

static void null_scan_stop(void)
{
	scanning = false;
}

static radio_stereo_mode_t null_get_stereo_mode(void)
{
	// We only support stereo
	return RADIO_MODE_STEREO;
}

static void null_set_stereo_mode(radio_stereo_mode_t mode)
{
	// We only support stereo, so do nothing
}

radio_impl_ops_t null_impl_ops = {
	.name = "null/mock radio",
	.probe = null_probe,
	.init = null_init,
	.set_output = null_set_output,
	.get_frequency = null_get_frequency,
	.set_frequency = null_set_frequency,
	.set_frequency_callback = null_set_frequency_callback,
	.get_band = null_get_band,
	.set_band = null_set_band,
	.band_supported = null_band_supported,
	.get_min_frequency = null_get_min_frequency,
	.get_max_frequency = null_get_max_frequency,
	.get_frequency_step = null_get_frequency_step,
	.start = null_start,
	.stop = null_stop,
	.scan_start = null_scan_start,
	.scan_stop = null_scan_stop,
	.get_stereo_mode = null_get_stereo_mode,
	.set_stereo_mode = null_set_stereo_mode
};
