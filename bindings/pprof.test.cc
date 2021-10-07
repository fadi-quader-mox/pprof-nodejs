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

#include <functional>
#include <iostream>
#include <algorithm>
#include <iomanip>

#include "pprof.h"
#include "tap.h"
#include "result.h"

//
// Byte array manipulation
//
std::string hex(const std::vector<char>& data) {
  std::stringstream ss;
  ss << std::hex << std::setw(2) << std::setfill('0');
  for (size_t i = 0; i < data.size(); i++) {
    ss << (0xff & static_cast<int>(data[i]));
  }
  return ss.str();
}

std::vector<char> slice(const std::vector<char>& bytes, int start = 0,
  int end = -1) {
  end = (end < 0 ? bytes.size() + 1 : start) + end;
  end = std::min(std::max(end, start), static_cast<int>(bytes.size()));
  std::vector<char> result(bytes.begin() + start, bytes.begin() + end);
  return result;
}

std::vector<char> slice(const std::string& str, int start = 0, int end = -1) {
  std::vector<char> bytes(str.begin(), str.end());
  return slice(bytes, start, end);
}

//
// PProf parser
//
const int kTypeVarInt = 0;
const int kTypeLengthDelim = 2;
const int kHasMoreBytes = 0b10000000;
const int kNumberBytes = 0b01111111;

struct FieldFlag {
  int flag;
  int field;
  int mode;

  explicit FieldFlag(int flag)
    : flag(flag),
      field(flag >> 3),
      mode(flag & 0b111) {}

  static Result<FieldFlag> New(int byte) {
    FieldFlag flag(byte);
    if (flag.mode != 0 && flag.mode != 2) {
      std::string message = "Invalid flag field: ";
      message += std::to_string(flag.field);
      message += ", mode: ";
      message += std::to_string(flag.mode);
      return Err<FieldFlag>(message);
    }
    return Ok<FieldFlag>(flag);
  }
};

struct NumberWithSize {
  int64_t value = 0;
  size_t offset = 0;
};

NumberWithSize get_number_with_size(const std::vector<char>& bytes) {
  NumberWithSize value;
  if (!bytes.size()) return value;

  value.value = bytes[0] & kNumberBytes;
  while (static_cast<uint8_t>(bytes[value.offset++]) >= kHasMoreBytes) {
    value.value |= (bytes[value.offset] & kNumberBytes) << (7 * value.offset);
  }

  return value;
}

struct Value {
  std::vector<char> value;
  size_t offset = 0;

  explicit Value(const std::vector<char>& value)
    : value(value) {}

  Value(const std::vector<char>& value, size_t offset)
    : value(value), offset(offset) {}
};

Value get_value(int mode, const std::vector<char>& bytes) {
  switch (mode) {
    case kTypeVarInt: {  // var-int
      for (size_t i = 0; i < bytes.size(); i++) {
        if (!(bytes[i] & kHasMoreBytes)) {
          return Value(slice(bytes, 0, i + 1));
        }
      }
      break;
    }
    case kTypeLengthDelim: {  // length-delimited
      NumberWithSize value = get_number_with_size(bytes);
      return Value(slice(bytes, value.offset, value.value), value.offset);
    }
  }

  return Value(bytes);
}

int64_t get_number(const std::vector<char>& bytes) {
  int64_t value = 0;
  for (size_t i = 0; i < bytes.size(); i++) {
    int byte = bytes[i];
    value |= (byte & kNumberBytes) << (7 * i);
    if (!(byte & kHasMoreBytes)) break;
  }
  return value;
}

std::vector<int64_t> get_numbers(const std::vector<char>& bytes) {
  std::vector<int64_t> numbers;

  for (size_t i = 0, start = 0; i < bytes.size(); i++) {
    if ((bytes[i] & kHasMoreBytes) == 0) {
      numbers.push_back(get_number(slice(bytes, start, i + 1)));
      start = i + 1;
    }
  }

  return numbers;
}

std::string get_string(const std::vector<char>& bytes) {
  return std::string(bytes.begin(), bytes.end());
}

struct ValueType {
  int64_t type = 0;
  int64_t unit = 0;

  ValueType() = default;

  inline bool operator==(const ValueType& rhs) const {
    return type == rhs.type && unit == rhs.unit;
  }

