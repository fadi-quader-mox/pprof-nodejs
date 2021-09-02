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

#include <vector>

#include "helpers.h"

std::string fallback(const std::string& a, const std::string& b) {
  return a.empty() ? b : a;
}

std::string fallback(v8::Local<v8::String> a, const std::string& b) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  return a->Length()
    ? *v8::String::Utf8Value(isolate, a)
    : b;
}

v8::Local<v8::String> ToString(const std::string& data) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  return v8::String::NewFromUtf8(isolate, data.data(),
    v8::NewStringType::kInternalized).ToLocalChecked();
}

v8::Local<v8::String> ToString(Napi::String str) {
  return ToString(str.Utf8Value());
}

// Create a function call location
pprof::Location GetLocation(const std::string& name,
                            const std::string& systemName,
                            const std::string& scriptName,
                            int lineNumber) {
  pprof::Function function(name, systemName, scriptName);
  pprof::Line line(function, lineNumber);
  return pprof::Location({line});
}

// Make a two element sample from the given stack location
pprof::Sample MakeSample(const std::vector<pprof::Location>& stack,
                         int64_t first,
                         int64_t second) {
  return pprof::Sample(stack, {first, second});
}
