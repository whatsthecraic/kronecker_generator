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

#include "csr_representation.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>

using namespace std;

/*********************************************************************************************************************
 *                                                                                                                   *
 *  Initialisation                                                                                                   *
 *                                                                                                                   *
 *********************************************************************************************************************/
static void convert2csr(uint64_t num_edges, packed_edge* edges, float* weights, uint64_t* out_num_vertices, uint64_t** out_csr_vertices, uint64_t** out_csr_edges, float** out_csr_weights){
    if(edges == nullptr) { throw std::invalid_argument("[convert2csr] edges is nullptr"); }
    if(weights == nullptr) { throw std::invalid_argument("[convert2csr] weights is nullptr"); }
    if(out_num_vertices == nullptr) { throw std::invalid_argument("[convert2csr] out_num_vertices is nullptr"); }
    if(out_csr_vertices == nullptr) { throw std::invalid_argument("[convert2csr] out_csr_vertices is nullptr"); }
    if(out_csr_edges == nullptr) { throw std::invalid_argument("[convert2csr] out_csr_edges is nullptr"); }
    if(out_csr_weights == nullptr) { throw std::invalid_argument("[convert2csr] out_csr_weights is nullptr"); }
    if(*out_csr_vertices != nullptr) { throw std::invalid_argument("[convert2csr] *out_csr_vertices expected nullptr"); }
    if(*out_csr_edges != nullptr) { throw std::invalid_argument("[convert2csr] *out_csr_edges expected nullptr"); }
    if(*out_csr_weights != nullptr) { throw std::invalid_argument("[convert2csr] *out_csr_weights expected nullptr"); }
    cout << "[convert2csr] Converting to the CSR representation..." << endl;

    // find the maximum vertex id
    uint64_t max_vertex_id = 0;
    for(uint64_t i = 0; i < num_edges; i++){
        max_vertex_id = max<uint64_t>(max_vertex_id, max(get_v0_from_edge(edges +i), get_v1_from_edge(edges +i)));
    }
    cout << "[convert2csr] Max vertex ID: " << max_vertex_id << "\n";
    uint64_t num_vertices = max_vertex_id +1;

    // allocate the output arrays
    auto fn_free = [](void* ptr){ free(ptr); };
    unique_ptr<uint64_t, decltype(fn_free)> ptr_csr_vertices{ (uint64_t*) calloc(sizeof(uint64_t), num_vertices), fn_free };
    if(ptr_csr_vertices.get() == nullptr) { cerr << "[convert2csr] Cannot allocate an array to store " << (num_vertices) << " vertices"; throw std::bad_alloc(); }
    unique_ptr<uint64_t, decltype(fn_free)> ptr_temp_vertex_ids{ (uint64_t*) calloc(sizeof(uint64_t), num_vertices), fn_free };
    if(ptr_temp_vertex_ids.get() == nullptr) { cerr << "[convert2csr] Cannot allocate a temporary array to store " << (num_vertices) << " vertices"; throw std::bad_alloc(); }
    unique_ptr<uint64_t, decltype(fn_free)> ptr_csr_edges{ (uint64_t*) calloc(sizeof(uint64_t), num_edges *2), fn_free };
    if(ptr_csr_edges.get() == nullptr) { cerr << "[convert2csr] Cannot allocate an array to store " << (num_edges *2) << " edges"; throw std::bad_alloc(); }
    unique_ptr<float, decltype(fn_free)> ptr_csr_weights{ (float*) calloc(sizeof(float), num_edges *2), fn_free };
    if(ptr_csr_weights.get() == nullptr) { cerr << "[convert2csr] Cannot allocate an array to store " << (num_edges *2) << " weights"; throw std::bad_alloc(); }
    uint64_t* __restrict csr_vertices = ptr_csr_vertices.get();
    uint64_t* __restrict tmp_indices = ptr_temp_vertex_ids.get();
    uint64_t* __restrict csr_edges = ptr_csr_edges.get();
    float* __restrict csr_weights = ptr_csr_weights.get();

    // get the number of edges per vertex
    for(uint64_t i = 0; i < num_edges; i++){
        assert(get_v0_from_edge(edges + i) <= max_vertex_id && "ID out of bound");
        csr_vertices[get_v0_from_edge(edges +i)] ++;
        csr_vertices[get_v1_from_edge(edges +i)] ++; // because the graph is undirected!
    }

    // prefix sum
    for(uint64_t i =1; i < num_vertices; i++){
        csr_vertices[i] = csr_vertices[i -1] + csr_vertices[i];
    }

    // populate the arrays edges & weights
    for(uint64_t i = 0; i < num_edges; i++){
        uint64_t src = get_v0_from_edge(edges +i);
        uint64_t dst = get_v1_from_edge(edges + i);
        float weight = weights[i];

        uint64_t src_base = (src == 0) ? 0 : csr_vertices[src -1];
        uint64_t& src_displacement = tmp_indices[src];
        csr_edges[src_base + src_displacement] = dst;
        csr_weights[src_base + src_displacement] = weight;
        src_displacement++;

        // because the input graph is undirected
        uint64_t dst_base = (dst == 0) ? 0 : csr_vertices[dst -1];
        uint64_t& dst_displacement = tmp_indices[dst];
        csr_edges[dst_base + dst_displacement] = src;
        csr_weights[dst_base + dst_displacement] = weight;
        dst_displacement++;
    }

    // return the output to the caller
    *out_num_vertices = num_vertices;
    *out_csr_vertices = csr_vertices; ptr_csr_vertices.release();
    *out_csr_edges = csr_edges; ptr_csr_edges.release();
    *out_csr_weights = csr_weights; ptr_csr_weights.release();
}