  static Result<ValueType> decode(const std::vector<char>& bytes) {
    ValueType value_type;

    size_t index = 0;
    while (index < bytes.size()) {
      Result<FieldFlag> flag_result = FieldFlag::New(bytes[index]);
      if (!flag_result.is_ok) return Err<ValueType>(flag_result.error);
      FieldFlag flag = flag_result.value;
      index++;

      auto value = get_value(flag.mode, slice(bytes, index));
      index += value.value.size() + value.offset;

      switch (flag.field) {
        case 1: {  // type
          value_type.type = get_number(value.value);
          break;
        }
        case 2: {  // unit
          value_type.unit = get_number(value.value);
          break;
        }
      }
    }

    return Ok<ValueType>(value_type);
  }
};

struct Label {
  int64_t key = 0;
  int64_t str = 0;
  int64_t num = 0;
  int64_t num_unit = 0;

  Label() = default;

  inline bool operator==(const Label& rhs) const {
    return key == rhs.key &&
      str == rhs.str &&
      num == rhs.num &&
      num_unit == rhs.num_unit;
  }

  static Result<Label> decode(const std::vector<char>& bytes) {
    Label label;

    size_t index = 0;
    while (index < bytes.size()) {
      Result<FieldFlag> flag_result = FieldFlag::New(bytes[index]);
      if (!flag_result.is_ok) return Err<Label>(flag_result.error);
      FieldFlag flag = flag_result.value;
      index++;

      auto value = get_value(flag.mode, slice(bytes, index));
      index += value.value.size() + value.offset;

      switch (flag.field) {
        case 1: {  // key
          label.key = get_number(value.value);
          break;
        }
        case 2: {  // str
          label.str = get_number(value.value);
          break;
        }
        case 3: {  // num
          label.num = get_number(value.value);
          break;
        }
        case 4: {  // num_unit
          label.num_unit = get_number(value.value);
          break;
        }
      }
    }

    return Ok<Label>(label);
  }
};

struct Sample {
  std::vector<int64_t> location_ids;
  std::vector<int64_t> values;
  std::vector<Label> labels;

  Sample() = default;

  inline bool operator==(const Sample& rhs) const {
    return location_ids == rhs.location_ids &&
      values == rhs.values &&
      labels == rhs.labels;
  }

  static Result<Sample> decode(const std::vector<char>& bytes) {
    Sample sample;

    size_t index = 0;
    while (index < bytes.size()) {
      Result<FieldFlag> flag_result = FieldFlag::New(bytes[index]);
      if (!flag_result.is_ok) return Err<Sample>(flag_result.error);
      FieldFlag flag = flag_result.value;
      index++;

      auto value = get_value(flag.mode, slice(bytes, index));
      index += value.value.size() + value.offset;

      switch (flag.field) {
        case 1: {  // location_ids
          sample.location_ids = get_numbers(value.value);
          break;
        }
        case 2: {  // values
          sample.values = get_numbers(value.value);
          break;
        }
        case 3: {  // labels
          auto result = Label::decode(value.value);
          if (!result.is_ok) return Err<Sample>(result.error);
          sample.labels.push_back(result.value);
          break;
        }
      }
    }

    return Ok<Sample>(sample);
  }
};

struct Mapping {
  int64_t id = 0;
  uint64_t memory_start = 0;
  uint64_t memory_limit = 0;
  uint64_t file_offset = 0;
  int64_t filename = 0;
  int64_t build_id = 0;
  bool has_functions = false;
  bool has_filenames = false;
  bool has_line_numbers = false;
  bool has_inline_frames = false;

  Mapping() = default;

  inline bool operator==(const Mapping& rhs) const {
    return id == rhs.id &&
      memory_start == rhs.memory_start &&
      memory_limit == rhs.memory_limit &&
      file_offset == rhs.file_offset &&
      filename == rhs.filename &&
      build_id == rhs.build_id &&
      has_functions == rhs.has_functions &&
      has_filenames == rhs.has_filenames &&
      has_line_numbers == rhs.has_line_numbers &&
      has_inline_frames == rhs.has_inline_frames;
  }

