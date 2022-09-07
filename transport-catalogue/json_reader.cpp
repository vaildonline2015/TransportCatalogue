#include "json.h"
#include "json_reader.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

#include <transport_catalogue.pb.h>
#include <istream>
#include <vector>

using namespace std;
using namespace transport;
using namespace json;
using namespace geo;
using namespace renderer;

void Requests::Add(Request&& request) {
	requests_.push_back(move(request));
}

void Requests::SetRouteRequestPresence() {
	has_route_request_ = true;
}

bool Requests::IsRouteRequestPresence() const {
	return has_route_request_;
}

vector<Request>::iterator Requests::begin() {
	return requests_.begin();
}
vector<Request>::iterator Requests::end() {
	return requests_.end();
}
vector<Request>::const_iterator Requests::begin() const {
	return requests_.cbegin();
}
vector<Request>::const_iterator Requests::end() const {
	return requests_.cend();
}


void ReadBaseRequests(Array& base_requests, serialize::TransportCatalogue& serialize_transport);
void ReadRenderSettings(Dict& render_settings, serialize::RenderSettings& render_attrs);
void ReadRoutingSettings(const Dict& routing_settings, serialize::RoutingSettings& routing_attrs);

void ReadInput(	std::istream& is, serialize::TransportCatalogue& serialize_transport, 
				filesystem::path& serialize_result_path) {
	
	Node node = LoadNode(is);

	serialize_result_path = node.AsDict().at("serialization_settings"s).AsDict().at("file"s).AsString();

	Array& base_requests = node.AsDict().at("base_requests"s).AsArray();
	ReadBaseRequests(base_requests, serialize_transport);

	Dict& render_settings = node.AsDict().at("render_settings"s).AsDict();
	ReadRenderSettings(render_settings , *serialize_transport.mutable_render_settings());

	Dict& routing_settings = node.AsDict().at("routing_settings"s).AsDict();
	ReadRoutingSettings(routing_settings, *serialize_transport.mutable_routing_settings());
}

void ReadStatRequests(Array& stat_requests, Requests& requests);

void ReadInput(std::istream& is, Requests& requests, std::filesystem::path& serialize_result_path) {

	Node node = LoadNode(is);

	serialize_result_path = node.AsDict().at("serialization_settings"s).AsDict().at("file"s).AsString();

	Array& stat_requests = node.AsDict().at("stat_requests"s).AsArray();
	ReadStatRequests(stat_requests, requests);
}

void ReadStopDataRequest(json::Dict& request, serialize::StopData* stop_data);
void ReadBusDataRequest(json::Dict& request, serialize::BusData* bus_data);

void ReadBaseRequests(Array& base_requests, serialize::TransportCatalogue& serialize_transport) {

	for (Node& node_request : base_requests) {

		Dict& request = node_request.AsDict();

		if (request.at("type"s) == "Stop"s) {
			ReadStopDataRequest(request, serialize_transport.add_stops_data());
		}
		else if (request.at("type"s) == "Bus"s) {
			ReadBusDataRequest(request, serialize_transport.add_buses_data());
		}
	}
}

void ReadJsonToPoint(const json::Node& point, serialize::Point& serialize_point);
void ReadJsonToColor(json::Node& color, serialize::Color& serialize_color);

void ReadRenderSettings(Dict& render_settings, serialize::RenderSettings& render_attrs) {

	render_attrs.set_map_width(render_settings.at("width"s).AsDouble());
	render_attrs.set_map_height(render_settings.at("height"s).AsDouble());
	render_attrs.set_map_padding(render_settings.at("padding"s).AsDouble());
	render_attrs.set_stop_radius(render_settings.at("stop_radius"s).AsDouble());
	render_attrs.set_line_width(render_settings.at("line_width"s).AsDouble());
	render_attrs.set_bus_label_font_size(static_cast<uint32_t>(render_settings.at("bus_label_font_size"s).AsInt()));
	render_attrs.set_stop_label_font_size(static_cast<uint32_t>(render_settings.at("stop_label_font_size"s).AsInt()));
	render_attrs.set_underlayer_width(render_settings.at("underlayer_width"s).AsDouble());

	ReadJsonToPoint(render_settings.at("bus_label_offset"s), *render_attrs.mutable_bus_label_offset());
	ReadJsonToPoint(render_settings.at("stop_label_offset"s), *render_attrs.mutable_stop_label_offset());
	ReadJsonToColor(render_settings.at("underlayer_color"s), *render_attrs.mutable_underlayer_color());
	
	json::Array color_palette = render_settings.at("color_palette"s).AsArray();
	for (json::Node& color : color_palette) {
		ReadJsonToColor(color, *render_attrs.add_stroke_colors());
	}
}

