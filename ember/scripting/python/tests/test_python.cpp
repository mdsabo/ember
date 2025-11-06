#include <catch2/catch_test_macros.hpp>

#include <Python.h>

TEST_CASE("Python simple script", "[Python]") {
    Py_Initialize();
    PyRun_SimpleString("assert (3+4) == 7\n");
    Py_Finalize();
}
