/*
 * Copyright (C) 2017, 2019 Konsulko Group
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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <json-c/json.h>
#include <afb/afb-binding.h>

#include "radio_impl.h"
#include "radio_impl_null.h"
#include "radio_impl_rtlsdr.h"
#include "radio_impl_kingfisher.h"
#include "radio_impl_tef665x.h"

static radio_impl_ops_t *radio_impl_ops;

static afb_event_t freq_event;
static afb_event_t scan_event;
static afb_event_t status_event;
static afb_event_t rds_event;

static bool playing;

static const char *signalcomposer_events[] = {
	"event.media.next",
	"event.media.previous",
	"event.media.mode",
	NULL,
};

static void freq_callback(uint32_t frequency, void *data)
{
	json_object *jresp = json_object_new_object();
	json_object *value = json_object_new_int((int) frequency);

	json_object_object_add(jresp, "value", value);
	afb_event_push(freq_event, json_object_get(jresp));
}

static void scan_callback(uint32_t frequency, void *data)
{
	json_object *jresp = json_object_new_object();
	json_object *value = json_object_new_int((int) frequency);

	json_object_object_add(jresp, "value", value);
	afb_event_push(scan_event, json_object_get(jresp));
}

static void rds_callback(void *rds_data)
{
	//rds_data is a json object
	afb_event_push(rds_event, json_object_get(rds_data));
}

/*
 * Binding verb handlers
 */

/*
 * @brief Get (and optionally set) frequency
 *
 * @param afb_req_t : an afb request structure
 *
 */
static void frequency(afb_req_t request)
{
	json_object *ret_json;
	const char *value = afb_req_value(request, "value");
	uint32_t frequency;

	if(value) {
		char *p;
		radio_band_t band;
		uint32_t min_frequency;
		uint32_t max_frequency;
		uint32_t step;

		frequency = (uint32_t) strtoul(value, &p, 10);
		if(frequency && *p == '\0') {
			band = radio_impl_ops->get_band();
			min_frequency = radio_impl_ops->get_min_frequency(band);
			max_frequency = radio_impl_ops->get_max_frequency(band);
			step = radio_impl_ops->get_frequency_step(band);
			if(frequency < min_frequency ||
			   frequency > max_frequency ||
			   (frequency - min_frequency) % step) {
				afb_req_reply(request, NULL, "failed", "Invalid frequency");
				return;
			}
			radio_impl_ops->set_frequency(frequency);
		} else {
			afb_req_reply(request, NULL, "failed", "Invalid frequency");
			return;
		}
	}
	ret_json = json_object_new_object();
	frequency = radio_impl_ops->get_frequency();
	json_object_object_add(ret_json, "frequency", json_object_new_int((int32_t) frequency));
	afb_req_reply(request, ret_json, NULL, NULL);
}

/*
 * @brief Get RDS information
 *
 * @param afb_req_t : an afb request structure
 *
 */
static void rds(afb_req_t request)
{
	json_object *ret_json;
	char * rds;

	if (radio_impl_ops->get_rds_info == NULL) {
		afb_req_reply(request, NULL, "failed", "not supported");
		return;
	}

	ret_json = json_object_new_object();
	rds = radio_impl_ops->get_rds_info();
	json_object_object_add(ret_json, "rds", json_object_new_string(rds?rds:""));
	free(rds);

	afb_req_reply(request, ret_json, NULL, NULL);
}

/* @brief Get quality information
 *
 * @param afb_req_t : an afb request structure
 *
 */
static void quality(afb_req_t request)
{
	json_object *ret_json;
	station_quality_t *quality_data;

	if (radio_impl_ops->get_quality_info == NULL) {
		afb_req_reply(request, NULL, "failed", "Not supported");
		return;
	}

	quality_data = radio_impl_ops->get_quality_info();
	ret_json=json_object_new_object();
	if(quality_data->af_update)
	{
		json_object_object_add(ret_json, "af_update", json_object_new_int((int) quality_data->af_update));
	}
	if(quality_data->time_stamp){
		json_object_object_add(ret_json, "timestamp", json_object_new_int((int) quality_data->time_stamp));
	}
	if(quality_data->rssi)
	{
		json_object_object_add(ret_json, "rssi", json_object_new_int((int) quality_data->rssi));
	}
	if(quality_data->usn)
	{
		json_object_object_add(ret_json, "usn", json_object_new_int((int) quality_data->usn));
	}
	if(quality_data->bandw)
	{
		json_object_object_add(ret_json, "bandwidth", json_object_new_int((int) quality_data->bandw));
	}
	afb_req_reply(request, ret_json, NULL, NULL);
	return;
}

