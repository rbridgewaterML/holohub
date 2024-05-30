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

#include "nrrd_loader.hpp"

#include <array>
#include <filesystem>
#include <string>

#include <zlib.h>

#include "volume.hpp"

namespace holoscan::ops {

bool is_nrrd(const std::string& file_name) {
  std::filesystem::path path(file_name);

  if (path.extension() == ".nhdr" || path.extension() == ".nrrd") { return true; }

  return false;
}

bool load_nrrd(const std::string& file_name, Volume& volume) {
  bool compressed = false;
  std::string data_file_name;
  nvidia::gxf::PrimitiveType primitive_type;
  std::array<int32_t, 3> dims;

  {
    std::stringstream meta_header;
    {
      std::ifstream file;
      file.open(file_name, std::ios::in);
      if (!file.is_open()) {
        holoscan::log_error("NRRD could not open {}", file_name);
        return false;
      }
      meta_header << file.rdbuf();
    }
    // get the parameters
    std::string parameter;
    while (std::getline(meta_header, parameter, ':')) {
      // remove spaces
      parameter.erase(
          std::remove_if(
              parameter.begin(), parameter.end(), [](unsigned char x) { return std::isspace(x); }),
          parameter.end());

      // get the value
      std::string value;
      std::getline(meta_header, value);
      // remove leading spaces
      auto it = value.begin();
      while ((it != value.end()) && (std::isspace(*it))) { it = value.erase(it); }
      // remove trailing spaces
      it = --value.end();
      while ((it != value.begin()) && (std::isspace(*it))) { it = value.erase(it); }

      if (parameter == "dimension") {
        int dims = std::stoi(value);
        if (dims != 3) {
          holoscan::log_error("NRRD expected a three dimensional input, instead NDims is {}", dims);
          return false;
        }
      } else if (parameter == "encoding") {
        if (value == "gz") {
          compressed = true;
        } else if (value == "raw") {
          compressed = false;
        } else {
          holoscan::log_error("NRRD unexpected value for {}: {}", parameter, value);
          return false;
        }
      } else if (parameter == "sizes") {
        std::stringstream value_stream(value);
        std::string value;
        for (int index = 0; std::getline(value_stream, value, ' ') && (index < 3); ++index) {
          dims[2 - index] = std::stoi(value);
        }
      } else if (parameter == "spacings") {
        std::stringstream value_stream(value);
        std::string value;
        for (int index = 0; std::getline(value_stream, value, ' ') && (index < 3); ++index) {
          volume.spacing_[index] = std::stof(value);
        }
      } else if (parameter == "type") {
        if ((value == "signed char") || (value == "int8") || (value == "int8_t")) {
          primitive_type = nvidia::gxf::PrimitiveType::kInt8;
        } else if ((value == "uchar") || (value == "unsigned char") || (value == "uint8") ||
                   (value == "uint8_t")) {
          primitive_type = nvidia::gxf::PrimitiveType::kUnsigned8;
        } else if ((value == "short") || (value == "short int") || (value == "signed short") ||
                   (value == "signed short int") || (value == "int16") || (value == "int16_t")) {
          primitive_type = nvidia::gxf::PrimitiveType::kInt16;
        } else if ((value == "ushort") || (value == "unsigned short") ||
                   (value == "unsigned short int") || (value == "uint16") ||
                   (value == "uint16_t")) {
          primitive_type = nvidia::gxf::PrimitiveType::kUnsigned16;
        } else if ((value == "int") || (value == "signed int") || (value == "int32") ||
                   (value == "int32_t")) {
          primitive_type = nvidia::gxf::PrimitiveType::kInt32;
        } else if ((value == "uint") || (value == "unsigned int") || (value == "uint32") ||
                   (value == "uint32_t")) {
          primitive_type = nvidia::gxf::PrimitiveType::kUnsigned32;
        } else if (value == "float") {
          primitive_type = nvidia::gxf::PrimitiveType::kFloat32;
        } else {
          holoscan::log_error("NRRD unexpected value for {}: {}", parameter, value);
          return false;
        }
      } else if (parameter == "datafile") {
        const std::string path = file_name.substr(0, file_name.find_last_of("/\\") + 1);
        data_file_name = path + value;
      } else if (parameter == "space") {
        std::stringstream values(value);
        std::string orientation, space;
        while (std::getline(values, space, '-')) {
          if (space == "left") {
            orientation += "L";
          } else if (space == "right") {
            orientation += "R";
          } else if (space == "anterior") {
            orientation += "A";
          } else if (space == "posterior") {
            orientation += "P";
          } else if (space == "superior") {
            orientation += "S";
          } else if (space == "inferior") {
            orientation += "I";
          } else {
            holoscan::log_error("NRRD unexpected space string {}", space);
            return false;
          }
        }
        volume.SetOrientation(orientation);
      } else if (parameter == "spaceorigin") {
        auto space_origin = parse_vector(value);
        std::copy_n(space_origin.begin(), 3, volume.space_origin_.begin());
      } else if (parameter == "spacedirections") {
        auto values = split_string_by_space(value);
        for (const auto& value : values) {
          auto space_directions = parse_vector(value);
          std::array<double, 3> space_direction = {0.0, 0.0, 0.0};
          std::copy_n(space_directions.begin(), 3, space_direction.begin());
          volume.space_directions_.push_back(space_direction);
        }
      }
    }
  }

  int byte_skip = 0;
  {
    std::ifstream file;
    file.open(file_name, std::ios::in);
    if (!file.is_open()) {
      holoscan::log_error("NRRD could not open {}", file_name);
      return false;
    }
    std::string line;
    while (std::getline(file, line)) {
      if (file.tellg() != -1) { byte_skip = file.tellg(); }
    }
  }

  const size_t data_size =
      dims[0] * dims[1] * dims[2] * nvidia::gxf::PrimitiveTypeSize(primitive_type);
  std::unique_ptr<uint8_t> data(new uint8_t[data_size]);
  if (is_nrrd(file_name) && data_file_name.size() == 0) {
    if (compressed) {
      holoscan::log_error("NRRD attached-header with compressed data is not supported.");
      return false;
    }
    std::ifstream file;
    file.open(file_name, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
      holoscan::log_error("NRRD could not open {}", data_file_name);
      return false;
    }

    file.seekg(byte_skip, std::ios_base::beg);

    file.read(reinterpret_cast<char*>(data.get()), data_size);
  } else if (data_file_name.size() != 0) {
    std::ifstream file;

    file.open(data_file_name, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
      holoscan::log_error("NRRD could not open {}", data_file_name);
      return false;
    }
    const std::streampos file_size = file.tellg();
    file.seekg(0, std::ios_base::beg);
    if (compressed) {
      // need to uncompress, first read to 'compressed_data' vector and then uncompress to 'data'
      std::vector<uint8_t> compressed_data(file_size);

      // read
      file.read(reinterpret_cast<char*>(compressed_data.data()), compressed_data.size());

      // uncompress
      z_stream strm{};
      int result = inflateInit2(&strm, 32 + MAX_WBITS);
      if (result != Z_OK) {
        holoscan::log_error("NRRD failed to uncompress {}, inflateInit2 failed with error code {}",
                            data_file_name,
                            result);
        return false;
      }

      strm.next_in = compressed_data.data();
      strm.avail_in = compressed_data.size();
      strm.next_out = data.get();
      strm.avail_out = data_size;

      result = inflate(&strm, Z_FINISH);
      inflateEnd(&strm);
      if (result != Z_STREAM_END) {
        holoscan::log_error("NRRD failed to uncompress {}, inflate failed with error code {}",
                            data_file_name,
                            result);
        return false;
      }
    } else {
      holoscan::log_error("NRRD unsupported file format");
    }
  }

  // allocate the tensor
  if (!volume.tensor_->reshapeCustom(nvidia::gxf::Shape(dims),
                                     primitive_type,
                                     nvidia::gxf::PrimitiveTypeSize(primitive_type),
                                     nvidia::gxf::Unexpected{GXF_UNINITIALIZED_VALUE},
                                     volume.storage_type_,
                                     volume.allocator_)) {
    holoscan::log_error("NRRD failed to reshape tensor");
    return false;
  }

  // copy the data
  switch (volume.storage_type_) {
    case nvidia::gxf::MemoryStorageType::kDevice:
      if (cudaMemcpy(volume.tensor_->pointer(),
                     reinterpret_cast<const void*>(data.get()),
                     data_size,
                     cudaMemcpyHostToDevice) != cudaSuccess) {
        holoscan::log_error("NRRD failed to copy to GPU memory");
        return false;
      }
      break;
    case nvidia::gxf::MemoryStorageType::kHost:
    case nvidia::gxf::MemoryStorageType::kSystem:
      memcpy(volume.tensor_->pointer(), data.get(), data_size);
      break;
    default:
      holoscan::log_error("NRRD unhandled storage type {}", int(volume.storage_type_));
      return false;
  }

  return true;
}

std::vector<double> parse_vector(std::string str) {
  std::vector<double> result;
  trim(str);
  // ensures the string is surround by parenthesis and then remove them
  if ((str[0] != '(') || (str[str.length() - 1] != ')')) return result;
  str = str.substr(1, str.length() - 2);

  std::stringstream ss(str);
  std::string token;
  while (std::getline(ss, token, ',')) { result.push_back(std::stod(token)); }
  return result;
}

void trim(std::string& str) {
  str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
              return !std::isspace(ch);
            }));
  str.erase(
      std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) { return !std::isspace(ch); })
          .base(),
      str.end());
}

std::vector<std::string> split_string_by_space(std::string str) {
  std::vector<std::string> result;
  size_t start_index = 0;
  while (true) {
    while ((start_index < str.length()) && isspace(str[start_index])) { start_index++; }
    if (start_index >= str.length()) return result;
    size_t end_index = start_index;
    while ((end_index < str.length()) && !isspace(str[end_index])) { end_index++; }
    result.push_back(str.substr(start_index, end_index - start_index));
    start_index = end_index;
  }
}

}  // namespace holoscan::ops
