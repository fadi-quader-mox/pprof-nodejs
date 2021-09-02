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

#include <v8-profiler.h>
#include <node.h>
#include <napi.h>

#include <string>

#include "promise-worker.h"
#include "helpers.h"

class TimeProfileEncoder : public PromiseWorker {
 public:
  TimeProfileEncoder(const Napi::Env& env,
                     v8::CpuProfiler* profiler,
                     v8::Local<v8::String> name,
                     int nanos);
  ~TimeProfileEncoder();
  void Execute() override;
  void OnOK() override;

 private:
  v8::CpuProfiler* cpuProfiler;
  v8::CpuProfile* cpuProfile;
  int intervalNanos;
  int64_t startTime;
  std::string output;
};

class TimeProfiler : public Napi::ObjectWrap<TimeProfiler> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  explicit TimeProfiler(const Napi::CallbackInfo& info);

 private:
  Napi::Value Start(const Napi::CallbackInfo& info);
  Napi::Value Stop(const Napi::CallbackInfo& info);

#if NAPI_VERSION < 6
  static Napi::FunctionReference constructor;
#endif

  v8::CpuProfiler* cpuProfiler;
};