  static Result<Mapping> decode(const std::vector<char>& bytes) {
    Mapping mapping;

    size_t index = 0;
    while (index < bytes.size()) {
      Result<FieldFlag> flag_result = FieldFlag::New(bytes[index]);
      if (!flag_result.is_ok) return Err<Mapping>(flag_result.error);
      FieldFlag flag = flag_result.value;
      index++;

      auto value = get_value(flag.mode, slice(bytes, index));
      index += value.value.size() + value.offset;

      switch (flag.field) {
        case 1: {  // id
          mapping.id = get_number(value.value);
          break;
        }
        case 2: {  // memory_start
          mapping.memory_start = get_number(value.value);
          break;
        }
        case 3: {  // memory_limit
          mapping.memory_limit = get_number(value.value);
          break;
        }
        case 4: {  // file_offset
          mapping.file_offset = get_number(value.value);
          break;
        }
        case 5: {  // filename
          mapping.filename = get_number(value.value);
          break;
        }
        case 6: {  // build_id
          mapping.build_id = get_number(value.value);
          break;
        }
        case 7: {  // has_functions
          mapping.has_functions = get_number(value.value);
          break;
        }
        case 8: {  // has_filenames
          mapping.has_filenames = get_number(value.value);
          break;
        }
        case 9: {  // has_line_numbers
          mapping.has_line_numbers = get_number(value.value);
          break;
        }
        case 10: {  // has_inline_frames
          mapping.has_inline_frames = get_number(value.value);
          break;
        }
      }
    }

    return Ok<Mapping>(mapping);
  }
};

struct Function {
  int64_t id = 0;
  int64_t name = 0;
  int64_t system_name = 0;
  int64_t filename = 0;
  int64_t start_line = 0;

  Function() = default;

  inline bool operator==(const Function& rhs) const {
    return id == rhs.id &&
      name == rhs.name &&
      system_name == rhs.system_name &&
      filename == rhs.filename &&
      start_line == rhs.start_line;
  }

  static Result<Function> decode(const std::vector<char>& bytes) {
    Function function;

    size_t index = 0;
    while (index < bytes.size()) {
      Result<FieldFlag> flag_result = FieldFlag::New(bytes[index]);
      if (!flag_result.is_ok) return Err<Function>(flag_result.error);
      FieldFlag flag = flag_result.value;
      index++;

      auto value = get_value(flag.mode, slice(bytes, index));
      index += value.value.size() + value.offset;

      switch (flag.field) {
        case 1: {  // id
          function.id = get_number(value.value);
          break;
        }
        case 2: {  // name
          function.name = get_number(value.value);
          break;
        }
        case 3: {  // system_name
          function.system_name = get_number(value.value);
          break;
        }
        case 4: {  // filename
          function.filename = get_number(value.value);
          break;
        }
        case 5: {  // start_line
          function.start_line = get_number(value.value);
          break;
        }
      }
    }

    return Ok<Function>(function);
  }
};

struct Line {
  int64_t function_id = 0;
  int64_t line_number = 0;

  Line() = default;

  inline bool operator==(const Line& rhs) const {
    return function_id == rhs.function_id &&
      line_number == rhs.line_number;
  }

  static Result<Line> decode(const std::vector<char>& bytes) {
    Line line;

    size_t index = 0;
    while (index < bytes.size()) {
      Result<FieldFlag> flag_result = FieldFlag::New(bytes[index]);
      if (!flag_result.is_ok) return Err<Line>(flag_result.error);
      FieldFlag flag = flag_result.value;
      index++;

      auto value = get_value(flag.mode, slice(bytes, index));
      index += value.value.size() + value.offset;

      switch (flag.field) {
        case 1: {  // function_id
          line.function_id = get_number(value.value);
          break;
        }
        case 2: {  // line_number
          line.line_number = get_number(value.value);
          break;
        }
      }
    }

    return Ok<Line>(line);
  }
};

struct Location {
  int64_t id = 0;
  int64_t mapping_id = 0;
  uint64_t address = 0;
  std::vector<Line> lines;
  bool is_folded = false;

  Location() = default;

  inline bool operator==(const Location& rhs) const {
    return id == rhs.id &&
      mapping_id == rhs.mapping_id &&
      address == rhs.address &&
      lines == rhs.lines &&
      is_folded != rhs.is_folded;
  }

  static Result<Location> decode(const std::vector<char>& bytes) {
    Location location;

    size_t index = 0;
    while (index < bytes.size()) {
      Result<FieldFlag> flag_result = FieldFlag::New(bytes[index]);
      if (!flag_result.is_ok) return Err<Location>(flag_result.error);
      FieldFlag flag = flag_result.value;
      index++;

      auto value = get_value(flag.mode, slice(bytes, index));
      index += value.value.size() + value.offset;

      switch (flag.field) {
        case 1: {  // id
          location.id = get_number(value.value);
          break;
        }
        case 2: {  // mapping_id
          location.mapping_id = get_number(value.value);
          break;
        }
        case 3: {  // address
          location.address = get_number(value.value);
          break;
        }
        case 4: {  // lines
          auto result = Line::decode(value.value);
          if (!result.is_ok) return Err<Location>(result.error);
          location.lines.push_back(result.value);
          break;
        }
        case 5: {  // is_folded
          location.is_folded = get_number(value.value);
          break;
        }
      }
    }

    return Ok<Location>(location);
  }
};

