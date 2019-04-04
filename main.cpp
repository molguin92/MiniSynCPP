/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
*
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#include <iostream>
#include "CLI11.hpp"

int main(int argc, char* argv[])
{
    bool reference_mode = false;
    std::string peer = "0.0.0.0";
    uint32_t port = 1337;

    CLI::App app{"Standalone demo implementation of the Tiny/MiniSync time synchronization algorithms."};
    app.add_flag("-r,--ref", reference_mode, "Start node in reference mode; "
                                             "i.e. other peers synchronize to this node's clock.");
    app.add_option<std::string>("ADDRESS", peer, "Address of peer to synchronize with.", true);
    app.add_option<uint32_t>("PORT", port, "UDP Port on peer to communicate with.", true);

    CLI11_PARSE(app, argc, argv);
    return 0;
}
