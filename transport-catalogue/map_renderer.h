#pragma once

#include "transport_catalogue.h"
#include "svg.h"
#include "json.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>
#include <string_view>

namespace renderer {

struct Attrs {
	std::vector<svg::Color> stroke_colors;
	double line_width;
	double map_width;
	double map_height;
	double map_padding;
	svg::Point bus_label_offset;
	uint32_t bus_label_font_size;
	svg::Color underlayer_color;
	double underlayer_width;
	double stop_radius;
	svg::Point stop_label_offset;
	uint32_t stop_label_font_size;
};

class MapRenderer {
public:
	MapRenderer(const transport::TransportCatalogue& transport_catalogue, const Attrs& render_attrs);

	void Render(std::ostream& os);

private:
	std::map<std::string_view, svg::Point> RenderBusPath(const std::set<std::string_view>& bus_names);
	void RenderBusNames(const std::set<std::string_view>& bus_names);
	void RenderStopSymbols();
	void RenderStopNames();

	const transport::TransportCatalogue& transport_catalogue_; 
	const Attrs& render_attrs_;
	svg::Document document_;
	std::map<std::string_view, svg::Point> stop_points_;
};

} // namespace renderer



