/**
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstddef>
#include <cstdint>

#include "third-party/graph500_generator/graph_generator.h" // packed_edge

/**
 * A CRS (or CSR) representation of the generated graph. The graph is directed
 */
class CsrRepresentation{
    uint64_t m_num_vertices { 0 };
    uint64_t* m_vertices { nullptr };
    uint64_t* m_edges { nullptr };
    float* m_weights { nullptr };

public:
    // Convert the undirected generated graph into a directed CSR representation
    CsrRepresentation(uint64_t num_edges, packed_edge* edges, float* weights);

    // Destructor
    ~CsrRepresentation();

    // Store the graph to path in the METIS v5 format
    void save_metis(const char* path, bool weights_as_int32 = false) const;

    // The total number of vertices in the graph
    uint64_t num_vertices() const;

    // The total number of edges in the graph
    uint64_t num_edges() const;

    // The base of in the edges array for the given vertex_id
    uint64_t get_vertex_base(uint64_t vertex_id) const;

    // Retrieve the number of outgoing edges for the given vertex_id
    uint64_t get_vertex_count(uint64_t vertex_id) const;
};