/* @brief Check alternative frequency
 *
 * @param afb_req_t : an afb request structure
 *
 */
static void alternative_frequency(afb_req_t request)
{
	json_object *ret_json;
	uint32_t alternative;
	const char *value;

	if (radio_impl_ops->set_alternative_frequency == NULL) {
		afb_req_reply(request, NULL, "failed", "Not supported");
		return;
	}

	value = afb_req_value(request, "value");
	if(value) {
		char *p;
		radio_band_t band;
		uint32_t min_frequency;
		uint32_t max_frequency;
		uint32_t step;

		alternative = (uint32_t) strtoul(value, &p, 10);
		if(alternative && *p == '\0') {
			band = radio_impl_ops->get_band();
			min_frequency = radio_impl_ops->get_min_frequency(band);
			max_frequency = radio_impl_ops->get_max_frequency(band);
			step = radio_impl_ops->get_frequency_step(band);
			if(alternative < min_frequency ||
			   alternative > max_frequency ||
			   (alternative - min_frequency) % step) {
				afb_req_reply(request, NULL, "failed", "Invalid alternative frequency");
				return;
			}
			radio_impl_ops->set_alternative_frequency(alternative);
			ret_json = json_object_new_object();
			json_object_object_add(ret_json, "alternative", json_object_new_int((int32_t) alternative));
			afb_req_reply(request, ret_json, NULL, NULL);
		} else {
			afb_req_reply(request, NULL, "failed", "Invalid alternative frequency");
			return;
		}
	}
	else {
		afb_req_reply(request, NULL, "failed", "Invalid alternative frequency");
		return;
	}
}

/*
 * @brief Get (and optionally set) frequency band
 *
 * @param afb_req_t : an afb request structure
 *
 */
static void band(afb_req_t request)
{
	json_object *ret_json;
	const char *value = afb_req_value(request, "value");
	int valid = 0;
	radio_band_t band;
	char band_name[4];

	if(value) {
		if(!strcasecmp(value, "AM")) {
			band = BAND_AM;
			valid = 1;
		} else if(!strcasecmp(value, "FM")) {
			band = BAND_FM;
			valid = 1;
		} else {
			char *p;
			band = strtoul(value, &p, 10);
			if(p != value && *p == '\0') {
				switch(band) {
				case BAND_AM:
				case BAND_FM:
					valid = 1;
					break;
				default:
					break;
				}
			}
		}
		if(valid) {
			radio_impl_ops->set_band(band);
		} else {
			afb_req_reply(request, NULL, "failed", "Invalid band");
			return;
		}
	}
	ret_json = json_object_new_object();
	band = radio_impl_ops->get_band();
	sprintf(band_name, "%s", band == BAND_AM ? "AM" : "FM");
	json_object_object_add(ret_json, "band", json_object_new_string(band_name));
	afb_req_reply(request, ret_json, NULL, NULL);
}

/*
 * @brief Check if band is supported
 *
 * @param afb_req_t : an afb request structure
 *
 */
static void band_supported(afb_req_t request)
{
	json_object *ret_json;
	const char *value = afb_req_value(request, "band");
	int valid = 0;
	radio_band_t band;

	if(value) {
		if(!strcasecmp(value, "AM")) {
			band = BAND_AM;
			valid = 1;
		} else if(!strcasecmp(value, "FM")) {
			band = BAND_FM;
			valid = 1;
		} else {
			char *p;
			band = strtoul(value, &p, 10);
			if(p != value && *p == '\0') {
				switch(band) {
				case BAND_AM:
				case BAND_FM:
					valid = 1;
					break;
				default:
					break;
				}
			}
		}
	}
	if(!valid) {
		afb_req_reply(request, NULL, "failed", "Invalid band");
		return;
	}
	ret_json = json_object_new_object();
	json_object_object_add(ret_json,
			       "supported",
			       json_object_new_int(radio_impl_ops->band_supported(band)));
	afb_req_reply(request, ret_json, NULL, NULL);
}

/*
 * @brief Get frequency range for a band
 *
 * @param afb_req_t : an afb request structure
 *
 */
