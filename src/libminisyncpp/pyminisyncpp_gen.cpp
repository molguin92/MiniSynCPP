/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
*
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#ifndef LIBMINISYNCPP_PYMINISYNCPP_GEN_CPP
#define LIBMINISYNCPP_PYMINISYNCPP_GEN_CPP

#include <pybind11/pybind11.h>
#include <minisync_api.h>

// Python bindings:
namespace py = pybind11;
PYBIND11_MODULE(pyminisyncpp, m)
{
    // documentation
    m.doc() =
        "PyMiniSynCPP: Python bindings for the libminisyncpp C++ library.\n"
        "See the C++ library documentation for explanation of the API.";

    // Factory bindings
    m.def("create_tinysync",
          &MiniSync::API::Factory::createTinySync,
          "Creates a TinySync algorithm object.");
}


#endif //LIBMINISYNCPP_PYMINISYNCPP_GEN_CPP
