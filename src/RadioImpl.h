// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright (C) 2022 Konsulko Group
 */

#ifndef RADIO_IMPL_H
#define RADIO_IMPL_H

#include <mutex>
#include <list>
#include <condition_variable>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "radio.grpc.pb.h"
extern "C" {
#include "radio_impl.h"
}

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;

using automotivegradelinux::Radio;
using automotivegradelinux::GetFrequencyRequest;
using automotivegradelinux::GetFrequencyResponse;
using automotivegradelinux::SetFrequencyRequest;
using automotivegradelinux::SetFrequencyResponse;
using automotivegradelinux::GetBandRequest;
using automotivegradelinux::GetBandResponse;
using automotivegradelinux::SetBandRequest;
using automotivegradelinux::SetBandResponse;
using automotivegradelinux::GetBandSupportedRequest;
using automotivegradelinux::GetBandSupportedResponse;
using automotivegradelinux::GetBandParametersRequest;
using automotivegradelinux::GetBandParametersResponse;
using automotivegradelinux::GetStereoModeRequest;
using automotivegradelinux::GetStereoModeResponse;
using automotivegradelinux::SetStereoModeRequest;
using automotivegradelinux::SetStereoModeResponse;
using automotivegradelinux::GetStereoModeRequest;
using automotivegradelinux::StartRequest;
using automotivegradelinux::StartResponse;
using automotivegradelinux::StopRequest;
using automotivegradelinux::StopResponse;
using automotivegradelinux::ScanStartRequest;
using automotivegradelinux::ScanStartResponse;
using automotivegradelinux::ScanStopRequest;
using automotivegradelinux::ScanStopResponse;
using automotivegradelinux::GetRDSRequest;
using automotivegradelinux::GetRDSResponse;
using automotivegradelinux::GetQualityRequest;
using automotivegradelinux::GetQualityResponse;
using automotivegradelinux::SetAlternativeFrequencyRequest;
using automotivegradelinux::SetAlternativeFrequencyResponse;
using automotivegradelinux::StatusRequest;
using automotivegradelinux::StatusResponse;


class RadioImpl final : public Radio::Service
{
public:
	explicit RadioImpl();

	bool Detect();

	Status GetFrequency(ServerContext* context, const GetFrequencyRequest* request, GetFrequencyResponse* response) override;
	Status SetFrequency(ServerContext* context, const SetFrequencyRequest* request, SetFrequencyResponse* response) override;
	Status GetBand(ServerContext* context, const GetBandRequest* request, GetBandResponse* response) override;
	Status SetBand(ServerContext* context, const SetBandRequest* request, SetBandResponse* response) override;
	Status GetBandSupported(ServerContext* context, const GetBandSupportedRequest* request, GetBandSupportedResponse* response) override;
	Status GetBandParameters(ServerContext* context, const GetBandParametersRequest* request, GetBandParametersResponse* response) override;
	Status GetStereoMode(ServerContext* context, const GetStereoModeRequest* request, GetStereoModeResponse* response) override;
	Status SetStereoMode(ServerContext* context, const SetStereoModeRequest* request, SetStereoModeResponse* response) override;
	Status Start(ServerContext* context, const StartRequest* request, StartResponse* response) override;
	Status Stop(ServerContext* context, const StopRequest* request, StopResponse* response) override;
	Status ScanStart(ServerContext* context, const ScanStartRequest* request, ScanStartResponse* response) override;
	Status ScanStop(ServerContext* context, const ScanStopRequest* request, ScanStopResponse* response) override;
	Status GetRDS(ServerContext* context, const GetRDSRequest* request, GetRDSResponse* response) override;
	Status GetQuality(ServerContext* context, const GetQualityRequest* request, GetQualityResponse* response) override;
	Status SetAlternativeFrequency(ServerContext* context, const SetAlternativeFrequencyRequest* request, SetAlternativeFrequencyResponse* response) override;
	Status GetStatusEvents(ServerContext* context, const StatusRequest* request, ServerWriter<StatusResponse>* writer) override;

        void Shutdown() { m_done = true; m_done_cv.notify_all(); }

	void SendStatusResponse(StatusResponse &response);

	static void FrequencyCallback(uint32_t frequency, void *data) {
		if (data)
			((RadioImpl*) data)->HandleFrequencyEvent(frequency);
	}

	static void ScanCallback(uint32_t frequency, void *data) {
		if (data)
			((RadioImpl*) data)->HandleScanEvent(frequency);
	}

	static void RDSCallback(void *rds_data, void *data) {
		if (data)
			((RadioImpl*) data)->HandleRDSEvent(rds_data);
	}

private:
	void HandleFrequencyEvent(uint32_t frequency);
	void HandleScanEvent(uint32_t frequency);
	void HandleRDSEvent(void *rds_data);

	std::mutex m_clients_mutex;
	std::list<std::pair<ServerContext*, ServerWriter<StatusResponse>*> > m_clients;

	std::mutex m_done_mutex;
	std::condition_variable m_done_cv;
	bool m_done = false;

	radio_impl_ops_t *m_radio_impl_ops = NULL;
	bool m_playing = false;
};

#endif // RADIO_IMPL_H