static void frequency_range(afb_req_t request)
{
	json_object *ret_json;
	const char *value = afb_req_value(request, "band");
	int valid = 0;
	radio_band_t band;
	uint32_t min_frequency;
	uint32_t max_frequency;

	if(value) {
		if(!strcasecmp(value, "AM")) {
			band = BAND_AM;
			valid = 1;
		} else if(!strcasecmp(value, "FM")) {
			band = BAND_FM;
			valid = 1;
		} else {
			char *p;
			band = strtoul(value, &p, 10);
			if(p != value && *p == '\0') {
				switch(band) {
				case BAND_AM:
				case BAND_FM:
					valid = 1;
					break;
				default:
					break;
				}
			}
		}
	}
	if(!valid) {
		afb_req_reply(request, NULL, "failed", "Invalid band");
		return;
	}
	ret_json = json_object_new_object();
	min_frequency = radio_impl_ops->get_min_frequency(band);
	max_frequency = radio_impl_ops->get_max_frequency(band);
	json_object_object_add(ret_json, "min", json_object_new_int((int32_t) min_frequency));
	json_object_object_add(ret_json, "max", json_object_new_int((int32_t) max_frequency));
	afb_req_reply(request, ret_json, NULL, NULL);
}

/*
 * @brief Get frequency step size (Hz) for a band
 *
 * @param afb_req_t : an afb request structure
 *
 */
static void frequency_step(afb_req_t request)
{
	json_object *ret_json;
	const char *value = afb_req_value(request, "band");
	int valid = 0;
	radio_band_t band;
	uint32_t step;

	if(value) {
		if(!strcasecmp(value, "AM")) {
			band = BAND_AM;
			valid = 1;
		} else if(!strcasecmp(value, "FM")) {
			band = BAND_FM;
			valid = 1;
		} else {
			char *p;
			band = strtoul(value, &p, 10);
			if(p != value && *p == '\0') {
				switch(band) {
				case BAND_AM:
				case BAND_FM:
					valid = 1;
					break;
				default:
					break;
				}
			}
		}
	}
	if(!valid) {
		afb_req_reply(request, NULL, "failed", "Invalid band");
		return;
	}
	ret_json = json_object_new_object();
	step = radio_impl_ops->get_frequency_step(band);
	json_object_object_add(ret_json, "step", json_object_new_int((int32_t) step));
	afb_req_reply(request, ret_json, NULL, NULL);
}

/*
 * @brief Start radio playback
 *
 * @param afb_req_t : an afb request structure
 *
 */
static void start(afb_req_t request)
{
	radio_impl_ops->set_output(NULL);
	radio_impl_ops->start();
	playing = true;
	afb_req_reply(request, NULL, NULL, NULL);

	json_object *jresp = json_object_new_object();
	json_object *value = json_object_new_string("playing");
	json_object_object_add(jresp, "value", value);
	afb_event_push(status_event, json_object_get(jresp));
}

/*
 * @brief Stop radio playback
 *
 * @param afb_req_t : an afb request structure
 *
 */
static void stop(afb_req_t request)
{
	radio_impl_ops->stop();
	playing = false;
	afb_req_reply(request, NULL, NULL, NULL);

	json_object *jresp = json_object_new_object();
	json_object *value = json_object_new_string("stopped");
	json_object_object_add(jresp, "value", value);
	afb_event_push(status_event, json_object_get(jresp));
}

/*
 * @brief Scan for a station in the specified direction
 *
 * @param afb_req_t : an afb request structure
 *
 */
static void scan_start(afb_req_t request)
{
	const char *value = afb_req_value(request, "direction");
	int valid = 0;
	radio_scan_direction_t direction;

	if(value) {
		if(!strcasecmp(value, "forward")) {
			direction = SCAN_FORWARD;
			valid = 1;
		} else if(!strcasecmp(value, "backward")) {
			direction = SCAN_BACKWARD;
			valid = 1;
		} else {
			char *p;
			direction = strtoul(value, &p, 10);
			if(p != value && *p == '\0') {
				switch(direction) {
				case SCAN_FORWARD:
				case SCAN_BACKWARD:
					valid = 1;
					break;
				default:
					break;
				}
			}
		}
	}
	if(!valid) {
		afb_req_reply(request, NULL, "failed", "Invalid direction");
		return;
	}
	radio_impl_ops->scan_start(direction, scan_callback, NULL);
	afb_req_reply(request, NULL, NULL, NULL);
}

/*
 * @brief Stop station scan
 *
 * @param afb_req_t : an afb request structure
 *
 */
