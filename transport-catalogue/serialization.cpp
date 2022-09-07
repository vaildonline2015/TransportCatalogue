#include "serialization.h"
#include "transport_catalogue.h"
#include "transport_router.h"
#include "router.h"

#include <transport_catalogue.pb.h>
#include <string>
#include <fstream>

using namespace std;
using namespace routing;

void ReadTransportRoutesData(const graph::Router<double>& router, serialize::TransportCatalogue& serialize_transport);
void ReadTransportBase(const serialize::TransportCatalogue& serialize_transport, transport::TransportCatalogue& transport);
void ReadRoutingSettings(const serialize::RoutingSettings& routing_settings, routing::Attrs& routing_attrs);

bool SerializeTransportCatalogue(	serialize::TransportCatalogue& serialize_transport, 
									const filesystem::path& serialize_result_path) {

	transport::TransportCatalogue transport;
	routing::Attrs routing_attrs = { 0,0 };
	ReadTransportBase(serialize_transport, transport);
	ReadRoutingSettings(serialize_transport.routing_settings(), routing_attrs);

	TransportRouter transport_router{ transport, routing_attrs };
	
	ReadTransportRoutesData(transport_router.GetRouter(), serialize_transport);

	ofstream ofs{ serialize_result_path, ios::binary };

	if (!serialize_transport.SerializeToOstream(&ofs)) {
		return false;
	}
	else {
		return true;
	}
}

void ReadRenderSettings(serialize::RenderSettings& render_settings, renderer::Attrs& render_attrs);
void ReadTransportRoutesData(const serialize::RoutesInternalData& serialize_routes_data, TransportRoutesData& routes_data);

bool DeserializeTransportCatalogue(filesystem::path& serialize_result_path, transport::TransportCatalogue& transport,
									InputAttrs& attrs, optional<routing::TransportRouter>& router,
									bool is_route_request_presence) {
	
	serialize::TransportCatalogue serialize_transport;

	ifstream ifs{ serialize_result_path, ios::binary };

	if (!serialize_transport.ParseFromIstream(&ifs)) {
		return false;
	}
	ReadTransportBase(serialize_transport, transport);
	ReadRenderSettings(*serialize_transport.mutable_render_settings(), attrs.render_attrs);

	if (is_route_request_presence) {

		ReadRoutingSettings(serialize_transport.routing_settings(), attrs.routing_attrs);
		
		TransportRoutesData routes_data;
		ReadTransportRoutesData(serialize_transport.routes_data(), routes_data);

		router.emplace(transport, attrs.routing_attrs, move(routes_data));
	}
	return true;
}

void ReadTransportRoutesData(const graph::Router<double>& router, serialize::TransportCatalogue& serialize_transport) {

	auto serialize_routes_data = serialize_transport.mutable_routes_data();

	for (auto& routes_data : router.GetRoutesInternalData()) {

		auto vector_routes_data = serialize_routes_data->add_routes_data();

		for (auto& route_data : routes_data) {

			auto serialize_route_data = vector_routes_data->add_route_data();

			if (route_data.has_value()) {

				serialize_route_data->mutable_weight()->set_value(route_data->weight);

				if (route_data->prev_edge.has_value()) {
					serialize_route_data->mutable_prev_edge()->set_value(route_data->prev_edge.value());
				}
				
			}
		}
	}
}

void ReadStopData(const serialize::StopData& stop_data, transport::TransportCatalogue& transport);
void ReadBusData(const serialize::BusData& bus_data, transport::TransportCatalogue& transport);

void ReadTransportBase(const serialize::TransportCatalogue &serialize_transport, transport::TransportCatalogue& transport) {

	auto& stops = serialize_transport.stops_data();
	auto& buses = serialize_transport.buses_data();

	for (auto& stop_data : stops) {
		ReadStopData(stop_data, transport);
	}
	for (auto& bus_data : buses) {
		ReadBusData(bus_data, transport);
	}
}

svg::Color GetColor(serialize::Color& color);
svg::Point GetPoint(const serialize::Point& point);

