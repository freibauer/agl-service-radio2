// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright (C) 2022 Konsulko Group
 */

#include "RadioImpl.h"

// C backend implementations
extern "C" {
#include "radio_impl_null.h"
#include "radio_impl_rtlsdr.h"
#include "radio_impl_kingfisher.h"
#include "radio_impl_tef665x.h"
}

using grpc::StatusCode;
using automotivegradelinux::Band;
using automotivegradelinux::BAND_UNSPECIFIED;
using automotivegradelinux::BAND_AM;
using automotivegradelinux::BAND_FM;
using automotivegradelinux::StereoMode;
using automotivegradelinux::STEREO_MODE_UNSPECIFIED;
using automotivegradelinux::STEREO_MODE_MONO;
using automotivegradelinux::STEREO_MODE_STEREO;
using automotivegradelinux::ScanDirection;
using automotivegradelinux::SCAN_DIRECTION_FORWARD;
using automotivegradelinux::SCAN_DIRECTION_BACKWARD;

RadioImpl::RadioImpl()
{
}

bool RadioImpl::Detect()
{
	// Probe for radio backends
	m_radio_impl_ops = &rtlsdr_impl_ops;
	int rc = m_radio_impl_ops->probe();
	if(rc != 0) {
		// Look for Kingfisher Si4689
		m_radio_impl_ops = &kf_impl_ops;
		rc = m_radio_impl_ops->probe();
	}
#if 0
	if(rc != 0) {
		m_radio_impl_ops = &tef665x_impl_ops;
		rc = m_radio_impl_ops->probe();
	}
#endif
	if (rc != 0) {
		m_radio_impl_ops = &null_impl_ops;
		rc = m_radio_impl_ops->probe();
	}
	if (rc != 0) {
		// We don't expect the null implementation to fail probe, but just in case...
		std::cerr << "No radio device found, exiting" << std::endl;
		return false;
	}
	// Try to initialize detected backend
	rc = m_radio_impl_ops->init();
	if(rc < 0) {
		std::cerr << m_radio_impl_ops->name << " initialization failed" << std::endl;
		return false;
	}
	std::cout << m_radio_impl_ops->name << "found" << std::endl;
	m_radio_impl_ops->set_frequency_callback(FrequencyCallback, this);
	m_radio_impl_ops->set_frequency_callback(FrequencyCallback, this);
#if 0	
	if(m_radio_impl_ops->set_rds_callback) {
		m_radio_impl_ops->set_rds_callback(RDSCallback);
	}
#endif
	return true;
}

Status RadioImpl::GetFrequency(ServerContext* context, const GetFrequencyRequest* request, GetFrequencyResponse* response)
{
	response->set_frequency(m_radio_impl_ops->get_frequency());
	return Status::OK;
}

Status RadioImpl::SetFrequency(ServerContext* context, const SetFrequencyRequest* request, SetFrequencyResponse* response)
{
	radio_band_t band = m_radio_impl_ops->get_band();
	uint32_t min_frequency = m_radio_impl_ops->get_min_frequency(band);
	uint32_t max_frequency = m_radio_impl_ops->get_max_frequency(band);
	uint32_t step = m_radio_impl_ops->get_frequency_step(band);

	uint32_t frequency = request->frequency();
	if(frequency < min_frequency ||
	   frequency > max_frequency ||
	   (frequency - min_frequency) % step) {
		//afb_req_reply(request, NULL, "failed", "Invalid frequency");
		return Status::OK;
	}
	m_radio_impl_ops->set_frequency(frequency);
	return Status::OK;
}

Status RadioImpl::GetBand(ServerContext* context, const GetBandRequest* request, GetBandResponse* response)
{
	Band band = BAND_UNSPECIFIED;
	radio_band_t impl_band = m_radio_impl_ops->get_band();

	if (impl_band == RADIO_BAND_AM)
		band = BAND_AM;
	else if (impl_band == RADIO_BAND_FM)
		band = BAND_FM;
	response->set_band(band);
	return Status::OK;
}

Status RadioImpl::SetBand(ServerContext* context, const SetBandRequest* request, SetBandResponse* response)
{
	radio_band_t impl_band = RADIO_BAND_UNSPECIFIED;
	if (request->band() == BAND_AM)
		impl_band = RADIO_BAND_AM;
	else if (request->band() == BAND_FM)
		impl_band = RADIO_BAND_FM;

	if (impl_band != RADIO_BAND_UNSPECIFIED) {
		m_radio_impl_ops->set_band(impl_band);
	} else {
		// FIXME: Indicate error
		return Status::OK;
	}
	
	return Status::OK;
}

Status RadioImpl::GetBandSupported(ServerContext* context, const GetBandSupportedRequest* request, GetBandSupportedResponse* response)
{
	radio_band_t impl_band = RADIO_BAND_UNSPECIFIED;
	if (request->band() == BAND_AM)
		impl_band = RADIO_BAND_AM;
	else if (request->band() == BAND_FM)
		impl_band = RADIO_BAND_FM;

	if (impl_band != RADIO_BAND_UNSPECIFIED)
		response->set_supported(m_radio_impl_ops->band_supported(impl_band));
	else
		response->set_supported(false);

	return Status::OK;
}

Status RadioImpl::GetBandParameters(ServerContext* context, const GetBandParametersRequest* request, GetBandParametersResponse* response)
{
	radio_band_t impl_band = RADIO_BAND_UNSPECIFIED;
	if (request->band() == BAND_AM)
		impl_band = RADIO_BAND_AM;
	else if (request->band() == BAND_FM)
		impl_band = RADIO_BAND_FM;

	if (impl_band != RADIO_BAND_UNSPECIFIED) {
		response->set_min(m_radio_impl_ops->get_min_frequency(impl_band));
		response->set_max(m_radio_impl_ops->get_max_frequency(impl_band));
		response->set_step(m_radio_impl_ops->get_frequency_step(impl_band));
	} else {
		// FIXME: Indicate error
		return Status::OK;
	}
	return Status::OK;
}

