#include "bind_dd.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/dd/dd.h"
#include "algorithms/dd/mining_algorithms.h"
#include "py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {
void BindDD(py::module_& main_module) {
    using namespace algos;

    auto dd_module = main_module.def_submodule("dd");
    py::class_<model::DDString>(dd_module, "DD")
            .def("__str__", &model::DDString::ToString)
            .def("__repr__", &model::DDString::ToString);

    BindPrimitive<dd::Split>(dd_module, &dd::DDAlgorithm::DDList, "DdAlgorithm", "get_dds",
                             {"Split"});
}
}  // namespace python_bindings
