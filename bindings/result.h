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

template<typename V, typename E>
struct Result {
  bool is_ok = false;
  union {
    V* value;
    E* error;
  };

  explicit Result(V* value) : is_ok(true), value(value) {}
  explicit Result(E* error) : is_ok(false), error(error) {}
};
template<typename V, typename E>
Result<V, E> Ok(V* value) {
  return Result<V, E>(value);
}
template<typename V, typename E>
Result<V, E> Ok(const V& value) {
  return Result<V, E>(&value);
}
template<typename V, typename E>
Result<V, E> Ok(V&& value) {
  return Result<V, E>(value);
}
template <typename V, typename E>
Result<V, E> Err(E* error) {
  return Result<V, E>(error);
}
template <typename V, typename E>
Result<V, E> Err(const E& error) {
  return Result<V, E>(&error);
}
template <typename V, typename E>
Result<V, E> Err(E&& error) {
  return Result<V, E>(error);
}

struct Error {
  const char* message;

  explicit Error(const char* message)
    : message(message) {}
};
