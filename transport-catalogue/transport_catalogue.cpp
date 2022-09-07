#include "transport_catalogue.h"

#include <istream>
#include <string>
#include <sstream>
#include <string_view>
#include <algorithm>
#include <set>


using namespace std;
using namespace geo;

namespace transport {

void TransportCatalogue::AddStop(string&& stop_name, StopData&& stop) {
	
	if (auto word_it = stop_names_.find(stop_name); word_it != stop_names_.end()) {
		stops_[*word_it] = move(stop);
	}
	else {
		stops_[*stop_names_.insert(move(stop_name)).first] = move(stop);
	}
}

void TransportCatalogue::AddBus(string&& bus_name, list<string>&& moving_bus_path, bool path_is_ring) {

	BusData& bus = buses_[move(bus_name)];
	bus.SetStateRingOfPath(path_is_ring);
	for (string& stop_name : moving_bus_path) {
		if (auto word_it = stop_names_.find(stop_name); word_it != stop_names_.end()) {
			bus.GetPath().push_back(*word_it);
			stop_names_in_buses_path_.insert(*word_it);
		}
		else {
			const string& stop_name_in_path = *stop_names_.insert(move(stop_name)).first;
			bus.GetPath().push_back(stop_name_in_path);
			stop_names_in_buses_path_.insert(stop_name_in_path);
		}
	}
}

pair<StopData, bool> TransportCatalogue::GetStopData(const string_view stop_name) const {
	auto stop_it = stops_.find(stop_name);
	if (stop_it == stops_.end()) {
		return { StopData{Coordinates {}}, false };
	}
	else {
		return { stop_it->second, true };
	}
}

pair<const BusData&, bool> TransportCatalogue::GetBusData(const string_view bus_name) const {
	auto bus_it = buses_.find(string(bus_name));
	if (bus_it == buses_.end()) {
		static BusData path;
		return { path, false };
	}
	else {
		return { bus_it->second, true };
	}
}

set<string_view> TransportCatalogue::GetBusList(const std::string_view stop_name) const {
	set<string_view> buses_with_stop;

	if (!GetStopData(stop_name).second) {
		throw invalid_argument("Stop not found"s);
	}
	for (const auto& [bus_name, bus] : buses_) {
		auto bus_it = find(bus.GetPath().begin(), bus.GetPath().end(), stop_name);
		if (bus_it != bus.GetPath().end()) {
			buses_with_stop.insert(bus_name);
		}
	}
	return buses_with_stop;
}

std::unordered_set<std::string>& TransportCatalogue::GetStopNames() {
	return stop_names_;
}

const std::unordered_set<std::string>& TransportCatalogue::GetStopNames() const {
	return stop_names_;
}

std::vector<geo::Coordinates> TransportCatalogue::GetAllStopCoordinatesInBusPathes() const {
	vector<Coordinates> coordinates;
	for (auto& stop_name : stop_names_in_buses_path_) {
		coordinates.push_back(stops_.at(stop_name).GetCoordinates());
	}
	return coordinates;
}

std::set<std::string_view> TransportCatalogue::GetBusNames() const {
	set<string_view> bus_names;

	for (auto& [bus_name, _] : buses_) {
		bus_names.insert(bus_name);
	}
	return bus_names;
}

size_t TransportCatalogue::GetStopsCount() const {
	return stop_names_.size();
}

const std::unordered_map<std::string, BusData>& TransportCatalogue::GetBuses() const {
	return buses_;
}

size_t TransportCatalogue::GetDistance(std::string_view stop_from, std::string_view stop_to) const {
	auto stop_distances = GetStopData(stop_from).first.GetDistances();

	size_t distance = 0;

	if (auto distance_it = stop_distances.find(stop_to); distance_it != stop_distances.end()) {
		distance = distance_it->second;
	}
	else {
		distance = GetStopData(stop_to).first.GetDistances().at(stop_from);
	}
	return distance;
}

bool BusData::IsRing() const {
	return is_ring_;
}

const std::list<std::string_view>& BusData::GetPath() const {
	return path_;
}

std::list<std::string_view>& BusData::GetPath() {
	return path_;
}

void BusData::SetStateRingOfPath(bool is_ring) {
	is_ring_ = is_ring;
}

StopData::StopData(const Coordinates& coordinates) : coordinates_(coordinates) {}

StopData::StopData(const Coordinates& coordinates,
	Distances&& distance_to_stop) : coordinates_(coordinates),
	distance_to_stop_(move(distance_to_stop)) {}

const Coordinates& StopData::GetCoordinates() const {
	return coordinates_;
}

Coordinates& StopData::GetCoordinates() {
	return coordinates_;
}

const StopData::Distances& StopData::GetDistances() const {
	return distance_to_stop_;
}



} // namespace transport