static void scan_stop(afb_req_t request)
{
	radio_impl_ops->scan_stop();
	afb_req_reply(request, NULL, NULL, NULL);
}

/*
 * @brief Get (and optionally set) stereo mode
 *
 * @param afb_req_t : an afb request structure
 *
 */
static void stereo_mode(afb_req_t request)
{
	json_object *ret_json;
	const char *value = afb_req_value(request, "value");
	int valid = 0;
	radio_stereo_mode_t mode;

	if(value) {
		if(!strcasecmp(value, "mono")) {
			mode = MONO;
			valid = 1;
		} else if(!strcasecmp(value, "stereo")) {
			mode = STEREO;
			valid = 1;
		} else {
			char *p;
			mode = strtoul(value, &p, 10);
			if(p != value && *p == '\0') {
				switch(mode) {
				case MONO:
				case STEREO:
					valid = 1;
					break;
				default:
					break;
				}
			}
		}
		if(valid) {
			radio_impl_ops->set_stereo_mode(mode);
		} else {
			afb_req_reply(request, NULL, "failed", "Invalid mode");
			return;
		}
	}
	ret_json = json_object_new_object();
	mode = radio_impl_ops->get_stereo_mode();

	json_object_object_add(ret_json, "mode", json_object_new_string(mode == MONO ? "mono" : "stereo"));
	afb_req_reply(request, ret_json, NULL, NULL);
}

/*
 * @brief Subscribe for an event
 *
 * @param afb_req_t : an afb request structure
 *
 */
static void subscribe(afb_req_t request)
{
	const char *value = afb_req_value(request, "value");
	if(value) {
		if(!strcasecmp(value, "frequency")) {
			afb_req_subscribe(request, freq_event);
		} else if(!strcasecmp(value, "station_found")) {
			afb_req_subscribe(request, scan_event);
		} else if(!strcasecmp(value, "status")) {
			afb_req_subscribe(request, status_event);
		} else if(!strcasecmp(value, "rds")) {
			afb_req_subscribe(request, rds_event);
		}
		 else {
			afb_req_reply(request, NULL, "failed", "Invalid event");
			return;
		}
	}
	afb_req_reply(request, NULL, NULL, NULL);
}

/*
 * @brief Unsubscribe for an event
 *
 * @param afb_req_t : an afb request structure
 *
 */
static void unsubscribe(afb_req_t request)
{
	const char *value = afb_req_value(request, "value");
	if(value) {
		if(!strcasecmp(value, "frequency")) {
			afb_req_unsubscribe(request, freq_event);
		} else if(!strcasecmp(value, "station_found")) {
			afb_req_unsubscribe(request, scan_event);
		} else if(!strcasecmp(value, "status")) {
			afb_req_unsubscribe(request, status_event);
			
		} else if(!strcasecmp(value, "rds")) {
			afb_req_unsubscribe(request, rds_event);
			
		} else {
			afb_req_reply(request, NULL, "failed", "Invalid event");
			return;
		}
	}
	afb_req_reply(request, NULL, NULL, NULL);
}

static const afb_verb_t verbs[]= {
	{ .verb = "frequency",		.session = AFB_SESSION_NONE, .callback = frequency,		.info = "Get/Set frequency" },
	{ .verb = "band",		.session = AFB_SESSION_NONE, .callback = band,			.info = "Get/Set band" },
	{ .verb = "rds",		.session = AFB_SESSION_NONE, .callback = rds,			.info = "Get RDS information" },
	{ .verb = "quality",	.session = AFB_SESSION_NONE, .callback = quality,   .info = "Get station quality information" },
	{ .verb = "alternative_frequency",	.session = AFB_SESSION_NONE, .callback = alternative_frequency,   .info = "Check an alternative frequency" },
	{ .verb = "band_supported",	.session = AFB_SESSION_NONE, .callback = band_supported,	.info = "Check band support" },
	{ .verb = "frequency_range",	.session = AFB_SESSION_NONE, .callback = frequency_range,	.info = "Get frequency range" },
	{ .verb = "frequency_step",	.session = AFB_SESSION_NONE, .callback = frequency_step,	.info = "Get frequency step" },
	{ .verb = "start",		.session = AFB_SESSION_NONE, .callback = start,			.info = "Start radio playback" },
	{ .verb = "stop",		.session = AFB_SESSION_NONE, .callback = stop,			.info = "Stop radio playback" },
	{ .verb = "scan_start",		.session = AFB_SESSION_NONE, .callback = scan_start,		.info = "Start station scan" },
	{ .verb = "scan_stop",		.session = AFB_SESSION_NONE, .callback = scan_stop,		.info = "Stop station scan" },
	{ .verb = "stereo_mode",	.session = AFB_SESSION_NONE, .callback = stereo_mode,		.info = "Get/Set stereo_mode" },
	{ .verb = "subscribe",		.session = AFB_SESSION_NONE, .callback = subscribe,		.info = "Subscribe for an event" },
	{ .verb = "unsubscribe",	.session = AFB_SESSION_NONE, .callback = unsubscribe,		.info = "Unsubscribe for an event" },
	{ }
};