Status RadioImpl::GetStereoMode(ServerContext* context, const GetStereoModeRequest* request, GetStereoModeResponse* response)
{
	StereoMode mode = STEREO_MODE_UNSPECIFIED;
	radio_stereo_mode_t impl_mode = m_radio_impl_ops->get_stereo_mode();

	if (impl_mode == RADIO_MODE_MONO)
		mode = STEREO_MODE_MONO;
	else if (impl_mode == RADIO_MODE_STEREO)
		mode = STEREO_MODE_STEREO;
	response->set_mode(mode);
	return Status::OK;
}

Status RadioImpl::SetStereoMode(ServerContext* context, const SetStereoModeRequest* request, SetStereoModeResponse* response)
{
	radio_stereo_mode_t impl_mode = RADIO_MODE_UNSPECIFIED;
	if (request->mode() == STEREO_MODE_MONO)
		impl_mode = RADIO_MODE_MONO;
	else if (request->mode() == STEREO_MODE_STEREO)
		impl_mode = RADIO_MODE_STEREO;

	if (impl_mode != RADIO_MODE_UNSPECIFIED) {
		m_radio_impl_ops->set_stereo_mode(impl_mode);
	} else {
		// FIXME: Indicate error
		return Status::OK;
	}
	return Status::OK;
}

Status RadioImpl::Start(ServerContext* context, const StartRequest* request, StartResponse* response)
{
	m_radio_impl_ops->set_output(NULL);
	m_radio_impl_ops->start();
	m_playing = true;

	StatusResponse status_response;
	auto play_status = status_response.mutable_play();
	play_status->set_playing(true);
	SendStatusResponse(status_response);
	
	return Status::OK;
}

Status RadioImpl::Stop(ServerContext* context, const StopRequest* request, StopResponse* response)
{
	m_radio_impl_ops->stop();
	m_playing = false;

	StatusResponse status_response;
	auto play_status = status_response.mutable_play();
	play_status->set_playing(false);
	SendStatusResponse(status_response);

	return Status::OK;
}

Status RadioImpl::ScanStart(ServerContext* context, const ScanStartRequest* request, ScanStartResponse* response)
{
	radio_scan_direction_t impl_direction = RADIO_SCAN_UNSPECIFIED;
	if (request->direction() == SCAN_DIRECTION_FORWARD)
		impl_direction = RADIO_SCAN_FORWARD;
	else if (request->direction() == SCAN_DIRECTION_BACKWARD)
		impl_direction = RADIO_SCAN_BACKWARD;

	if (impl_direction != RADIO_SCAN_UNSPECIFIED) {
		m_radio_impl_ops->scan_start(impl_direction, ScanCallback, this);
	} else {
		// FIXME: Indicate error
		return Status::OK;
	}

	return Status::OK;
}

Status RadioImpl::ScanStop(ServerContext* context, const ScanStopRequest* request, ScanStopResponse* response)
{
	m_radio_impl_ops->scan_stop();
	return Status::OK;
}

Status RadioImpl::GetRDS(ServerContext* context, const GetRDSRequest* request, GetRDSResponse* response)
{
	return Status::OK;
}

Status RadioImpl::GetQuality(ServerContext* context, const GetQualityRequest* request, GetQualityResponse* response)
{
	return Status::OK;
}

Status RadioImpl::SetAlternativeFrequency(ServerContext* context, const SetAlternativeFrequencyRequest* request, SetAlternativeFrequencyResponse* response)
{
	return Status::OK;
}

Status RadioImpl::GetStatusEvents(ServerContext* context, const StatusRequest* request, ServerWriter<StatusResponse>* writer)
{
	// Save client information
	m_clients_mutex.lock();
	m_clients.push_back(std::pair(context, writer));
	m_clients_mutex.unlock();

	// For now block until client disconnect / server shutdown
	// A switch to the async or callback server APIs might be more elegant than
        // holding the thread like this, and may be worth investigating at some point.
	std::unique_lock lock(m_done_mutex);
	m_done_cv.wait(lock, [context, this]{ return (context->IsCancelled() || m_done); });

	return Status::OK;
}

void RadioImpl::SendStatusResponse(StatusResponse &response)
{
	const std::lock_guard<std::mutex> lock(m_clients_mutex);

	if (m_clients.empty())
		return;

	auto it = m_clients.begin();
	while (it != m_clients.end()) {
		if (it->first->IsCancelled()) {
			// Client has gone away, remove from list
			std::cout << "Removing cancelled RPC client!" << std::endl;
			it = m_clients.erase(it);

			// We're not exiting, but wake up blocked client RPC handlers so
			// the canceled one will clean exit.
			// Note that in practice this means the client RPC handler thread
			// sticks around until the next status event is sent.
			m_done_cv.notify_all();

			continue;
		} else {
			it->second->Write(response);
			++it;
		}
	}
}


void RadioImpl::HandleFrequencyEvent(uint32_t frequency)
{
	StatusResponse response;
	auto frequency_status = response.mutable_frequency();
	frequency_status->set_frequency(frequency);
	SendStatusResponse(response);
}

void RadioImpl::HandleScanEvent(uint32_t frequency)
{
	StatusResponse response;
	auto scan_status = response.mutable_scan();
	scan_status->set_station_found(frequency);
	SendStatusResponse(response);
}

void RadioImpl::HandleRDSEvent(void *rds_data)
{
	// TODO
}

