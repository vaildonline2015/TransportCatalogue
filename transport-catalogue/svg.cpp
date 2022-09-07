#include "svg.h"
#include <iomanip>
#include <sstream>

namespace svg {

using namespace std;

Rgb::Rgb(unsigned int to_red, unsigned int to_green, unsigned int to_blue) : red(to_red), green(to_green), blue(to_blue) {
}
Rgba::Rgba(unsigned int to_red, unsigned int to_green, 
			unsigned int to_blue, double to_opacity) : 
														red(to_red), green(to_green), blue(to_blue), opacity(to_opacity) {
}

struct ColorToString {
	std::string operator()(std::monostate) const { return "none"s; }
	std::string operator()(std::string str) const { return str; }

	std::string operator()(Rgb rgb) const {
		return "rgb("s + std::to_string(rgb.red) + ","s + std::to_string(rgb.green)
			+ ","s + std::to_string(rgb.blue) + ")"s;
	}

	std::string operator()(Rgba rgba) const {
		ostringstream iss;
		iss << rgba.opacity;
		string opacity = iss.str();
		return "rgba("s + std::to_string(rgba.red) + ","s + std::to_string(rgba.green)
			+ ","s + std::to_string(rgba.blue) + ","s + opacity + ")"s;
	}
};

std::ostream& operator << (std::ostream& os, const Color& color) {
	os << std::visit(ColorToString{}, color);
	return os;
}

std::ostream& operator << (std::ostream& os, const StrokeLineCap& stroke_line_cap)
{
	switch (stroke_line_cap) {
	case StrokeLineCap::BUTT: {
		os << "butt"s;
		break;
	}
	case StrokeLineCap::ROUND: {
		os << "round"s;
		break;
	}
	case StrokeLineCap::SQUARE: {
		os << "square"s;
		break;
	}
	}
	return os;
}
std::ostream& operator << (std::ostream& os, const StrokeLineJoin& stroke_line_join)
{
	switch (stroke_line_join) {
	case StrokeLineJoin::ARCS: {
		os << "arcs"s;
		break;
	}
	case StrokeLineJoin::BEVEL: {
		os << "bevel"s;
		break;
	}
	case StrokeLineJoin::MITER: {
		os << "miter"s;
		break;
	}
	case StrokeLineJoin::MITER_CLIP: {
		os << "miter-clip"s;
		break;
	}
	case StrokeLineJoin::ROUND: {
		os << "round"s;
		break;
	}
	}
	return os;
}

RenderContext::RenderContext(std::ostream& out)
	: out(out) {
}

RenderContext::RenderContext(std::ostream& out, int indent_step, int indent)
	: out(out)
	, indent_step(indent_step)
	, indent(indent) {
}

RenderContext RenderContext::Indented() const {
	return { out, indent_step, indent + indent_step };
}

void RenderContext::RenderIndent() const {
	for (int i = 0; i < indent; ++i) {
		out.put(' ');
	}
}


// --------- Object ---------

void Object::Render(const RenderContext& context) const {
	context.RenderIndent();

	RenderObject(context);

	context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
	center_ = center;
	return *this;
}

Circle& Circle::SetRadius(double radius)  {
	radius_ = radius;
	return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
	auto& out = context.out;
	out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
	out << "r=\""sv << radius_ << "\""sv;
	Attrs(context.out);
	out << "/>"sv;
}

// ---------- Polyline ------------------

Polyline& Polyline::AddPoint(Point point) {
	points_.push_back(move(point));
	return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {

	auto& out = context.out;
	out << "<polyline points=\""sv;
	if (!points_.empty()) {
		auto it = points_.begin();
		while (next(it) != points_.end()) {
			out << it->x << ","sv << it->y << " "sv;
			++it;
		}
		out << it->x << ","sv << it->y << "\""sv;
	}
	else {
		out << "\""sv;
	}
	Attrs(context.out);
	out << "/>"sv;
	
}

// ---------- Text ------------------

Text& Text::SetPosition(Point pos) {
	pos_ = move(pos);
	return *this;
}

Text& Text::SetOffset(Point offset) {
	offset_ = move(offset);
	return *this;
}

Text& Text::SetFontSize(uint32_t size) {
	size_ = size;
	return *this;
}

Text& Text::SetFontFamily(std::string font_family) {
	font_family_ = move(font_family);
	return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
	font_weight_ = move(font_weight);
	return *this;
}

Text& Text::SetData(std::string data) {
	data_ = move(data);
	return *this;
}

void Text::RenderObject(const RenderContext& context) const {
	auto& out = context.out;
	out << "<text x=\""sv << pos_.x << "\" y=\""sv << pos_.y << "\" dx=\""sv << offset_.x <<
		"\" dy=\""sv << offset_.y << "\" font-size=\""sv << size_ << "\"";
	if (!font_family_.empty()) {
		out << " font-family=\""sv << font_family_ << "\"";
	}
	if (!font_weight_.empty()) {
		out << " font-weight=\""sv << font_weight_ << "\"";
	}
	Attrs(context.out);
	out<< ">"sv << data_ << "</text>"sv;
}

// ---------- Document ------------------

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
	objects_.push_back(move(obj));
}

void Document::Render(std::ostream& out) const {
	out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
	out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;

	RenderContext ctx(out, 2, 2);

	for (auto& ptr : objects_) {
		ptr->Render(ctx);
	}

	out << "</svg>"sv;
}

}  // namespace svg