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

#include <string>
#include <iostream>

class Tap {
  int plan_count = 0;
  int count = 0;
  int failures = 0;
  int skip_count = 0;
  int skipped = 0;
  std::string skip_message = "";
  std::string indent = "";

  explicit Tap(const std::string& indent) : indent(indent) {}

  void line(bool pass, const std::string& message) {
    std::string directive = skip_message;
    if (skip_count) {
      skip_count--;
      skipped++;
      if (!skip_count) {
        skip_message = "";
      }
    } else {
      count++;
      if (!pass) {
        failures++;
      }
    }
    // Status information
    std::cout << indent << (pass ? "" : "not ") << "ok " << count;
    // Optional message
    if (message.size()) {
      std::cout << " - " << message << " ";
    }
    // Directive
    if (directive.size()) {
      std::cout << " # SKIP " << directive;
    }
    std::cout << std::endl;
  }

 public:
  Tap() {
    std::cout << "TAP version 13" << std::endl;
  }

  void plan(int n) {
    plan_count = n;
    std::cout << indent << 1 << ".." << plan_count << std::endl;
  }

  void comment(const std::string& message) {
    std::cout << indent << "# " << message << std::endl;
  }

  void skip(int n, const std::string& message) {
    skip_count = n;
    skip_message = message;
  }

  void skip(const std::string& message) {
    skip(1, message);
  }

  void fail(const std::string& message) {
    line(false, message);
  }

  void pass(const std::string& message) {
    line(true, message);
  }

  template<typename T>
  void ok(T value, const std::string& message = "") {
    line(value, message);
  }

  template<typename T>
  void not_ok(T value, const std::string& message = "") {
    line(!value, message);
  }

  template<typename A, typename B>
  void equal(A a, B b, const std::string& message = "") {
    line(a == b, message);
  }

  template<typename A, typename B>
  void not_equal(A a, B b, const std::string& message = "") {
    line(a != b, message);
  }

  void bail_out(const std::string& reason) {
    std::cout << "Bail out! " << reason << std::endl;
    exit(1);
  }

  int end() {
    if (plan_count == 0) {
      plan(count + skipped);
    }
    if (failures > 0) return 1;
    if ((count + skipped) != plan_count) return 1;
    return 0;
  }

  void test(const std::string& message, std::function<void(Tap*)> fn) {
    std::cout << indent << "# Subtest: " << message << std::endl;
    Tap t(indent + "    ");
    fn(&t);
    line(t.end() == 0, message);
  }
};
