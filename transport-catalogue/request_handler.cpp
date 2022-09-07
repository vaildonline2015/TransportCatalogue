#include "request_handler.h"
#include "transport_catalogue.h"
#include "json_reader.h"
#include "json.h"
#include "map_renderer.h"
#include "json_builder.h"
#include "router.h"
#include "transport_router.h"
#include "graph.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include <set>
#include <sstream>
#include <optional>

using namespace std;
using namespace transport;
using namespace json;
using namespace renderer;
using namespace routing;

RequestHandler::RequestHandler(const transport::TransportCatalogue& transport_catalogue, 
								const Requests& requests,
								const InputAttrs& attrs, 
								const std::optional<routing::TransportRouter>& router) :
																	transport_catalogue_(transport_catalogue), 
																	requests_(requests), 
																	attrs_(attrs),
																	router_(router){
}

void RequestHandler::ProcessRequests(ostream& os) {

	json::Builder result_builder;
	result_builder.StartArray();

	for (const Request& request : requests_) {

		json::Builder response_builder;
		response_builder.StartDict().Key("request_id"s).Value(request.id);

		if (request.type == TypeRequest::BUS) {
			ProcessBusRequest(request, response_builder);
		}
		else if (request.type == TypeRequest::STOP){
			ProcessStopRequest(request, response_builder);
		}
		else if (request.type == TypeRequest::ROUTE) {
			ProcessRouteRequest(request, response_builder);
		}
		else if (request.type == TypeRequest::MAP) {
			ProcessMapRequest(response_builder);
		}

		response_builder.EndDict();
		result_builder.Value(move(response_builder.Build().AsDict()));
	}
	os << setprecision(6);
	result_builder.EndArray().Build().Print(os);
}


template<typename ITERATOR>
size_t CalculateRoadLengthOneWay(const TransportCatalogue& transport_catalogue, ITERATOR begin_it, ITERATOR end_it) {
	size_t road_length = 0;

	for (auto stop_it = begin_it; next(stop_it) != end_it; ++stop_it) {
		auto next_stop_it = next(stop_it);
		auto stop_distances = transport_catalogue.GetStopData(*stop_it).first.GetDistances();
		if (auto distance_it = stop_distances.find(*next_stop_it); distance_it != stop_distances.end()) {
			road_length += distance_it->second;
		}
		else {
			road_length += transport_catalogue.GetStopData(*next_stop_it).first.GetDistances().at(*stop_it);
		}
	}
	return road_length;
}

void RequestHandler::ProcessBusRequest(const Request& request, json::Builder& response_builder) {
	string_view bus_name = request.name;
	pair<const BusData&, bool> find_bus_result = transport_catalogue_.GetBusData(bus_name);

	if (!find_bus_result.second) {
		response_builder.Key("error_message"s).Value("not found"s);
	}
	else {
		const BusData& bus = find_bus_result.first;

		size_t stop_names_on_route = bus.IsRing() ? bus.GetPath().size() : bus.GetPath().size() * 2 - 1;
		response_builder.Key("stop_count"s).Value(static_cast<int>(stop_names_on_route));

		size_t unique_stop_names_on_route = unordered_set<string_view>(bus.GetPath().begin(), bus.GetPath().end()).size();
		response_builder.Key("unique_stop_count"s).Value(static_cast<int>(unique_stop_names_on_route));

		double direct_geolength = 0.;
		for (auto stop_it = bus.GetPath().begin(); next(stop_it) != bus.GetPath().end(); ++stop_it) {
			direct_geolength += ComputeDistance(transport_catalogue_.GetStopData(*stop_it).first.GetCoordinates(), transport_catalogue_.GetStopData(*next(stop_it)).first.GetCoordinates());
		}

		size_t road_length = CalculateRoadLengthOneWay(transport_catalogue_, bus.GetPath().begin(), bus.GetPath().end());

		if (!bus.IsRing()) {
			direct_geolength *= 2;
			road_length += CalculateRoadLengthOneWay(transport_catalogue_, bus.GetPath().rbegin(), bus.GetPath().rend());
		}

		response_builder.Key("route_length"s).Value(static_cast<int>(road_length));
		response_builder.Key("curvature"s).Value(road_length / direct_geolength);
	}
}

void RequestHandler::ProcessStopRequest(const Request& request, json::Builder& response_builder) {
	if (!transport_catalogue_.GetStopData(request.name).second) {
		response_builder.Key("error_message"s).Value("not found"s);
	}
	else {
		set<string_view> buses_with_stop = transport_catalogue_.GetBusList(request.name);

		Builder buses_result;
		buses_result.StartArray();
		for (const auto& bus : buses_with_stop) {
			buses_result.Value(string{ bus });
		}
		response_builder.Key("buses"s).Value(move(buses_result.EndArray().Build().AsArray()));
	}
}

void RequestHandler::ProcessRouteRequest(const Request& request, json::Builder& response_builder) {
	
	size_t from = router_->GetVertexId(request.route_final_stops.from);
	size_t to = router_->GetVertexId(request.route_final_stops.to);

	auto result = router_->BuildRoute(from, to);

	if (result == nullopt) {
		response_builder.Key("error_message"s).Value("not found"s);
	}
	else {
		response_builder.Key("total_time"s).Value(result->weight);
		Builder path_items;
		path_items.StartArray();

		for (auto it = result->edges.begin(); it != result->edges.end(); ++it) {

			Builder wait_builder;
			wait_builder.StartDict();
			wait_builder.Key("type"s).Value("Wait"s).Key("stop_name"s);

			graph::VertexId vertex_from = router_->GetEdgeVertexFrom(*it);
			wait_builder.Value(string{ router_->GetVertexStopName(vertex_from) });

			int bus_wait_time = static_cast<int>(router_->GetWaitTime());
			wait_builder.Key("time"s).Value(bus_wait_time);
			path_items.Value(move(wait_builder.EndDict().Build().AsDict()));
			
			Builder bus_builder;
			bus_builder.StartDict();
			bus_builder.Key("type"s).Value("Bus"s).Key("bus"s).Value(string{ router_->GetEdgeInfo(*it).bus_name });
			bus_builder.Key("time"s).Value(router_->GetEdgeWeight(*it) - bus_wait_time).Key("span_count"s);
			bus_builder.Value(router_->GetEdgeInfo(*it).span_count);
			path_items.Value(move(bus_builder.EndDict().Build().AsDict()));
		}
		response_builder.Key("items"s).Value(move(path_items.EndArray().Build().AsArray()));
	}
}

void RequestHandler::ProcessMapRequest(json::Builder& response_builder) {
	stringstream ss;
	renderer::MapRenderer{ transport_catalogue_, attrs_.render_attrs}.Render(ss);
	response_builder.Key("map"s).Value(ss.str());
}