struct Profile {
  std::vector<ValueType> sample_types;
  std::vector<Sample> samples;
  std::vector<Mapping> mappings;
  std::vector<Location> locations;
  std::vector<Function> functions;
  std::vector<std::string> string_table;
  int64_t drop_frames = 0;
  int64_t keep_frames = 0;
  int64_t time_nanos = 0;
  int64_t duration_nanos = 0;
  ValueType period_type;
  int64_t period = 0;
  std::vector<int64_t> comment;
  int64_t default_sample_type = 0;

  Profile() = default;

  inline bool operator==(const Profile& rhs) const {
    return sample_types == rhs.sample_types &&
      samples == rhs.samples &&
      mappings == rhs.mappings &&
      locations == rhs.locations &&
      functions == rhs.functions &&
      string_table == rhs.string_table &&
      drop_frames == rhs.drop_frames &&
      keep_frames == rhs.keep_frames &&
      time_nanos == rhs.time_nanos &&
      duration_nanos == rhs.duration_nanos &&
      &period_type == &rhs.period_type &&
      period == rhs.period &&
      comment == rhs.comment &&
      default_sample_type == rhs.default_sample_type;
  }

  static Result<Profile> decode(const std::vector<char>& bytes) {
    Profile profile;

    size_t index = 0;
    while (index < bytes.size()) {
      Result<FieldFlag> flag_result = FieldFlag::New(bytes[index]);
      if (!flag_result.is_ok) return Err<Profile>(flag_result.error);
      FieldFlag flag = flag_result.value;
      index++;

      auto value = get_value(flag.mode, slice(bytes, index));
      index += value.value.size() + value.offset;

      switch (flag.field) {
        case 1: {  // sample_types
          auto result = ValueType::decode(value.value);
          if (!result.is_ok) return Err<Profile>(result.error);
          profile.sample_types.push_back(result.value);
          break;
        }
        case 2: {  // samples
          auto result = Sample::decode(value.value);
          if (!result.is_ok) return Err<Profile>(result.error);
          profile.samples.push_back(result.value);
          break;
        }
        case 3: {  // mappings
          auto result = Mapping::decode(value.value);
          if (!result.is_ok) return Err<Profile>(result.error);
          profile.mappings.push_back(result.value);
          break;
        }
        case 4: {  // locations
          auto result = Location::decode(value.value);
          if (!result.is_ok) return Err<Profile>(result.error);
          profile.locations.push_back(result.value);
          break;
        }
        case 5: {  // functions
          auto result = Function::decode(value.value);
          if (!result.is_ok) return Err<Profile>(result.error);
          profile.functions.push_back(result.value);
          break;
        }
        case 6: {  // string_table
          profile.string_table.push_back(get_string(value.value));
          break;
        }
        case 7: {  // drop_frames
          profile.drop_frames = get_number(value.value);
          break;
        }
        case 8: {  // keep_frames
          profile.keep_frames = get_number(value.value);
          break;
        }
        case 9: {  // time_nanos
          profile.time_nanos = get_number(value.value);
          break;
        }
        case 10: {  // duration_nanos
          profile.duration_nanos = get_number(value.value);
          break;
        }
        case 11: {  // period_type
          auto result = ValueType::decode(value.value);
          if (!result.is_ok) return Err<Profile>(result.error);
          profile.period_type = result.value;
          break;
        }
        case 12: {  // period
          profile.period = get_number(value.value);
          break;
        }
        case 13: {  // comment
          profile.comment.push_back(get_number(value.value));
          break;
        }
        case 14: {  // default_sample_type
          profile.default_sample_type = get_number(value.value);
          break;
        }
      }
    }

    return Ok<Profile>(profile);
  }
};

//
// Tests
//
std::string lookup(const Profile& profile, int64_t index) {
  return profile.string_table.at(index);
}
Location get_location(const Profile& profile, int64_t id) {
  return profile.locations.at(id - 1);
}
Function get_function(const Profile& profile, int64_t id) {
  return profile.functions.at(id - 1);
}
Mapping get_mapping(const Profile& profile, int64_t id) {
  return profile.mappings.at(id - 1);
}

template<typename E, typename R>
void compare(Tap* t, const Profile& profile, E expected, R received,
  const std::string& name);

