#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "serialization.h"

#include <istream>
#include <vector>
#include <transport_catalogue.pb.h>
#include <filesystem>

enum class TypeRequest {
	BUS,
	STOP,
	MAP,
	ROUTE
};

struct RouteFinalStops {
	std::string from;
	std::string to;
};

struct Request {
	int id;
	std::string name;
	TypeRequest type;
	RouteFinalStops route_final_stops;
};

class Requests {
public:
	void Add(Request&& request);
	void SetRouteRequestPresence();
	bool IsRouteRequestPresence() const;

	std::vector<Request>::iterator begin();
	std::vector<Request>::iterator end();
	std::vector<Request>::const_iterator begin() const;
	std::vector<Request>::const_iterator end() const;
private:
	std::vector<Request> requests_;
	bool has_route_request_ = false;
};

void ReadInput(	std::istream& is, serialize::TransportCatalogue& serialize_transport,
				std::filesystem::path& serialize_result_path);

void ReadInput(std::istream& is, Requests& requests, std::filesystem::path& serialize_result_path);