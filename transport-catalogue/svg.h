#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace svg {

using namespace std::literals;

struct Rgb {
	Rgb() = default;
	Rgb(unsigned int to_red, unsigned int to_green, unsigned int to_blue);

	uint8_t red = 0;
	uint8_t green = 0;
	uint8_t blue = 0;
};

struct Rgba {
	Rgba() = default;
	Rgba(unsigned int to_red, unsigned int to_green, unsigned int to_blue, double to_opacity);

	uint8_t red = 0;
	uint8_t green = 0;
	uint8_t blue = 0;
	double opacity = 1.0;
};

using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;
inline const std::monostate NoneColor;

std::ostream& operator << (std::ostream& os, const Color& color);

enum class StrokeLineCap {
	BUTT,
	ROUND,
	SQUARE,
};
enum class StrokeLineJoin {
	ARCS,
	BEVEL,
	MITER,
	MITER_CLIP,
	ROUND,
};

std::ostream& operator << (std::ostream& os, const StrokeLineCap& stroke_line_cap);
std::ostream& operator << (std::ostream& os, const StrokeLineJoin& stroke_line_join);

struct RenderContext {
	RenderContext(std::ostream& out);

	RenderContext(std::ostream& out, int indent_step, int indent = 0);

	RenderContext Indented() const;

	void RenderIndent() const;

	std::ostream& out;
	int indent_step = 0;
	int indent = 0;
};

class Object {
public:
	void Render(const RenderContext& context) const;

	virtual ~Object() = default;

private:
	virtual void RenderObject(const RenderContext& context) const = 0;
};

class ObjectContainer {
public:
	template<typename T>
	void Add(T object) {
		AddPtr(std::make_unique<T>(std::move(object)));
	}

	virtual void AddPtr(std::unique_ptr<Object>&& object) = 0;

	virtual ~ObjectContainer() = default;
};

class Document : public ObjectContainer {
public:

	void AddPtr(std::unique_ptr<Object>&& object) override;

	void Render(std::ostream& out) const;

	virtual ~Document() = default;
private:
	std::vector<std::unique_ptr<Object>> objects_;
};

class Drawable {
public:
	virtual void Draw(ObjectContainer& container) const = 0;

	virtual ~Drawable() = default;
};

template <typename Owner>
class PathProps {
public:
	Owner& SetFillColor(Color color) {
		fill_color_ = std::move(color);
		return AsOwner();
	}
	Owner& SetStrokeColor(Color color) {
		stroke_color_ = std::move(color);
		return AsOwner();
	}
	Owner& SetStrokeWidth(double width) {
		width_ = width;
		return AsOwner();
	}
	Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
		line_cap_ = line_cap;
		return AsOwner();
	}
	Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
		line_join_ = line_join;
		return AsOwner();
	}

protected:
	~PathProps() = default;

	void Attrs(std::ostream& out) const {
		using namespace std::literals;

		if (fill_color_) {
			out << " fill=\""sv << *fill_color_ << "\""sv;
		}
		if (stroke_color_) {
			out << " stroke=\""sv << *stroke_color_ << "\""sv;
		}
		if (width_) {
			out << " stroke-width=\""sv << *width_ << "\""sv;
		}
		if (line_cap_) {
			out << " stroke-linecap=\""sv << *line_cap_ << "\""sv;
		}
		if (line_join_) {
			out << " stroke-linejoin=\""sv << *line_join_ << "\""sv;
		}
	}

private:
	Owner& AsOwner() {
		return static_cast<Owner&>(*this);
	}

	std::optional<Color> fill_color_;
	std::optional<Color> stroke_color_;
	std::optional<double> width_;
	std::optional<StrokeLineCap> line_cap_;
	std::optional<StrokeLineJoin> line_join_;
};

struct Point {
	Point() = default;
	Point(double x, double y)
		: x(x)
		, y(y) {
	}
	double x = 0;
	double y = 0;
};

class Circle final : public Object, public PathProps<Circle> {
public:
	Circle& SetCenter(Point center);
	Circle& SetRadius(double radius);

private:
	void RenderObject(const RenderContext& context) const override;

	Point center_;
	double radius_ = 1.0;
};

class Polyline final : public Object, public PathProps<Polyline> {
public:
	Polyline& AddPoint(Point point);

private:
	void RenderObject(const RenderContext& context) const override;

	std::vector<Point> points_;
};

class Text final : public Object, public PathProps<Text> {
public:

	Text& SetPosition(Point pos);
	Text& SetOffset(Point offset);
	Text& SetFontSize(uint32_t size);
	Text& SetFontFamily(std::string font_family);
	Text& SetFontWeight(std::string font_weight);
	Text& SetData(std::string data);

private:
	void RenderObject(const RenderContext& context) const override;

	Point pos_ = { 0.0, 0.0 };
	Point offset_ = { 0.0, 0.0 };
	uint32_t size_ = 1;
	std::string font_family_;
	std::string font_weight_;
	std::string data_ = ""s;
};

}  // namespace svg