void ReadRenderSettings(serialize::RenderSettings& render_settings, renderer::Attrs& render_attrs) {

	render_attrs.map_width = render_settings.map_width();
	render_attrs.map_height = render_settings.map_height();
	render_attrs.map_padding = render_settings.map_padding();
	render_attrs.stop_radius = render_settings.stop_radius();
	render_attrs.line_width = render_settings.line_width();
	render_attrs.bus_label_font_size = render_settings.bus_label_font_size();
	render_attrs.stop_label_font_size = render_settings.stop_label_font_size();
	render_attrs.underlayer_width = render_settings.underlayer_width();

	render_attrs.bus_label_offset = GetPoint(render_settings.bus_label_offset());
	render_attrs.stop_label_offset = GetPoint(render_settings.stop_label_offset());
	render_attrs.underlayer_color = GetColor(*render_settings.mutable_underlayer_color());

	for (auto& color : *render_settings.mutable_stroke_colors()) {
		render_attrs.stroke_colors.push_back(GetColor(color));
	}
}

void ReadStopData(const serialize::StopData& stop_data, transport::TransportCatalogue& transport) {

	string stop_name = stop_data.name();
	double lat = stop_data.latitude();
	double lng = stop_data.longitude();

	if (!stop_data.road_distances_size()) {
		transport.AddStop(move(stop_name), transport::StopData{ geo::Coordinates{ lat, lng } });
	}
	else {
		unordered_map<string_view, size_t> distance_to_stop;

		auto& road_distances = stop_data.road_distances();

		for (auto& road_distance : road_distances) {

			auto& transport_stop_names = transport.GetStopNames();
			if (auto word_it = transport_stop_names.find(road_distance.stop_name()); word_it != transport_stop_names.end()) {
				distance_to_stop[*word_it] = road_distance.distance();
			}
			else {
				distance_to_stop[*transport_stop_names.insert(road_distance.stop_name()).first] = road_distance.distance();
			}
		}
		transport.AddStop(move(stop_name), transport::StopData{ geo::Coordinates{ lat, lng }, move(distance_to_stop) });
	}
}

void ReadBusData(const serialize::BusData& bus_data, transport::TransportCatalogue& transport) {

	bool  path_is_ring = bus_data.is_roundtrip();
	string bus_name = bus_data.name();

	list<string> bus_path;

	auto& stops = bus_data.stops();

	for (auto& stop : stops) {
		bus_path.push_back(stop);
	}
	transport.AddBus(move(bus_name), move(bus_path), path_is_ring);
}

void ReadRoutingSettings(const serialize::RoutingSettings& routing_settings, routing::Attrs& routing_attrs) {

	routing_attrs.bus_velocity = routing_settings.bus_velocity();
	routing_attrs.bus_wait_time = routing_settings.bus_wait_time();
}

svg::Color GetColor(serialize::Color& color) {
	if (!color.color_str().empty()) {
		return move(*color.mutable_color_str());
	}
	else {
		uint8_t red = static_cast<uint8_t>(color.red());
		uint8_t green = static_cast<uint8_t>(color.green());
		uint8_t blue = static_cast<uint8_t>(color.blue());

		if (!color.has_opacity()) {
			return svg::Rgb(red, green, blue);
		}
		else {
			double opacity = color.opacity().value();
			return svg::Rgba(red, green, blue, opacity);
		}
	}
}

svg::Point GetPoint(const serialize::Point& point) {
	return svg::Point{ point.x(), point.y() };
}

void ReadTransportRoutesData(const serialize::RoutesInternalData& serialize_routes_data, TransportRoutesData& routes_data) {

	for (auto& serialize_vector_route_data : serialize_routes_data.routes_data()) {

		auto& vector_route_data = routes_data.emplace_back();

		for (auto& serialize_route_data : serialize_vector_route_data.route_data()) {

			auto& route_data = vector_route_data.emplace_back();

			if (serialize_route_data.has_weight()) {

				route_data.emplace();
				route_data->weight = serialize_route_data.weight().value();
			
				if (serialize_route_data.has_prev_edge()) {
					route_data->prev_edge.emplace(serialize_route_data.prev_edge().value());
				}
			}
		}
	}
}