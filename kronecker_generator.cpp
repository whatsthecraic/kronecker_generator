/*
 * kronecker_generator.cpp
 *
 *  Created on: 23 May 2019
 *      Author: Dean De Leo
 */

#include <cstdint>
#include <cstdlib> // abort
#include <fstream>
#include <iostream>
#include <getopt.h> // getopt_long
#include <strings.h> // strcasecmp

#include "third-party/graph500_generator/graph_generator.h"
#include "third-party/graph500_generator/utils.h"

using namespace std;

/**
 * The scale of the graph
 *
 */
int po_scale;
const char* po_path_output;

static void print_help(const char* program_name){
    cout << "Generate a Kronecker graph according to the Graph500 specification v3\n";
    cout << "Usage: " << program_name << " [options] <scale> [output.wel]\n";
    cout << "Program options:\n";
    cout << "-h --help     : display the help menu\n\n";
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
            {"help", no_argument, nullptr, 'h'},
            {0, 0, 0, 0} // keep at the end
    };
    int option_index = -1;
    while((getopt_rc = getopt_long(argc, argv, "hv", long_options, &option_index)) != -1){
        switch(getopt_rc){
        case 'h':
            print_help(argv[0]);
            exit(EXIT_SUCCESS);
        default:
            cerr << "ERROR: invalid return code: " << getopt_rc << endl;
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
}

/**
 * Kronecker graph generator
 */
int main(int argc, char* argv[]){
    parse_program_options(argc, argv);
    cout << "Scale: " << po_scale << ", output: " << po_path_output << "\n";

    cout << "Generating the graph..." << endl;

    // as in make_graph(int log_numverts, int64_t M, uint64_t userseed1, uint64_t userseed2, int64_t* nedges_ptr_in, packed_edge** result_ptr_in)
    int64_t num_edges = 16ull << po_scale;
    packed_edge* edges = (packed_edge*) xmalloc(num_edges * sizeof(packed_edge));
    float* weights = (float*) xmalloc(num_edges * sizeof(float));
    uint_fast32_t seeds[5]; make_mrg_seed(2, 3, seeds);
    generate_kronecker_range(seeds, po_scale, 0, num_edges, edges, weights);

    cout << "Writing the graph in `" << po_path_output << "' ..." << endl;
    fstream f(po_path_output, ios_base::out);
    if(!f.good()) {
        cerr << "Cannot open the file " << po_path_output << endl;
        abort();
    }
    for(int64_t i = 0; i < num_edges; i++){
        f << get_v0_from_edge(edges + i) << " " << get_v1_from_edge(edges + i) << " " << weights[i] << "\n";

        if(!f.good()){
            cerr << "Error writing in " << po_path_output << endl;
            abort();
        }
    }
    f.close();

    free(weights); weights = nullptr;
    free(edges); edges = nullptr;
    cout << "Done\n";
    return 0;
}

