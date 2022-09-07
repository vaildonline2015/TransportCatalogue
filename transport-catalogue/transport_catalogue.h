#pragma once

#include "geo.h"

#include <string_view>
#include <istream>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <string>
#include <set>
#include <vector>

namespace transport {

class BusData {
public:
	bool IsRing() const;

	const std::list<std::string_view>& GetPath() const;
	std::list<std::string_view>& GetPath();

	void SetStateRingOfPath(bool is_ring);

private:
	bool is_ring_;
	std::list<std::string_view> path_;
};

class StopData {
	using Distances = std::unordered_map<std::string_view, size_t>; // size_t - meters

public:
	StopData() = default;
	explicit StopData(const geo::Coordinates& coordinates);
	StopData(const geo::Coordinates& coordinates, Distances&& distance_to_stop);

	const geo::Coordinates& GetCoordinates() const;
	geo::Coordinates& GetCoordinates();
	const Distances& GetDistances() const;

private:
	geo::Coordinates coordinates_;
	Distances distance_to_stop_;
};

class TransportCatalogue {
public:
	void AddStop(std::string&& stop_name, StopData&& stop);
	void AddBus(std::string&& bus_name, std::list<std::string>&& bus_path, bool path_is_ring);

	std::pair<StopData, bool> GetStopData(const std::string_view stop_name) const;
	std::pair<const BusData&, bool> GetBusData(const std::string_view bus_name) const;
	std::set<std::string_view> GetBusList(const std::string_view stop_name) const;

	std::unordered_set<std::string>& GetStopNames();
	const std::unordered_set<std::string>& GetStopNames() const;
	std::vector<geo::Coordinates> GetAllStopCoordinatesInBusPathes() const;
	std::set<std::string_view> GetBusNames() const;

	size_t GetStopsCount() const;

	///
	size_t GetDistance(std::string_view stop_from, std::string_view stop_to) const;
	const std::unordered_map<std::string, BusData>& GetBuses() const;


private:
	std::unordered_set<std::string> stop_names_;
	std::unordered_map<std::string_view, StopData> stops_;
	std::unordered_map<std::string, BusData> buses_;
	std::unordered_set<std::string_view> stop_names_in_buses_path_;
};

} // namespace transport

