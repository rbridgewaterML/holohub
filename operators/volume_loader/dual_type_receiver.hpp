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

#ifndef VOLUME_LOADER_DUAL_TYPE_RECEIVER
#define VOLUME_LOADER_DUAL_TYPE_RECEIVER

#include <pybind11/stl.h>
#include "gil_guarded_pyobject.hpp"

namespace holoscan::ops {

template <typename T>
struct is_shared_ptr : std::false_type {};
template <typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};

/*
 * Any object emitted from a Python operator on a port (except Tensor) is tracked by the
 * GILGuardedObject. With that we can detect if the object received by the C++ operator
 * is coming from Python and do the conversion to a C++ type.
 * This needs to be moved to Holoscan SDK.
 * See discussion https://nvidia.slack.com/archives/C03E643SK89/p1701875573021209.
 */
template <typename PTYPE, typename CTYPE>
void dual_type_converter(std::any& value, CTYPE& result) {
  if (value.type() == typeid(CTYPE)) {
    result = std::any_cast<CTYPE>(value);
  } else if (value.type() == typeid(std::shared_ptr<holoscan::GILGuardedPyObject>)) {
    auto py_object = std::any_cast<std::shared_ptr<holoscan::GILGuardedPyObject>>(value);
    if (py::isinstance<PTYPE>(py_object->obj())) {
      py::gil_scoped_acquire gil;
      if constexpr (is_shared_ptr<CTYPE>::value) {
        result.reset(new typename CTYPE::element_type);
        *(result.get()) =
            py::cast<typename CTYPE::element_type>(static_cast<PTYPE&>(py_object->obj()));
      } else {
        result = py::cast<CTYPE>(static_cast<PTYPE&>(py_object->obj()));
      }
    } else {
      throw std::runtime_error(fmt::format("VolumeLoaderOp: Unhandled type"));
    }
  } else {
    throw std::runtime_error(fmt::format("VolumeLoaderOp: Unhandled type"));
  }
}

template <typename PTYPE, typename CTYPE>
void dual_type_receiver(InputContext& input, const std::string& port_name, CTYPE& result) {
  auto maybe_value = input.receive<std::any>(port_name.c_str());
  if (maybe_value) { dual_type_converter<PTYPE, CTYPE>(maybe_value.value(), result); }
}

}  // namespace holoscan::ops

#endif /* VOLUME_LOADER_DUAL_TYPE_RECEIVER */
