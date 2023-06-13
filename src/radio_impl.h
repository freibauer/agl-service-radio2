/*
 * Copyright (C) 2017 Konsulko Group
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

#ifndef _RADIO_IMPL_H
#define _RADIO_IMPL_H

#include <stdint.h>

typedef enum {
	RADIO_BAND_UNSPECIFIED = 0,
	RADIO_BAND_AM,
	RADIO_BAND_FM
} radio_band_t;

typedef enum {
	RADIO_SCAN_UNSPECIFIED = 0,
	RADIO_SCAN_FORWARD,
	RADIO_SCAN_BACKWARD
} radio_scan_direction_t;

typedef void (*radio_scan_callback_t)(uint32_t frequency, void *data);

typedef void (*radio_freq_callback_t)(uint32_t frequency, void *data);

typedef void (*radio_rds_callback_t)(void *rds_data, void *dat);

typedef enum {
	RADIO_MODE_UNSPECIFIED = 0,
	RADIO_MODE_MONO,
	RADIO_MODE_STEREO
} radio_stereo_mode_t;

/*
 * AF_update
 * true if quality belongs to an alternative frequency
 *
 * time_stamp
 * if time_stamp is zero, quality data won't be valid
 * reliability depending on time stamp
 * it takes some time after tuning to get valid quality data
 *
 * rssi
 * (signed)
 * level detector result(RF input level)
 *
 * usn
 * FM ultrasonic noise detector (relative usn detector result)
 *
 * bandwidth
 * IF bandwidth
 */
typedef struct
{
	bool     af_update;
	uint16_t time_stamp;
	int16_t  rssi;
	uint16_t usn;
	uint16_t bandw;
} station_quality_t;

typedef struct {
	char *name;

	int (*probe)(void);

	/* NOTE: init should return -1 if called before probe has been called and returned success */
	int (*init)(void);

	void (*set_output)(const char *output);

	uint32_t (*get_frequency)(void);

	void (*set_frequency)(uint32_t frequency);

	void (*set_frequency_callback)(radio_freq_callback_t callback,
				       void *data);

	void (*set_rds_callback)(radio_rds_callback_t callback);

	radio_band_t (*get_band)(void);

	void (*set_band)(radio_band_t band);

	int (*band_supported)(radio_band_t band);

	uint32_t (*get_min_frequency)(radio_band_t band);

	uint32_t (*get_max_frequency)(radio_band_t band);

	uint32_t (*get_frequency_step)(radio_band_t band);

	bool (*get_corking_state)(void);

	void (*start)(void);

	void (*stop)(void);

	void (*scan_start)(radio_scan_direction_t direction,
			   radio_scan_callback_t callback,
			   void *data);

	void (*scan_stop)(void);

	radio_stereo_mode_t (*get_stereo_mode)(void);

	void (*set_stereo_mode)(radio_stereo_mode_t mode);

	char * (*get_rds_info)(void);

	station_quality_t * (*get_quality_info)(void);

	void (*set_alternative_frequency)(uint32_t frequency);
} radio_impl_ops_t;

#endif /* _RADIO_IMPL_H */
