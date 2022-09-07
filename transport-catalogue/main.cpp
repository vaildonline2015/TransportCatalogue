#include "transport_catalogue.h"
#include "json_reader.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "serialization.h"
#include "transport_router.h"

#include <transport_catalogue.pb.h>
#include <fstream>
#include <iostream>
#include <string_view>
#include <filesystem>
#include <optional>

using namespace std;

void PrintUsage(std::ostream& stream = std::cerr) {
	stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {

	if (argc != 2) {
		PrintUsage();
		return 1;
	}
	const std::string_view mode(argv[1]);

	filesystem::path serialize_result_path;

	if (mode == "make_base"sv) {

		serialize::TransportCatalogue serialize_transport;
		ReadInput(cin, serialize_transport, serialize_result_path);
		
		if (!SerializeTransportCatalogue(serialize_transport, serialize_result_path)) {

			std::cerr << "Serialization error\n";
			return 1;
		}
	} 
	else if (mode == "process_requests"sv) {

		Requests requests;
		ReadInput(cin, requests, serialize_result_path);
		
		transport::TransportCatalogue transport;
		InputAttrs attrs;
		optional<routing::TransportRouter> router;

		if (!DeserializeTransportCatalogue(serialize_result_path, transport, attrs, router, requests.IsRouteRequestPresence())) {

			std::cerr << "Deserialization error\n";
			return 1;
		}
		RequestHandler{ transport, requests, attrs, router }.ProcessRequests(cout);
	} 
	else {
		PrintUsage();
		return 1;
	}
}