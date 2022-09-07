#pragma once

#include "graph.h"
#include "transport_catalogue.h"
#include "router.h"

#include <string_view>
#include <exception>
#include <vector>
#include <memory>
#include <optional>
#include <numeric>

namespace routing {

struct Attrs {
	double bus_velocity = 0; // kph
	size_t bus_wait_time = 0; // minutes
};

struct EdgeInfo {
	std::string_view bus_name;
	int span_count;
};

using TransportEdge = graph::Edge<double>; // weight = minutes
using TransportGraph = graph::DirectedWeightedGraph<double>;
using RouteInfo = std::optional < graph::Router<double>::RouteInfo>;
using graph::VertexId;
using TransportRoutesData = graph::Router<double>::RoutesInternalData;

class TransportRouter {
public:
	TransportRouter(const transport::TransportCatalogue& transport_catalogue, Attrs attrs);
	TransportRouter(const transport::TransportCatalogue& transport_catalogue, Attrs attrs, TransportRoutesData&& routes_data);

	RouteInfo BuildRoute(size_t vertex_id_from, size_t vertex_id_to) const;

	VertexId GetVertexId(std::string_view stop_name) const;

	VertexId GetEdgeVertexFrom(graph::EdgeId edge_id) const;
	std::string_view GetVertexStopName(graph::EdgeId edge_id) const;

	double GetEdgeWeight(graph::EdgeId edge_id) const;
	const EdgeInfo& GetEdgeInfo(graph::EdgeId edge_id) const;

	size_t GetWaitTime() const;

	const TransportGraph& GetGraph() const;

	const graph::Router<double>& GetRouter() const;

private:
	inline void RouterInit();

	void MatchStopsWithVertexId();

	template<typename ITERATOR>
	void AddBusEdgesOneWay(std::string_view bus_name, ITERATOR begin_it, ITERATOR end_it);

	void AddEdge(const TransportEdge& edge, const EdgeInfo& edge_info);

	const transport::TransportCatalogue& transport_catalogue_;
	routing::Attrs attrs_;
	TransportGraph graph_;
	std::unordered_map<std::string_view, size_t> stops_to_vertex_id_;
	std::vector<std::string_view> vertex_id_to_stop_name_;
	std::vector<EdgeInfo> edge_id_to_info_;
	std::optional<graph::Router<double>> router_;
};

template<typename ITERATOR>
void TransportRouter::AddBusEdgesOneWay(std::string_view bus_name, ITERATOR path_begin_it, ITERATOR path_end_it) {

	std::vector<double> travel_times;

	for (auto stop_it = path_begin_it; next(stop_it) != path_end_it; ++stop_it) {

		TransportEdge edge;
		edge.from = stops_to_vertex_id_.at(*stop_it);
		edge.to = stops_to_vertex_id_.at(*next(stop_it));

		size_t distance = transport_catalogue_.GetDistance(*stop_it, *next(stop_it));
		double travel_time = (distance / (attrs_.bus_velocity / 3.6)) / 60;
		edge.weight = travel_time + attrs_.bus_wait_time;

		EdgeInfo edge_info;
		edge_info.bus_name = bus_name;
		edge_info.span_count = 1;

		AddEdge(edge, edge_info);

		auto copy_stop_it = stop_it;

		for (auto it = travel_times.rbegin(); it != travel_times.rend(); ++it) {
			edge.weight += *it;
			edge.from = stops_to_vertex_id_.at(*--copy_stop_it);
			++edge_info.span_count;
			AddEdge(edge, edge_info);
		}
		travel_times.push_back(travel_time);
	}
	
}

} // namespace routing