static void onevent(afb_api_t api, const char *event, struct json_object *object)
{
	json_object *tmp = NULL;
	const char *uid;
	const char *value;

	json_object_object_get_ex(object, "uid", &tmp);
	if (tmp == NULL)
		return;

	uid = json_object_get_string(tmp);
	if (strncmp(uid, "event.media.", 12))
		return;

	if (!playing ||
	    (radio_impl_ops->get_corking_state &&
	     radio_impl_ops->get_corking_state())) {
		return;
	}

	json_object_object_get_ex(object, "value", &tmp);
	if (tmp == NULL)
		return;

	value = json_object_get_string(tmp);
	if (strncmp(value, "true", 4))
		return;

	if (!strcmp(uid, "event.media.next")) {
		radio_impl_ops->scan_start(SCAN_FORWARD, scan_callback, NULL);
	} else if (!strcmp(uid, "event.media.previous")) {
		radio_impl_ops->scan_start(SCAN_BACKWARD, scan_callback, NULL);
	} else if (!strcmp(uid, "event.media.mode")) {
		// Do nothing ATM
	} else {
		AFB_WARNING("Unhandled signal-composer uid '%s'", uid);
	}
}

static int init(afb_api_t api)
{
	// Probe for radio backends
	radio_impl_ops = &rtlsdr_impl_ops;
	int rc = radio_impl_ops->probe();
	if(rc != 0) {
		// Look for Kingfisher Si4689
		radio_impl_ops = &kf_impl_ops;
		rc = radio_impl_ops->probe();
	}
	if(rc != 0) {
		radio_impl_ops = &tef665x_impl_ops;
		rc = radio_impl_ops->probe();
	}
	if (rc != 0) {
		radio_impl_ops = &null_impl_ops;
		rc = radio_impl_ops->probe();
	}
	if (rc != 0) {
		// We don't expect the null implementation to fail probe, but just in case...
		AFB_API_ERROR(afbBindingV3root, "No radio device found, exiting");
		return rc;
	}
	// Try to initialize detected backend
	rc = radio_impl_ops->init();
	if(rc < 0) {
		AFB_API_ERROR(afbBindingV3root,
			      "%s initialization failed\n",
			      radio_impl_ops->name);
		return rc;
	}
	AFB_API_NOTICE(afbBindingV3root, "%s found\n", radio_impl_ops->name);
	radio_impl_ops->set_frequency_callback(freq_callback, NULL);
	radio_impl_ops->set_frequency_callback(freq_callback, NULL);
	if(radio_impl_ops->set_rds_callback) {
		radio_impl_ops->set_rds_callback(rds_callback);
	}

	rc = afb_daemon_require_api("signal-composer", 1);
	if (rc) {
		AFB_WARNING("unable to initialize signal-composer binding");
	} else {
		const char **tmp = signalcomposer_events;
		json_object *args = json_object_new_object();
		json_object *signals = json_object_new_array();

		while (*tmp) {
			json_object_array_add(signals, json_object_new_string(*tmp++));
		}
		json_object_object_add(args, "signal", signals);
		if(json_object_array_length(signals)) {
			afb_api_call_sync(api, "signal-composer", "subscribe",
					  args, NULL, NULL, NULL);
		} else {
			json_object_put(args);
		}
	}

	// Initialize event structures
	freq_event = afb_daemon_make_event("frequency");
	scan_event = afb_daemon_make_event("station_found");
	status_event = afb_daemon_make_event("status");
	rds_event = afb_daemon_make_event("rds");
	return 0;
}

const afb_binding_t afbBindingExport = {
	.info = "radio service",
	.api  = "radio",
	.specification = "Radio API",
	.verbs = verbs,
	.onevent = onevent,
	.init = init,
};