template<typename E, typename R,
  std::enable_if_t<!std::is_arithmetic<E>::value, bool> = true>
void compare(Tap* t, const Profile& profile, const std::vector<E>& expected,
  const std::vector<R>& received);

void compare(Tap* t, const Profile& profile,
  const std::vector<int64_t>& expected, const std::vector<int64_t>& received) {
  t->plan(2);
  t->equal(expected.size(), received.size(), "count");
  t->equal(expected, received, "is equal");
}

void compare(Tap* t, const Profile& profile, const pprof::ValueType& expected,
  const ValueType& received) {
  t->plan(2);
  t->equal(lookup(profile, received.type), expected.type, "type");
  t->equal(lookup(profile, received.unit), expected.unit, "unit");
}

void compare(Tap* t, const Profile& profile, const pprof::Function& expected,
  const Function& received) {
  t->plan(4);
  t->equal(lookup(profile, received.name), expected.name, "name");
  t->equal(lookup(profile, received.system_name), expected.system_name,
    "system_name");
  t->equal(lookup(profile, received.filename), expected.filename, "filename");
  t->equal(received.start_line, expected.start_line, "start_line");
}

void compare(Tap* t, const Profile& profile, const pprof::Line& expected,
  const Line& received) {
  t->plan(3);
  t->equal(received.line_number, expected.line, "line_number");

  auto function = get_function(profile, received.function_id);
  t->equal(function.id, received.function_id, "function_id");
  compare(t, profile, expected.function, function, "function");
}

void compare(Tap* t, const Profile& profile, const pprof::Mapping& expected,
  int64_t received) {
  t->plan(10);
  auto mapping = get_mapping(profile, received);
  t->equal(mapping.id, received, "mapping_id");
  t->equal(mapping.memory_start, expected.memory_start, "memory_start");
  t->equal(mapping.memory_limit, expected.memory_limit, "memory_limit");
  t->equal(mapping.file_offset, expected.file_offset, "file_offset");
  t->equal(mapping.filename, expected.filename, "filename");
  t->equal(mapping.build_id, expected.build_id, "build_id");
  t->equal(mapping.has_functions, expected.has_functions, "has_functions");
  t->equal(mapping.has_filenames, expected.has_filenames, "has_filenames");
  t->equal(mapping.has_line_numbers, expected.has_line_numbers,
    "has_line_numbers");
  t->equal(mapping.has_inline_frames, expected.has_inline_frames,
    "has_inline_frames");
}

void compare(Tap* t, const Profile& profile, const pprof::Location& expected,
  int64_t received) {
  auto location = get_location(profile, received);
  t->plan(4 + (location.mapping_id != 0 ? 1 : 0));
  t->equal(received, location.id, "id");
  t->equal(expected.address, location.address, "address");
  t->equal(expected.is_folded, location.is_folded, "is_folded");
  if (location.mapping_id != 0) {
    compare(t, profile, expected.mapping, location.mapping_id, "mapping");
  }

  compare(t, profile, expected.lines, location.lines, "line");
}

void compare(Tap* t, const Profile& profile, const pprof::Label& expected,
  const Label& received) {
  t->plan(4);
  t->equal(lookup(profile, received.key), expected.key, "key");
  t->equal(lookup(profile, received.str), expected.str, "str");
  t->equal(received.num, expected.num, "num");
  t->equal(lookup(profile, received.num_unit), expected.num_unit, "num_unit");
}

void compare(Tap* t, const Profile& profile, const pprof::Sample& expected,
  const Sample& received) {
  t->plan(3);
  compare(t, profile, expected.values, received.values, "values");
  compare(t, profile, expected.locations, received.location_ids, "location");
  compare(t, profile, expected.labels, received.labels, "label");
}

template<typename E, typename R,
  std::enable_if_t<!std::is_arithmetic<E>::value, bool>>
void compare(Tap* t, const Profile& profile, const std::vector<E>& expected,
  const std::vector<R>& received) {
  t->plan(expected.size() + 1);
  t->equal(expected.size(), received.size(), "count");
  for (size_t i = 0; i < expected.size(); i++) {
    auto est = expected.at(i);
    auto rst = received.at(i);
    compare(t, profile, est, rst, std::to_string(i));
  }
}

template<typename E, typename R>
void compare(Tap* t, const Profile& profile, E expected, R received,
  const std::string& name) {
  t->test(name, [=](Tap* t) { compare(t, profile, expected, received); });
}