CsrRepresentation::CsrRepresentation(uint64_t num_edges, packed_edge* edges, float* weights) {
    convert2csr(num_edges, edges, weights, &m_num_vertices, &m_vertices, &m_edges, &m_weights);
}

CsrRepresentation::~CsrRepresentation(){
    free(m_vertices); m_vertices = nullptr;
    free(m_edges); m_edges = nullptr;
    free(m_weights); m_weights = nullptr;
}


/*********************************************************************************************************************
 *                                                                                                                   *
 *  Properties                                                                                                       *
 *                                                                                                                   *
 *********************************************************************************************************************/
uint64_t CsrRepresentation::num_vertices() const {
    return m_num_vertices;
}

uint64_t CsrRepresentation::num_edges() const {
    return m_vertices[ m_num_vertices -1 ];
}

uint64_t CsrRepresentation::get_vertex_base(uint64_t vertex_id) const {
    if(vertex_id >= num_vertices())
        throw std::out_of_range("invalid vertex id");
    else if(vertex_id == 0)
        return 0;
    else
        return m_vertices[vertex_id -1];
}

uint64_t CsrRepresentation::get_vertex_count(uint64_t vertex_id) const {
    if(vertex_id >= num_vertices())
        throw std::out_of_range("invalid vertex id");
    else if(vertex_id == 0)
        return m_vertices[vertex_id];
    else
        return m_vertices[vertex_id] - m_vertices[vertex_id -1];
}

/*********************************************************************************************************************
 *                                                                                                                   *
 *  METIS                                                                                                            *
 *                                                                                                                   *
 *********************************************************************************************************************/
void CsrRepresentation::save_metis(const char* path, bool weights_as_int32) const {
    const uint64_t num_vertices_ = num_vertices();
    const uint64_t num_edges_ = num_edges();
    assert(num_edges_ % 2 == 0 && "Because the input graph is undirected");

    cout << "[save_metis] Writing the graph to `" << path << "' ..." << endl;
    fstream f(path, ios_base::out);
    if(!f.good()) {
        cerr << "Cannot open the file " << path << endl;
        abort();
    }

    // Header
    f << num_vertices_ << " " << (num_edges_/2) << " 001\n"; // 001 is a special code to signal the edges have weights associated
    if(!f.good()){
        cerr << "Error writing the header: " << path << endl;
        abort();
    }

    // Body
    for(uint64_t vertex_id = 0; vertex_id < num_vertices_; vertex_id++){
        uint64_t edge_base = get_vertex_base(vertex_id);
        for(uint64_t edge_id = 0, num_edges_per_vertex_id = get_vertex_count(vertex_id); edge_id  < num_edges_per_vertex_id; edge_id ++){
            if(edge_id > 0) f << " "; // separate from the previous pair <dst, weight>
            f << (m_edges[edge_base + edge_id] +1) << " "; // +1, because vertices start from 1 in METIS
            if(weights_as_int32){
                f << static_cast<int32_t>(static_cast<double>(m_weights[edge_base + edge_id]) * numeric_limits<int32_t>::max()) / 1024;
            } else {
                f << m_weights[edge_base + edge_id];
            }
        }

        f << "\n";

        if(!f.good()){
            cerr << "Error writing in " << path << endl;
            abort();
        }
    }

    f.close();
}

