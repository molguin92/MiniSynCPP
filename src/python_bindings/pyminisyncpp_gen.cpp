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

class Algorithm
{
protected:
    std::shared_ptr<MiniSync::API::Algorithm> algo;

public:
    virtual ~Algorithm() = default;

    virtual void addDataPoint(long double To, long double Tb, long double Tr)
    {
        algo->addDataPoint(
            MiniSync::us_t{To},
            MiniSync::us_t{Tb},
            MiniSync::us_t{Tr});
    }

    virtual long double getDrift()
    {
        return algo->getDrift();
    }

    virtual long double getDriftError()
    {
        return algo->getDriftError();
    }

    virtual long double getOffset()
    {
        return algo->getOffset().count();
    }

    virtual long double getOffsetError()
    {
        return algo->getOffsetError().count();
    }
};

class MiniSyncAlgorithm : public Algorithm
{
public:
    MiniSyncAlgorithm() : Algorithm()
    {
        this->algo = std::move(MiniSync::API::Factory::createMiniSync());
    }

    ~MiniSyncAlgorithm() override = default;
};

class TinySyncAlgorithm : public Algorithm
{
public:
    TinySyncAlgorithm() : Algorithm()
    {
        this->algo = std::move(MiniSync::API::Factory::createTinySync());
    }

    ~TinySyncAlgorithm() override = default;
};

// Trampoline class template
template<class AlgorithmBase = Algorithm>
class PyAlgorithm : public AlgorithmBase
{
public:
    using AlgorithmBase::AlgorithmBase; //inherit constructors
    void addDataPoint(long double To, long double Tb, long double Tr) override
    {
        PYBIND11_OVERLOAD(void, AlgorithmBase, addDataPoint, To, Tb, Tr);
    }

    long double getDrift() override
    {
        PYBIND11_OVERLOAD(long double, AlgorithmBase, getDrift,);
    }

    long double getDriftError() override
    {
        PYBIND11_OVERLOAD(long double, AlgorithmBase, getDriftError,);
    }

    long double getOffset() override
    {
        PYBIND11_OVERLOAD(long double, AlgorithmBase, getOffset,);
    }

    long double getOffsetError() override
    {
        PYBIND11_OVERLOAD(long double, AlgorithmBase, getOffsetError,);
    }

};

// Python bindings:
namespace py = pybind11;
PYBIND11_MODULE(pyminisyncpp, m)
{
    // documentation
    m.doc() = "PyMiniSynCPP: Python bindings for the libminisyncpp C++ library.\n";

    py::class_<Algorithm, PyAlgorithm<>> pyAlgo(m, "Algorithm");
    py::class_<TinySyncAlgorithm, PyAlgorithm<TinySyncAlgorithm>> pyTiny(m, "TinySyncAlgorithm");
    py::class_<MiniSyncAlgorithm, PyAlgorithm<MiniSyncAlgorithm>> pyMini(m, "MiniSyncAlgorithm");

    // method definitions
    pyAlgo
        .def(py::init())
        .def("addDataPoint", &Algorithm::addDataPoint)
        .def("getOffset", &Algorithm::getOffset)
        .def("getOffsetError", &Algorithm::getOffsetError)
        .def("getDrift", &Algorithm::getDrift)
        .def("getDriftError", &Algorithm::getDriftError);

    pyTiny
        .def(py::init())
        .def("addDataPoint", &TinySyncAlgorithm::addDataPoint)
        .def("getOffset", &TinySyncAlgorithm::getOffset)
        .def("getOffsetError", &TinySyncAlgorithm::getOffsetError)
        .def("getDrift", &TinySyncAlgorithm::getDrift)
        .def("getDriftError", &TinySyncAlgorithm::getDriftError);

    pyMini
        .def(py::init())
        .def("addDataPoint", &MiniSyncAlgorithm::addDataPoint)
        .def("getOffset", &MiniSyncAlgorithm::getOffset)
        .def("getOffsetError", &MiniSyncAlgorithm::getOffsetError)
        .def("getDrift", &MiniSyncAlgorithm::getDrift)
        .def("getDriftError", &MiniSyncAlgorithm::getDriftError);
}


#endif //LIBMINISYNCPP_PYMINISYNCPP_GEN_CPP
