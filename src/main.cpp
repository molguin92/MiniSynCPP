/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
*
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#include <iostream>
#include "CLI11/CLI11.hpp"
#include "loguru/loguru.hpp"
#include "node.h"
#include "exception.h"
#include <memory>
#include "config.h"

MiniSync::Node* node = nullptr;

void sig_handler(int)
{
    if (node != nullptr)
        node->shut_down();
    else exit(0);
}

int main(int argc, char* argv[])
{
    loguru::g_stderr_verbosity = loguru::Verbosity_ERROR; // set default verbosity to ERROR or FATAL
    loguru::init(argc, argv); // parse -v flags

    signal(SIGINT, sig_handler); // override logurus signal handlers

    std::string peer;
    uint16_t port;
    uint16_t bind_port;
    std::string output_file;

    std::ostringstream app_description{};
    app_description
        << "MiniSynCPP v" << APP_VERSION_MAJOR << "." << APP_VERSION_MINOR << ". "
        << "Standalone demo implementation of the Tiny/MiniSync time synchronization algorithms.";

    CLI::App app{app_description.str()};

    auto* ref_mode = app.add_subcommand("REF_MODE", "Start node in reference mode; "
                                                    "i.e. other peers synchronize to this node's clock.");
    ref_mode->add_option<uint16_t>("BIND_PORT", bind_port, "Local UDP port to bind to.")->required(true);
    ref_mode->add_option("-v", loguru::g_stderr_verbosity, "Set verbosity level.", true);

    auto* sync_mode = app.add_subcommand("SYNC_MODE", "Start node in synchronization mode.");

    sync_mode->add_option<uint16_t>("BIND_PORT", bind_port, "Local UDP port to bind to.")->required(true);
    sync_mode->add_option<std::string>("ADDRESS", peer, "Address of peer to synchronize with.")->required(true);
    sync_mode->add_option<uint16_t>("PORT", port, "Target UDP Port on peer.")->required(true);
    sync_mode->add_option("-v", loguru::g_stderr_verbosity, "Set verbosity level.", true);
    sync_mode->add_option("-o,--output", output_file, "Output stats to file.", false);

    app.fallthrough(true);
    app.require_subcommand(1, 1);

    CLI11_PARSE(app, argc, argv)

    auto modes = app.get_subcommands();
    CHECK_EQ_F(modes.size(), 1, "Wrong number of subcommands - THIS SHOULD NEVER HAPPEN?");

    if (modes.front()->get_name() == "REF_MODE")
    {
        // LOG_F(INFO, "Started node in REFERENCE mode.");
        // MiniSync::ReferenceNode node{bind_port};
        node = new MiniSync::ReferenceNode{bind_port};
    }
    else if (modes.front()->get_name() == "SYNC_MODE")
    {
        // LOG_F(INFO, "Started node in SYNCHRONIZATION mode.");

        node = new MiniSync::SyncNode(bind_port, peer, port,
                                      std::unique_ptr<MiniSync::SyncAlgorithm>(new MiniSync::MiniSyncAlgorithm()),
                                      output_file);
    }
    else
        ABORT_F("Invalid mode specified for application - THIS SHOULD NEVER HAPPEN?");

    try
    {
        node->run();
    }
    catch (std::exception& e)
    {
        LOG_F(ERROR, "%s", e.what());
        node->shut_down();
    }

    delete (node);
    return 0;
}

