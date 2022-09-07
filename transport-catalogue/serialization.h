#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

#include <transport_catalogue.pb.h>
#include <filesystem>
#include <optional>

struct InputAttrs {
	renderer::Attrs render_attrs;
	routing::Attrs routing_attrs;
};

bool SerializeTransportCatalogue(	serialize::TransportCatalogue& serialize_transport,
									const std::filesystem::path& serialize_result_path);

bool DeserializeTransportCatalogue(std::filesystem::path& serialize_result_path,
									transport::TransportCatalogue& transport,
									InputAttrs& attrs, std::optional<routing::TransportRouter>& router,
									bool is_route_request_presence);