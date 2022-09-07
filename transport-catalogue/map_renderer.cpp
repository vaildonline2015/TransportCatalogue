#include "map_renderer.h"
#include "transport_catalogue.h"
#include "svg.h"
#include "geo.h"

#include <iostream>
#include <map>
#include <string>
#include <string_view>

using namespace transport;
using namespace std;

namespace renderer {

namespace {

inline const double EPSILON = 1e-6;

bool IsZero(double value) {
	return std::abs(value) < EPSILON;
}

class SphereProjector {
public:
	// points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
	template <typename PointInputIt>
	SphereProjector(PointInputIt points_begin, PointInputIt points_end,
		double max_width, double max_height, double padding)
		: padding_(padding) //
	{
		// Если точки поверхности сферы не заданы, вычислять нечего
		if (points_begin == points_end) {
			return;
		}

		// Находим точки с минимальной и максимальной долготой
		const auto [left_it, right_it] = std::minmax_element(
			points_begin, points_end,
			[](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
		min_lon_ = left_it->lng;
		const double max_lon = right_it->lng;

		// Находим точки с минимальной и максимальной широтой
		const auto [bottom_it, top_it] = std::minmax_element(
			points_begin, points_end,
			[](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
		const double min_lat = bottom_it->lat;
		max_lat_ = top_it->lat;

		// Вычисляем коэффициент масштабирования вдоль координаты x
		std::optional<double> width_zoom;
		if (!IsZero(max_lon - min_lon_)) {
			width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
		}

		// Вычисляем коэффициент масштабирования вдоль координаты y
		std::optional<double> height_zoom;
		if (!IsZero(max_lat_ - min_lat)) {
			height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
		}

		if (width_zoom && height_zoom) {
			// Коэффициенты масштабирования по ширине и высоте ненулевые,
			// берём минимальный из них
			zoom_coeff_ = std::min(*width_zoom, *height_zoom);
		}
		else if (width_zoom) {
			// Коэффициент масштабирования по ширине ненулевой, используем его
			zoom_coeff_ = *width_zoom;
		}
		else if (height_zoom) {
			// Коэффициент масштабирования по высоте ненулевой, используем его
			zoom_coeff_ = *height_zoom;
		}
	}

	// Проецирует широту и долготу в координаты внутри SVG-изображения
	svg::Point operator()(geo::Coordinates coords) const;

private:
	double padding_;
	double min_lon_ = 0;
	double max_lat_ = 0;
	double zoom_coeff_ = 0;
};

struct StopAttrs {
	svg::Point point;
	svg::Color fillColor;
};

} // namespace

MapRenderer::MapRenderer(const transport::TransportCatalogue& transport_catalogue, const Attrs& render_attrs) 
						: transport_catalogue_(transport_catalogue), render_attrs_(render_attrs) {}

void MapRenderer::Render(std::ostream& os) {

	set<string_view> bus_names = transport_catalogue_.GetBusNames();

	stop_points_ = RenderBusPath(bus_names);
	RenderBusNames(bus_names);
	RenderStopSymbols();
	RenderStopNames();

	document_.Render(os);
}


map<string_view, svg::Point> MapRenderer::RenderBusPath(const set<string_view>& bus_names) {

	std::vector<geo::Coordinates> geo_coordinates = transport_catalogue_.GetAllStopCoordinatesInBusPathes();
	const SphereProjector proj{
				geo_coordinates.begin(), geo_coordinates.end(),
				render_attrs_.map_width, render_attrs_.map_height, render_attrs_.map_padding
	};

	size_t index_color = 0;
	map<string_view, svg::Point> stop_points;

	for (const std::string_view& bus_name : bus_names) {

		svg::Polyline polyline;
		polyline.SetFillColor(svg::NoneColor).SetStrokeWidth(render_attrs_.line_width);
		polyline.SetStrokeLineCap(svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
		polyline.SetStrokeColor(render_attrs_.stroke_colors[index_color % render_attrs_.stroke_colors.size()]);

		auto& bus_data = transport_catalogue_.GetBusData(bus_name).first;
		auto& bus_path = bus_data.GetPath();

		if (bus_path.empty()) {
			continue;
		}
		++index_color;

		vector<svg::Point> path_points;

		for (string_view stop_name : bus_path) {
			geo::Coordinates geo_coordinates = transport_catalogue_.GetStopData(stop_name).first.GetCoordinates();
			const svg::Point screen_coordinate = proj(geo_coordinates);
			polyline.AddPoint(screen_coordinate);
			path_points.push_back(screen_coordinate);
			stop_points[stop_name] = screen_coordinate;
		}

		if (!bus_data.IsRing()) {
			for (auto it = path_points.rbegin() + 1; it != path_points.rend(); ++it) {
				polyline.AddPoint(*it);
			}
		}
		document_.Add(polyline);
	}
	return stop_points;
}

void MapRenderer::RenderBusNames(const set<string_view>& bus_names) {

	svg::Text background;
	background.SetOffset(render_attrs_.bus_label_offset).SetFontSize(render_attrs_.bus_label_font_size);
	background.SetFontFamily("Verdana"s).SetFontWeight("bold"s);
	background.SetFillColor(render_attrs_.underlayer_color).SetStrokeColor(render_attrs_.underlayer_color);
	background.SetStrokeWidth(render_attrs_.underlayer_width);
	background.SetStrokeLineCap(svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

	svg::Text text;
	text.SetOffset(render_attrs_.bus_label_offset).SetFontSize(render_attrs_.bus_label_font_size);
	text.SetFontFamily("Verdana"s).SetFontWeight("bold"s);

	size_t index_color = 0;

	for (const std::string_view& bus_name : bus_names) {

		text.SetFillColor(render_attrs_.stroke_colors[index_color % render_attrs_.stroke_colors.size()]);

		auto& bus_data = transport_catalogue_.GetBusData(bus_name).first;
		auto& bus_path = bus_data.GetPath();

		if (bus_path.empty()) {
			continue;
		}
		++index_color;

		text.SetData(string(bus_name));
		text.SetPosition(stop_points_.at(bus_path.front()));

		background.SetData(string(bus_name));
		background.SetPosition(stop_points_.at(bus_path.front()));

		document_.Add(background);
		document_.Add(text);

		if (!bus_data.IsRing() && bus_path.front() != bus_path.back()) {

			text.SetPosition(stop_points_.at(bus_path.back()));
			background.SetPosition(stop_points_.at(bus_path.back()));

			document_.Add(background);
			document_.Add(text);
		}
	}
}

void MapRenderer::RenderStopSymbols() {
	svg::Circle circle;

	for (auto& [_, point] : stop_points_) {

		circle.SetCenter(point);
		circle.SetRadius(render_attrs_.stop_radius);
		circle.SetFillColor("white"s);

		document_.Add(circle);
	}
}

void MapRenderer::RenderStopNames() {

	svg::Text background;
	background.SetOffset(render_attrs_.stop_label_offset).SetFontSize(render_attrs_.stop_label_font_size);
	background.SetFontFamily("Verdana"s);
	background.SetFillColor(render_attrs_.underlayer_color).SetStrokeColor(render_attrs_.underlayer_color);
	background.SetStrokeWidth(render_attrs_.underlayer_width);
	background.SetStrokeLineCap(svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

	svg::Text text;
	text.SetOffset(render_attrs_.stop_label_offset).SetFontSize(render_attrs_.stop_label_font_size);
	text.SetFontFamily("Verdana"s);
	text.SetFillColor("black"s);

	for (auto& [stop_name, point] : stop_points_) {

		background.SetData(string(stop_name));
		background.SetPosition(point);

		text.SetData(string(stop_name));
		text.SetPosition(point);

		document_.Add(background);
		document_.Add(text);
	}
}

namespace {

svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
	return {
		(coords.lng - min_lon_) * zoom_coeff_ + padding_,
		(max_lat_ - coords.lat) * zoom_coeff_ + padding_
	};
}

} // namespace

} // namespace renderer