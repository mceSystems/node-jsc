/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "NodeIOS.h"

// We could include node's src/node.h, but we only need the "Start" function
namespace node {
    int Start(int argc, char *argv[]);
}

int node_start(int argc, char *argv[]) {
    return node::Start(argc, argv);
}
