/*
 * SPDX-FileCopyrightText: Copyright (c) 2023-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef A41960E0_8886_44FF_A7F2_FFC680D33515
#define A41960E0_8886_44FF_A7F2_FFC680D33515

#ifndef PYBIND11_CORE_GIL_GUARDED_PYOBJECT_HPP
#define PYBIND11_CORE_GIL_GUARDED_PYOBJECT_HPP

#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace holoscan {

/**
 * @brief A wrapper around pybind11::object class that allows to be destroyed
 * with acquiring the GIL.
 *
 * This class is used in PyInputContext::py_receive() and PyOutputContext::py_emit() methods
 * to allow the Python code (decreasing the reference count) to be executed with the GIL acquired.
 *
 * Without this wrapper, the Python code would be executed without the GIL by the GXF execution
 * engine that destroys the Entity object and executes Message::~Message() and
 * pybind11::object::~object(), which would cause a segfault.
 */
class __attribute__((visibility("hidden"))) GILGuardedPyObject {
 public:
  GILGuardedPyObject() = delete;
  explicit GILGuardedPyObject(const py::object& obj) : obj_(obj) {}
  explicit GILGuardedPyObject(py::object&& obj) : obj_(obj) {}

  py::object& obj() { return obj_; }

  ~GILGuardedPyObject() {
    // Acquire GIL before destroying the PyObject
    py::gil_scoped_acquire scope_guard;
    py::handle handle = obj_.release();
    if (handle) { handle.dec_ref(); }
  }

 private:
  py::object obj_;
};

}  // namespace holoscan

#endif /* PYBIND11_CORE_GIL_GUARDED_PYOBJECT_HPP */

#endif /* A41960E0_8886_44FF_A7F2_FFC680D33515 */
