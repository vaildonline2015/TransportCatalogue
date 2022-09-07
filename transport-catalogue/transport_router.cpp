#include "transport_router.h"
#include "transport_catalogue.h"
#include "algorithm"


namespace routing {

using namespace std;
using namespace transport;

TransportRouter::TransportRouter(const TransportCatalogue& transport_catalogue, Attrs attrs)
								:
								transport_catalogue_(transport_catalogue),
								attrs_(attrs),
								graph_(transport_catalogue.GetStopNames().size())

{
	RouterInit();
	router_.emplace(graph_);
}

TransportRouter::TransportRouter(const TransportCatalogue& transport_catalogue, Attrs attrs, TransportRoutesData&& routes_data)
	:
	transport_catalogue_(transport_catalogue),
	attrs_(attrs),
	graph_(transport_catalogue.GetStopNames().size())

{
	RouterInit();
	router_.emplace(graph_, move(routes_data));
}

void TransportRouter::RouterInit() {

	vertex_id_to_stop_name_.reserve(transport_catalogue_.GetStopNames().size());
	MatchStopsWithVertexId();

	const std::unordered_map<std::string, BusData>& buses = transport_catalogue_.GetBuses();

	for (auto& [bus_name, bus] : buses) {

		AddBusEdgesOneWay(bus_name, bus.GetPath().begin(), bus.GetPath().end());

		if (!bus.IsRing()) {
			AddBusEdgesOneWay(bus_name, bus.GetPath().rbegin(), bus.GetPath().rend());
		}
	}
}

void TransportRouter::AddEdge(const TransportEdge& edge, const EdgeInfo& edge_info) {
	graph_.AddEdge(edge);
	edge_id_to_info_.push_back(edge_info);
}

RouteInfo TransportRouter::BuildRoute(size_t vertex_id_from, size_t vertex_id_to) const {
	return router_->BuildRoute(vertex_id_from, vertex_id_to);
}

void TransportRouter::MatchStopsWithVertexId() {
	size_t count = 0;

	const std::unordered_set<std::string>& stops = transport_catalogue_.GetStopNames();

	for_each(stops.begin(), stops.end(), 
			 [&count, this](string_view stop) {
												stops_to_vertex_id_[stop] = count++; 
												vertex_id_to_stop_name_.push_back(stop);
											  }
			);
}

VertexId TransportRouter::GetVertexId(string_view stop_name) const {
	return stops_to_vertex_id_.at(stop_name);
}

VertexId TransportRouter::GetEdgeVertexFrom(graph::EdgeId edge_id) const {
	return graph_.GetEdge(edge_id).from;
}

std::string_view TransportRouter::GetVertexStopName(graph::EdgeId edge_id) const {
	return vertex_id_to_stop_name_[edge_id];
}

double TransportRouter::GetEdgeWeight(graph::EdgeId edge_id) const {
	return graph_.GetEdge(edge_id).weight;
}

const EdgeInfo& TransportRouter::GetEdgeInfo(graph::EdgeId edge_id) const {
	return edge_id_to_info_.at(edge_id);
}

size_t TransportRouter::GetWaitTime() const {
	return attrs_.bus_wait_time;
}

const TransportGraph& TransportRouter::GetGraph() const {
	return graph_;
}

const graph::Router<double>& TransportRouter::GetRouter() const {
	return *router_;
}

} //namespace routing