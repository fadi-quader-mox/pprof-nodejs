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

#pragma once

#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

namespace pprof {

struct Profile;
struct Function;
struct Location;

struct ValueType {
  std::string type = 0;  // 1 - int64
  std::string unit = 0;  // 2 - int64

  ValueType(const std::string& type, const std::string& unit)
    : type(type), unit(unit) {}

  inline bool operator==(const ValueType& rhs) const {
    return type == rhs.type && unit == rhs.unit;
  }
};

struct Label {
  std::string key = "";       // 1 - int64
  std::string str = "";       // 2 - int64
  int64_t num = 0;            // 3 - int64
  std::string num_unit = "";  // 4 - int64

  Label(const std::string& key, const std::string& str)
    : key(key), str(str) {}

  Label(const std::string& key, int64_t num, const std::string& unit)
    : key(key), num(num), num_unit(unit) {}

  inline bool operator==(const Label& rhs) const {
    return key == rhs.key &&
      str == rhs.str &&
      num == rhs.num &&
      num_unit == rhs.num_unit;
  }
};

struct Sample {
  std::vector<Location> locations;  // 1
  std::vector<int64_t> values;      // 2
  std::vector<Label> labels;        // 3

  Sample(const std::vector<Location>& locations,
         const std::vector<int64_t>& values,
         const std::vector<Label>& labels = {})
    : locations(locations),
      values(values),
      labels(labels) {}

  inline bool operator==(const Sample& rhs) const {
    return locations == rhs.locations &&
      values == rhs.values &&
      labels == rhs.labels;
  }
};

struct Mapping {
  // 1 - id
  uint64_t memory_start = 0;         // 2
  uint64_t memory_limit = 0;         // 3
  uint64_t file_offset = 0;          // 4
  int64_t filename = 0;              // 5
  int64_t build_id = 0;              // 6
  bool has_functions = false;        // 7
  bool has_filenames = false;        // 8
  bool has_line_numbers = false;     // 9
  bool has_inline_frames = false;    // 10

  inline bool operator==(const Mapping& rhs) const {
    return memory_start == rhs.memory_start &&
      memory_limit == rhs.memory_limit &&
      file_offset == rhs.file_offset &&
      filename == rhs.filename &&
      build_id == rhs.build_id &&
      has_functions == rhs.has_functions &&
      has_filenames == rhs.has_filenames &&
      has_line_numbers == rhs.has_line_numbers &&
      has_inline_frames == rhs.has_inline_frames;
  }
};

struct Function {
  // 1 - id
  std::string name = "";         // 2
  std::string system_name = "";  // 3
  std::string filename = "";     // 4
  int64_t start_line = 0;        // 5

  Function(const std::string& name = "",
           const std::string& system_name = "",
           const std::string& filename = "",
           int64_t start_line = 0)
    : name(name),
      system_name(system_name),
      filename(filename),
      start_line(start_line) {}

  inline bool operator==(const Function& rhs) const {
    return name == rhs.name &&
      system_name == rhs.system_name &&
      filename == rhs.filename &&
      start_line == rhs.start_line;
  }
};

struct Line {
  Function function;  // 1
  int64_t line = 0;   // 2

  Line(const Function& function, int64_t line)
    : function(function), line(line) {}

  inline bool operator==(const Line& rhs) const {
    return function == rhs.function &&
      line == rhs.line;
  }
};

struct Location {
  // 1 - id
  Mapping mapping;           // 2
  uint64_t address = 0;      // 3
  std::vector<Line> lines;   // 4
  bool is_folded = false;    // 5

  explicit Location(const std::vector<Line>& lines = {},
                    const Mapping& mapping = Mapping(),
                    uint64_t address = 0,
                    bool is_folded = false)
    : mapping(mapping),
      address(address),
      lines(lines),
      is_folded(is_folded) {}

  inline bool operator==(const Location& rhs) const {
    return mapping == rhs.mapping &&
      address == rhs.address &&
      is_folded == rhs.is_folded &&
      lines == rhs.lines;
  }
};

struct Profile {
  std::vector<ValueType> sample_type;           // 1
  std::vector<Sample> sample;                   // 2
  // 3 - mapping
  // 4 - location
  // 5 - function
  // 6 - string_table
  int64_t drop_frames = 0;                      // 7
  int64_t keep_frames = 0;                      // 8
  int64_t time_nanos = 0;                       // 9
  int64_t duration_nanos = 0;                   // 10
  ValueType period_type;                        // 11
  int64_t period = 0;                           // 12
  std::vector<std::string> comment;             // 13
  int64_t default_sample_type = 0;              // 14

  Profile(
    const ValueType& object_sample_type,
    const ValueType& period_sample_type)
    : sample_type({object_sample_type, period_sample_type}),
      period_type(period_sample_type) {}

  uint64_t add_sample(const Sample& s) {
    sample.push_back(s);
    return sample.size() - 1;
  }

  inline bool operator==(const Profile& rhs) const {
    return drop_frames == rhs.drop_frames &&
      keep_frames == rhs.keep_frames &&
      time_nanos == rhs.time_nanos &&
      duration_nanos == rhs.duration_nanos &&
      period_type == rhs.period_type &&
      period == rhs.period &&
      comment == rhs.comment &&
      default_sample_type == rhs.default_sample_type &&
      sample_type == rhs.sample_type &&
      sample == rhs.sample;
  }
};

//
// Encoding
//
enum WireType {
  // int32, int64, uint32, uint64, sint32, sint64, bool, enum
  WireTypeVarInt,
  // fixed64, sfixed64, double
  WireType64Bit,
  // string, bytes, embedded messages
  WireTypeLengthDelimited,
  // fixed32, sfixed32, float
  WireType32Bit = 5
};

class Encoder {
  std::vector<std::string> string_table;
  std::vector<Mapping> mappings;
  std::vector<Location> locations;
  std::vector<Function> functions;

  uint64_t dedup(const std::string& str);
  uint64_t dedup(const Mapping& mapping);
  uint64_t dedup(const Location& location);
  uint64_t dedup(const Function& function);

 public:
  Encoder();

  std::string encode(uint64_t number);
  std::string encode(int64_t number);
  std::string encode(size_t number);
  std::string encode(bool v);
  template <typename T>
  std::string encode_varint(uint8_t index, T number);
  template <typename T>
  std::string encode_length_delimited(uint8_t index, const T& value);
  template <typename T>
  std::string encode_length_delimited_with_id(
    uint64_t id, uint8_t index, const T& value);
  std::string encode_string(uint8_t index, const std::string& str);
  std::string encode(const ValueType& value_type);
  std::string encode(const Label& label);
  std::string encode(const Sample& sample);
  std::string encode(uint64_t id, const Mapping& mapping);
  std::string encode(const Line& line);
  std::string encode(uint64_t id, const Location& location);
  std::string encode(uint64_t id, const Function& function);
  std::string encode(const Profile& profile);
};

}  // namespace pprof