void ReadJsonToPoint(const json::Node& point, serialize::Point& serialize_point) {
	json::Array coordinates = point.AsArray();
	serialize_point.set_x(coordinates[0].AsDouble());
	serialize_point.set_y(coordinates[1].AsDouble());
}

void ReadJsonToColor(json::Node& color, serialize::Color& serialize_color) {

	if (color.IsString()) {
		serialize_color.set_color_str(move(color.AsString()));
	}
	else {
		uint8_t red = static_cast<uint8_t>(color.AsArray()[0].AsInt());
		uint8_t green = static_cast<uint8_t>(color.AsArray()[1].AsInt());
		uint8_t blue = static_cast<uint8_t>(color.AsArray()[2].AsInt());

		serialize_color.set_red(red);
		serialize_color.set_green(green);
		serialize_color.set_blue(blue);

		if (color.AsArray().size() == 4) {
			double opacity = color.AsArray()[3].AsDouble();
			serialize_color.mutable_opacity()->set_value(opacity);
		}
	}
}

void ReadStatRequests(Array& stat_requests, Requests& requests) {
	for (Node& request : stat_requests) {

		int id = request.AsDict().at("id"s).AsInt();
		const string& type_request = request.AsDict().at("type"s).AsString();
		TypeRequest type;

		if (type_request == "Bus"s) {
			type = TypeRequest::BUS;
		}
		else if (type_request == "Stop"s) {
			type = TypeRequest::STOP;
		}
		else if (type_request == "Map"s) {
			type = TypeRequest::MAP;
		}
		else {
			type = TypeRequest::ROUTE;
			requests.SetRouteRequestPresence();
		}

		string name;

		if (type == TypeRequest::BUS || type == TypeRequest::STOP) {
			name = move(request.AsDict().at("name"s).AsString());
		}

		RouteFinalStops final_stops;

		if (type == TypeRequest::ROUTE) {
			final_stops.from = move(request.AsDict().at("from"s).AsString());
			final_stops.to = move(request.AsDict().at("to"s).AsString());
		}

		requests.Add(Request{ id , move(name), type, move(final_stops) });
	}
}

void ReadStopDataRequest(json::Dict& request, serialize::StopData* stop_data) {

	stop_data->set_name(move(request.at("name"s).AsString()));
	stop_data->set_latitude(request.at("latitude"s).AsDouble());
	stop_data->set_longitude(request.at("longitude"s).AsDouble());

	if (auto it = request.find("road_distances"s); it != request.end()) {

		Dict& node_distances = it->second.AsDict();

		for (auto& [stop_name, distance] : node_distances) {

			auto road_distance = stop_data->add_road_distances();
			road_distance->set_stop_name(move(stop_name));
			road_distance->set_distance(distance.AsInt());
		}
	}
}

void ReadBusDataRequest(json::Dict& request, serialize::BusData* bus_data) {

	bus_data->set_is_roundtrip(request.at("is_roundtrip"s).AsBool());
	bus_data->set_name(move(request.at("name"s).AsString()));

	list<string> bus_path;

	for (Node& stop : request.at("stops"s).AsArray()) {

		*bus_data->add_stops() = move(stop.AsString());
	}
}

void ReadRoutingSettings(const Dict& routing_settings, serialize::RoutingSettings& routing_attrs) {

	routing_attrs.set_bus_velocity(routing_settings.at("bus_velocity"s).AsDouble());
	routing_attrs.set_bus_wait_time(static_cast<size_t>(routing_settings.at("bus_wait_time"s).AsInt()));
}