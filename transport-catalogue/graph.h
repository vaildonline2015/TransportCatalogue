#pragma once

#include "ranges.h"

#include <cstdlib>
#include <vector>

namespace graph {

using VertexId = size_t;
using EdgeId = size_t;

template <typename Weight>
struct Edge {
	VertexId from;
	VertexId to;
	Weight weight;
};

template <typename Weight>
class DirectedWeightedGraph {
private:
	using EdgesIdList = std::vector<EdgeId>;
	using IncidentEdgesRange = ranges::Range<typename EdgesIdList::const_iterator>;

public:
	DirectedWeightedGraph() = default;
	explicit DirectedWeightedGraph(size_t vertex_count);
	EdgeId AddEdge(const Edge<Weight>& edge);

	size_t GetVertexCount() const;
	size_t GetEdgeCount() const;
	const Edge<Weight>& GetEdge(EdgeId edge_id) const;
	IncidentEdgesRange GetIncidentEdges(VertexId vertex) const;

	const std::vector<Edge<Weight>>& GetEdges() const;
private:
	std::vector<Edge<Weight>> edges_;
	std::vector<EdgesIdList> vertex_to_edges_id_;
};

template <typename Weight>
const std::vector<Edge<Weight>>& DirectedWeightedGraph<Weight>::GetEdges() const {

	return edges_;
}

template <typename Weight>
DirectedWeightedGraph<Weight>::DirectedWeightedGraph(size_t vertex_count)
	: vertex_to_edges_id_(vertex_count) {
}

template <typename Weight>
EdgeId DirectedWeightedGraph<Weight>::AddEdge(const Edge<Weight>& edge) {
	edges_.push_back(edge);
	const EdgeId id = edges_.size() - 1;
	vertex_to_edges_id_.at(edge.from).push_back(id);
	return id;
}

template <typename Weight>
size_t DirectedWeightedGraph<Weight>::GetVertexCount() const {
	return vertex_to_edges_id_.size();
}

template <typename Weight>
size_t DirectedWeightedGraph<Weight>::GetEdgeCount() const {
	return edges_.size();
}

template <typename Weight>
const Edge<Weight>& DirectedWeightedGraph<Weight>::GetEdge(EdgeId edge_id) const {
	return edges_.at(edge_id);
}

template <typename Weight>
typename DirectedWeightedGraph<Weight>::IncidentEdgesRange
DirectedWeightedGraph<Weight>::GetIncidentEdges(VertexId vertex) const {
	return ranges::AsRange(vertex_to_edges_id_.at(vertex));
}
}  // namespace graph