void compare(Tap* t, const pprof::Profile& expected, const Profile& received) {
  t->plan(9);
  t->equal(received.drop_frames, expected.drop_frames, "drop_frames");
  t->equal(received.keep_frames, expected.keep_frames, "keep_frames");
  t->equal(received.time_nanos, expected.time_nanos, "time_nanos");
  t->equal(received.duration_nanos, expected.duration_nanos,
    "duration_nanos");
  t->equal(received.period, expected.period, "period");
  t->equal(received.default_sample_type, expected.default_sample_type,
    "default_sample_type");
  compare(t, received, expected.period_type, received.period_type,
    "period_type");
  compare(t, received, expected.sample_type, received.sample_types,
    "sample_type");
  compare(t, received, expected.sample, received.samples, "sample");
}

int main() {
  Tap t;
  t.plan(3);

  t.test("number encoding", [](Tap *t) {
    t->plan(1);

    // Up to 127 fits in one byte
    for (uint64_t i = 0; i < 128; i++) {
      std::vector<uint8_t> bytes;
      bytes.push_back(i);
      std::string expected(bytes.begin(), bytes.end());

      std::string encoded = pprof::Encoder().encode(i);
      if (encoded != expected) {
        t->fail("0-127 should have one byte");
        return;
      }
    }

    // 128-255 fits in two bytes
    for (uint64_t i = 128; i < 256; i++) {
      std::vector<uint8_t> bytes;
      bytes.push_back(i);
      bytes.push_back(1);
      std::string expected(bytes.begin(), bytes.end());

      std::string encoded = pprof::Encoder().encode(i);
      if (encoded != expected) {
        t->fail("128-255 should have two bytes");
        return;
      }
    }

    // 256 fits in two bytes with 2 in the second byte
    uint64_t i = 256;
    std::vector<uint8_t> bytes;
    bytes.push_back(128);
    bytes.push_back(2);
    std::string expected(bytes.begin(), bytes.end());

    std::string encoded = pprof::Encoder().encode(i);
    for (auto byte : slice(encoded)) {
      std::cout << std::to_string((uint8_t)byte) << std::endl;
    }
    if (encoded != expected) {
      t->fail("256 should have two bytes");
      return;
    }

    t->pass("encodes numbers correctly");
  });

  t.test("basic structure", [](Tap* t) {
    t->plan(2);
    pprof::Profile profile({"object", "count"}, {"heap", "bytes"});
    profile.period = 90;
    profile.time_nanos = 1234;
    profile.duration_nanos = 5678;
    profile.drop_frames = 123;
    profile.keep_frames = 321;

    pprof::Function function("name", "systemName", "scriptName");
    pprof::Line line(function, 123);
    pprof::Location location({line});

    pprof::Label label("foo", "bar");
    pprof::Sample sample({location}, {1234, 5678}, {label});
    profile.add_sample(sample);

    std::string encoded = pprof::Encoder().encode(profile);

    Result<Profile> result_profile = Profile::decode(slice(encoded));
    if (!result_profile.is_ok) {
      t->fail("parse error: " + result_profile.error);
      return;
    } else {
      t->pass("parsed");
    }

    auto parsed = result_profile.value;
    t->test("profile", [=](Tap* t) {
      compare(t, profile, parsed);
    });
  });

  t.test("deduplication", [](Tap* t) {
    t->plan(5);
    pprof::Profile profile({"object", "count"}, {"heap", "bytes"});
    {
      pprof::Line line({"name", "systemName", "scriptName"}, 123);
      pprof::Location location({line});
      pprof::Sample sample({location}, {1234, 5678});
      profile.add_sample(sample);
    }
    {
      pprof::Line line({"name", "systemName", "scriptName"}, 123);
      pprof::Location location({line});
      pprof::Sample sample({location}, {1234, 5678});
      profile.add_sample(sample);
    }

    std::string encoded = pprof::Encoder().encode(profile);

    Result<Profile> result_profile = Profile::decode(slice(encoded));
    if (!result_profile.is_ok) {
      t->fail("parse error: " + result_profile.error);
      return;
    } else {
      t->pass("parsed");
    }

    auto parsed = result_profile.value;
    t->equal(static_cast<int>(parsed.samples.size()), 2, "has two samples");
    t->equal(static_cast<int>(parsed.locations.size()), 1, "has one location");
    t->equal(static_cast<int>(parsed.functions.size()), 1, "has one function");
    t->equal(static_cast<int>(parsed.mappings.size()), 0, "has no mapping");
  });

  return t.end();
}
