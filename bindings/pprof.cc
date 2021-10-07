/**
 * Copyright 2021 Datadog Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pprof.h"

#include <iostream>

namespace pprof {

//
// Encoding
//
Encoder::Encoder() {
  string_table.push_back("");
}

uint64_t Encoder::dedup(const std::string& str) {
  auto found = std::find(string_table.begin(), string_table.end(), str);
  if (found != string_table.end()) {
    return found - string_table.begin();
  }
  string_table.push_back(str);
  return string_table.size() - 1;
}
uint64_t Encoder::dedup(const Mapping& mapping) {
  if (mapping == Mapping()) return 0;
  auto found = std::find(mappings.begin(), mappings.end(), mapping);
  if (found != mappings.end()) {
    return found - mappings.begin() + 1;
  }
  mappings.push_back(mapping);
  return mappings.size();
}
uint64_t Encoder::dedup(const Location& location) {
  if (location == Location()) return 0;
  auto found = std::find(locations.begin(), locations.end(), location);
  if (found != locations.end()) {
    return found - locations.begin() + 1;
  }
  locations.push_back(location);
  return locations.size();
}
uint64_t Encoder::dedup(const Function& function) {
  if (function == Function()) return 0;
  auto found = std::find(functions.begin(), functions.end(), function);
  if (found != functions.end()) {
    return found - functions.begin() + 1;
  }
  functions.push_back(function);
  return functions.size();
}

template <typename T, typename>
std::string Encoder::encode(T number) {
  std::vector<uint8_t> bytes;
  while (number >= 0b10000000) {
    bytes.push_back((static_cast<unsigned char>(number) & 0b01111111) | 0b10000000);
    number >>= 7;
  }
  bytes.push_back(static_cast<unsigned char>(number));
  std::string str(bytes.begin(), bytes.end());
  return str;
}
std::string Encoder::encode(bool v) {
  std::ostringstream bytes;
  bytes << static_cast<unsigned char>(v ? 1 : 0);
  return bytes.str();
}
template <typename T>
std::string Encoder::encode_varint(uint8_t index, T number) {
  std::ostringstream bytes;
  if (number != 0) {
    bytes << static_cast<unsigned char>((index << 3) | WireTypeVarInt);
    bytes << encode(number);
  }
  return bytes.str();
}
template <typename T>
std::string Encoder::encode_length_delimited(uint8_t index, const T& value) {
  std::ostringstream bytes;
  std::string encoded_value = encode(value);
  if (encoded_value.size()) {
    bytes << static_cast<unsigned char>((index << 3) | WireTypeLengthDelimited);
    bytes << encode(encoded_value.size());
    bytes << encoded_value;
  }
  return bytes.str();
}
template <typename T>
std::string Encoder::encode_length_delimited_with_id(
  uint64_t id, uint8_t index, const T& value) {
  std::ostringstream bytes;
  std::string encoded_value = encode(id, value);
  if (encoded_value.size()) {
    bytes << static_cast<unsigned char>((index << 3) | WireTypeLengthDelimited);
    bytes << encode(encoded_value.size());
    bytes << encoded_value;
  }
  return bytes.str();
}
std::string Encoder::encode_string(uint8_t index, const std::string& str) {
  std::ostringstream bytes;
  bytes << static_cast<unsigned char>((index << 3) | WireTypeLengthDelimited);
  bytes << encode(str.size());
  bytes << str;
  return bytes.str();
}
std::string Encoder::encode(const ValueType& value_type) {
  std::ostringstream bytes;
  bytes << encode_varint(1, dedup(value_type.type));
  bytes << encode_varint(2, dedup(value_type.unit));
  return bytes.str();
}
std::string Encoder::encode(const Label& label) {
  std::ostringstream bytes;
  bytes << encode_varint(1, dedup(label.key));
  bytes << encode_varint(2, dedup(label.str));
  bytes << encode_varint(3, label.num);
  bytes << encode_varint(4, dedup(label.num_unit));
  return bytes.str();
}
std::string Encoder::encode(const Sample& sample) {
  std::ostringstream bytes;
  if (sample.locations.size()) {
    std::vector<uint64_t> ids;
    std::transform(
      sample.locations.begin(),
      sample.locations.end(),
      std::back_inserter(ids),
      [this](const pprof::Location& location) -> uint64_t {
        return this->dedup(location);
      });

    std::ostringstream locationBytes;
    for (auto locationId : ids) {
      locationBytes << encode(locationId);
    }

    std::string location_ids = locationBytes.str();
    if (location_ids.size()) {
      bytes << static_cast<unsigned char>((1 << 3) | WireTypeLengthDelimited);
      bytes << encode(location_ids.size());
      bytes << location_ids;
    }
  }
  if (sample.values.size()) {
    std::ostringstream valuesBytes;
    for (auto value : sample.values) {
      valuesBytes << encode(value);
    }

    std::string values = valuesBytes.str();
    if (values.size()) {
      bytes << static_cast<unsigned char>((2 << 3) | WireTypeLengthDelimited);
      bytes << encode(values.size());
      bytes << values;
    }
  }
  for (auto label : sample.labels) {
    bytes << encode_length_delimited(3, label);
  }
  return bytes.str();
}
std::string Encoder::encode(uint64_t id, const Mapping& mapping) {
  std::ostringstream bytes;
  bytes << encode_varint(1, id);
  bytes << encode_varint(2, mapping.memory_start);
  bytes << encode_varint(3, mapping.memory_limit);
  bytes << encode_varint(4, mapping.file_offset);
  bytes << encode_varint(5, mapping.filename);
  bytes << encode_varint(6, mapping.build_id);
  bytes << encode_varint(7, mapping.has_functions);
  bytes << encode_varint(8, mapping.has_filenames);
  bytes << encode_varint(9, mapping.has_line_numbers);
  bytes << encode_varint(10, mapping.has_inline_frames);
  return bytes.str();
}
std::string Encoder::encode(const Line& line) {
  std::ostringstream bytes;
  bytes << encode_varint(1, dedup(line.function));
  bytes << encode_varint(2, line.line);
  return bytes.str();
}
std::string Encoder::encode(uint64_t id, const Location& location) {
  std::ostringstream bytes;
  bytes << encode_varint(1, id);
  bytes << encode_varint(2, dedup(location.mapping));
  bytes << encode_varint(3, location.address);
  for (auto line : location.lines) {
    bytes << encode_length_delimited(4, line);
  }
  bytes << encode_varint(5, location.is_folded);
  return bytes.str();
}
std::string Encoder::encode(uint64_t id, const Function& function) {
  std::ostringstream bytes;
  bytes << encode_varint(1, id);
  bytes << encode_varint(2, dedup(function.name));
  bytes << encode_varint(3, dedup(function.system_name));
  bytes << encode_varint(4, dedup(function.filename));
  bytes << encode_varint(5, function.start_line);
  return bytes.str();
}
std::string Encoder::encode(const Profile& profile) {
  std::ostringstream bytes;
  for (auto sample_type : profile.sample_type) {
    bytes << encode_length_delimited(1, sample_type);
  }
  for (auto sample : profile.sample) {
    bytes << encode_length_delimited(2, sample);
  }
  // Add locations before mappings as mappings are extracted from them
  std::ostringstream locationBytes;
  for (size_t i = 0; i < locations.size(); i++) {
    locationBytes << encode_length_delimited_with_id(i + 1, 4, locations.at(i));
  }
  for (size_t i = 0; i < mappings.size(); i++) {
    bytes << encode_length_delimited_with_id(i + 1, 3, mappings.at(i));
  }
  bytes << locationBytes.str();
  for (size_t i = 0; i < functions.size(); i++) {
    bytes << encode_length_delimited_with_id(i + 1, 5, functions.at(i));
  }
  auto period_type = encode_length_delimited(11, profile.period_type);
  std::ostringstream commentBytes;
  for (auto comment : profile.comment) {
    commentBytes << encode_varint(13, dedup(comment));
  }
  // Add string table last as other field encodings may add to it
  for (auto str : string_table) {
    bytes << encode_string(6, str);
  }
  bytes << encode_varint(7, profile.drop_frames);
  bytes << encode_varint(8, profile.keep_frames);
  bytes << encode_varint(9, profile.time_nanos);
  bytes << encode_varint(10, profile.duration_nanos);
  bytes << period_type;
  bytes << encode_varint(12, profile.period);
  bytes << commentBytes.str();
  bytes << encode_varint(14, profile.default_sample_type);
  return bytes.str();
}

}  // namespace pprof
