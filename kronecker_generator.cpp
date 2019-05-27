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

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib> // abort
#include <cstring>
#include <fstream>
#include <getopt.h> // getopt_long
#include <iostream>
#include <memory>
#include <limits>
#include <strings.h> // strcasecmp

#include "third-party/graph500_generator/graph_generator.h"
#include "third-party/graph500_generator/utils.h"

#include "csr_representation.hpp"

using namespace std;

enum class OutputGraphType {
    PLAIN, // each line is an edge with the form: src dst weight
    METIS, // the format specified the user manual of the METIS Graph Partitiones v5
};

/**
 * Input parameters
 * po_ = program options
 */
uint64_t po_edgefactor = 16; // avg num. of edges per vertex
bool po_int32 = false; // convert the weights into 4 byte signer integers
OutputGraphType po_output_type = OutputGraphType::PLAIN; // the format the graph is serialised
const char* po_path_output; // where to store the produced graph
int po_scale; // scale of the graph

// Function prototypes
static void save_plain(uint64_t num_edges, packed_edge* edges, float* weights);
static void print_help(const char* program_name);
static void parse_program_options(int argc, char* argv[]);


/**
 * Kronecker graph generator
 */
int main(int argc, char* argv[]){
    parse_program_options(argc, argv);
    cout << "Scale: " << po_scale << ", edge factor: " << po_edgefactor << ", output: " << po_path_output << "\n";

    cout << "Generating the graph..." << endl;

    // as in make_graph(int log_numverts, int64_t M, uint64_t userseed1, uint64_t userseed2, int64_t* nedges_ptr_in, packed_edge** result_ptr_in)
    int64_t num_edges = po_edgefactor << po_scale;
    packed_edge* edges = (packed_edge*) xmalloc(num_edges * sizeof(packed_edge));
    float* weights = (float*) xmalloc(num_edges * sizeof(float));
    uint_fast32_t seeds[5]; make_mrg_seed(2, 3, seeds);
    generate_kronecker_range(seeds, po_scale, 0, num_edges, edges, weights);

    // serialise the graph format
    switch(po_output_type){
    case OutputGraphType::PLAIN:
        save_plain(num_edges, edges, weights);
        break;
    case OutputGraphType::METIS: {
        CsrRepresentation csr {(uint64_t) num_edges, edges, weights};
        csr.save_metis(po_path_output, po_int32);
    } break;
    default:
        cerr << "Invalid graph type: " << (int) po_output_type << endl;
        abort();
    }

    free(weights); weights = nullptr;
    free(edges); edges = nullptr;
    cout << "Done\n";
    return 0;
}

static void print_help(const char* program_name){
    cout << "Generate a Kronecker graph according to the Graph500 specification v3\n";
    cout << "Usage: " << program_name << " [options] <scale> [output.wel]\n";
    cout << "Program options:\n";
    cout << "-e --edgefactor : avg. num. edges per vertex (def. 16)\n";
    cout << "-h --help       : display the help menu\n";
    cout << "--int32         : convert the weights into ints\n\n";
    cout << "The program generates a graph with |V| = 2^scale vertices and |E| = 16 * |V|. The output is an edge list in the format: \n";
    cout << "vertex_1 vertex_2 weight\n";
    cout << "where the weight is a double in [0, 1), generated according to a uniform distribution.\n\n";
    cout << "Graph500 scales:\n";
    cout << "* toy: 26\n" <<
            "* mini: 29\n" <<
            "* small: 32\n" <<
            "* medium: 36\n" <<
            "* large: 39\n" <<
            "* huge: 42\n";
}

static void parse_program_options(int argc, char* argv[]){
    int getopt_rc = 0;
    struct option long_options[] = {
            /* name, has_arg in (no_argument, required_argument and optional_argument), flag = nullptr, returned value */
            {"edgefactor", required_argument, nullptr, 'e'},
            {"help", no_argument, nullptr, 'h'},
            {"int32", no_argument, nullptr, 'i'},
            {0, 0, 0, 0} // keep at the end
    };
    int option_index = -1;
    while((getopt_rc = getopt_long(argc, argv, "e:hv", long_options, &option_index)) != -1){
        switch(getopt_rc){
        case 'e':{
            int user_edge_factor = atoi(optarg);
            if(user_edge_factor <= 0){
                cerr << "ERROR: Invalid value for the edge factor: " << optarg << endl;
                abort();
            }
            po_edgefactor = user_edge_factor;
        } break;
        case 'h':
            print_help(argv[0]);
            exit(EXIT_SUCCESS);
        case 'i':
            po_int32 = true;
            break;
        default:
            cerr << "ERROR: Invalid argument: ";
            if(optind >= 0){
                cerr << argv[optind];
            } else {
                cerr << optind;
            }
            cerr << "\n";
            abort();
        }
    };

    // mandatory arguments not given
    if(optind >= argc){
        print_help(argv[0]);
        exit(EXIT_SUCCESS);
    }

    // read the graph scale (=> total number of vertices is 2^{scale}
    po_scale = atoi(argv[optind]);
    if(po_scale <= 0){
        if(strcasecmp(argv[optind], "toy") == 0){
            po_scale = 26;
        } else if(strcasecmp(argv[optind], "mini") == 0){
            po_scale = 29;
        } else if(strcasecmp(argv[optind], "small") == 0){
            po_scale = 32;
        } else if(strcasecmp(argv[optind], "medium") == 0){
            po_scale = 36;
        } else if(strcasecmp(argv[optind], "large") == 0){
            po_scale = 39;
        } else if(strcasecmp(argv[optind], "huge") == 0){
            po_scale = 42;
        } else {
            cerr << "--> ERROR, invalid first argument: " << argv[optind] << ", expected the scale.\n" <<
                    "--> See " << argv[0] << " -h for the proper usage\n";
            exit(EXIT_FAILURE);
        }
    }

    if(optind +1 >= argc){ // default
        po_path_output = "output.wel";
    } else {
        po_path_output = argv[optind+1];
    }


    // check the output extension
    char* file_ext = strrchr(po_path_output, '.');
    if(file_ext != nullptr){
        file_ext++; // skip the dot
        if(strcmp(file_ext, "graph") == 0 || strcmp(file_ext, "metis") == 0){
            po_output_type = OutputGraphType::METIS;
        }
    }

}


static void save_plain(uint64_t num_edges, packed_edge* edges, float* weights){
    cout << "[save_plain] Writing the graph in `" << po_path_output << "' ..." << endl;
    fstream f(po_path_output, ios_base::out);
    if(!f.good()) {
        cerr << "Cannot open the file " << po_path_output << endl;
        abort();
    }
    for(int64_t i = 0; i < num_edges; i++){
        f << get_v0_from_edge(edges + i) << " " << get_v1_from_edge(edges + i) << " ";
        if(po_int32){
            f << static_cast<int32_t>(static_cast<double>(weights[i]) * numeric_limits<int32_t>::max()) / 1024;
        } else {
            f << weights[i];
        }
        f << "\n";

        if(!f.good()){
            cerr << "Error writing in " << po_path_output << endl;
            abort();
        }
    }
    f.close();
}

