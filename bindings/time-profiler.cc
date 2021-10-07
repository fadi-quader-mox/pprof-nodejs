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

#include <iostream>

#include <sstream>
#include <vector>
#include <map>

#include "time-profiler.h"

// Statically allocate some things that will never change
static const char native_string[] = "<native>";
static const char anonymous_string[] = "(anonymous)";
static const char program_string[] = "(program)";
static const char idle_string[] = "(idle)";

static const pprof::ValueType sample_count("sample", "count");
static const pprof::ValueType wall_nanoseconds("wall", "nanoseconds");

// TODO(qard): Add source map support
TimeProfileEncoder::TimeProfileEncoder(const Napi::Env& env,
                                       v8::CpuProfiler* profiler,
                                       v8::Local<v8::String> name,
                                       int nanos)
  : PromiseWorker(env),
    cpuProfiler(profiler),
    cpuProfile(cpuProfiler->StopProfiling(name)),
    intervalNanos(nanos),
    startTime(now()) {}

TimeProfileEncoder::~TimeProfileEncoder() {
  cpuProfile->Delete();
  cpuProfile = nullptr;

  cpuProfiler->Dispose();
  cpuProfiler = nullptr;
}

void TimeProfileEncoder::Execute() {
  pprof::Profile profile(sample_count, wall_nanoseconds);
  profile.time_nanos = startTime;
  profile.period = intervalNanos;

  // Duration nanos
  auto startTime = cpuProfile->GetStartTime();
  auto endTime = cpuProfile->GetEndTime();
  profile.duration_nanos = (endTime - startTime) * 1000;

  // Add root children to queue with empty stacks
  std::map<const v8::CpuProfileNode*, std::vector<pprof::Location>> nodeStacks;
  std::vector<const v8::CpuProfileNode*> children;

  auto root = cpuProfile->GetTopDownRoot();
  for (int i = 0; i < root->GetChildrenCount(); i++) {
    auto child = root->GetChild(i);
    nodeStacks[child] = {};
    children.push_back(child);
  }

  // Process node queue
  while (children.size() > 0) {
    // Get last node
    auto node = children.back();
    children.pop_back();

    // Find function and script names
    auto scriptName = fallback(node->GetScriptResourceNameStr(),
                               native_string);
    auto name = fallback(node->GetFunctionNameStr(), anonymous_string);

    // Filter out some nodes we're not interested in
    if (name == idle_string || name == program_string) continue;

    // Create call location for this sample
    auto location =
      GetLocation(name, name, scriptName, node->GetLineNumber());

    // Add location to the stack for this node
    auto stack = nodeStacks[node];
    stack.emplace(stack.begin(), location);

    // Create time sample
    auto hitCount = node->GetHitCount();
    if (hitCount > 0) {
      profile.add_sample(
        MakeSample(stack, hitCount, hitCount * intervalNanos));
    }

    // Add node children to queue with a copy of the current stack
    for (int i = 0; i < node->GetChildrenCount(); i++) {
      auto child = node->GetChild(i);
      std::vector<pprof::Location> newStack(stack.begin(), stack.end());
      nodeStacks[child] = newStack;
      children.push_back(child);
    }
  }

  // Encode to pprof buffer
  output = pprof::Encoder().encode(profile);
}

void TimeProfileEncoder::OnOK() {
  Napi::Env env = Env();
  Napi::HandleScope scope(env);
  Resolve(Napi::Buffer<char>::Copy(env, output.data(), output.size()));
}

#if NAPI_VERSION < 6
Napi::FunctionReference TimeProfiler::constructor;
#endif

TimeProfiler::TimeProfiler(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<TimeProfiler>(info) {
  auto isolate = v8::Isolate::GetCurrent();
  Napi::Env env = info.Env();

  if (info.Length() != 1 || !info[0].IsNumber()) {
    Napi::TypeError::New(env, "Sample rate must be a number.")
      .ThrowAsJavaScriptException();
    return;
  }

  Napi::Number interval = info[0].As<Napi::Number>();

  cpuProfiler = v8::CpuProfiler::New(isolate);
  cpuProfiler->SetSamplingInterval(interval.DoubleValue());
}

Napi::Value TimeProfiler::Start(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() != 2) {
    Napi::TypeError::New(env, "Start must have two arguments.")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  if (!info[0].IsString()) {
    Napi::TypeError::New(env, "Profile name must be a string.")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  if (!info[1].IsBoolean()) {
    Napi::TypeError::New(env, "Include lines must be a boolean.")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  v8::Local<v8::String> name = ToString(info[0].As<Napi::String>());
  bool includeLines = info[1].As<Napi::Boolean>().Value();

  // Sample counts and timestamps are not used, so we do not need to record
  // samples.
  const bool recordSamples = false;

  if (includeLines) {
    cpuProfiler->StartProfiling(name, v8::CpuProfilingMode::kCallerLineNumbers,
                                recordSamples);
  } else {
    cpuProfiler->StartProfiling(name, recordSamples);
  }

  return env.Null();
}

Napi::Value TimeProfiler::Stop(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() != 2) {
    Napi::TypeError::New(env, "Stop must have three arguments.")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  if (!info[0].IsString()) {
    Napi::TypeError::New(env, "Profile name must be a string.")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  if (!info[1].IsNumber()) {
    Napi::TypeError::New(env, "Profile name must be a string.")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  v8::Local<v8::String> name = ToString(info[0].As<Napi::String>());
  int intervalNanos = info[1].As<Napi::Number>().Int32Value() * 1000;

  // Create a promise worker to encoder the time profile
  // NOTE: This instance is managed, so don't `delete` it.
  TimeProfileEncoder* encoder = new TimeProfileEncoder(
    env, cpuProfiler, name, intervalNanos);

  encoder->Queue();

  return encoder->Promise();
}

Napi::Object TimeProfiler::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func =
    DefineClass(env,
      "TimeProfiler",
      {InstanceMethod("start", &TimeProfiler::Start),
        InstanceMethod("stop", &TimeProfiler::Stop)});

#if NAPI_VERSION < 6
  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
#else
  Napi::FunctionReference* constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  env.SetInstanceData(constructor);
#endif  // NAPI_VERSION < 6

  exports.Set("TimeProfiler", func);
  return exports;
}
