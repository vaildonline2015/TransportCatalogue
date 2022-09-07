#pragma once

#include "transport_catalogue.h"
#include "json_reader.h"
#include "json_builder.h"
#include "transport_router.h"
#include "router.h"

#include <iostream>
#include <optional>

class RequestHandler {
public:
	RequestHandler(const transport::TransportCatalogue& transport_catalogue, const Requests& requests, 
					const InputAttrs& attrs, const std::optional<routing::TransportRouter>& router);

	void ProcessRequests(std::ostream& os);

private:

	void ProcessBusRequest(const Request& request, json::Builder& response_builder);
	void ProcessStopRequest(const Request& request, json::Builder& response_builder);
	void ProcessRouteRequest(const Request& request, json::Builder& response_builder);
	void ProcessMapRequest(json::Builder& response_builder);

	const transport::TransportCatalogue& transport_catalogue_; 
	const Requests& requests_;
	const InputAttrs& attrs_;
	const std::optional<routing::TransportRouter>& router_;
};
