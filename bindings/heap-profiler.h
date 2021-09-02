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
#include <vector>

#include "promise-worker.h"
#include "helpers.h"

// This is used to efficiently remove v8 types before using the tree
struct AllocationNode {
 public:
  AllocationNode(v8::Isolate *isolate, v8::AllocationProfile::Node *node);

  std::string name;
  std::string script_name;
  int line_number;

  std::vector<std::shared_ptr<AllocationNode>> children;
  std::vector<v8::AllocationProfile::Allocation> allocations;
};

// TODO(qard): Add source map support
class HeapProfileEncoder : public PromiseWorker {
 public:
  HeapProfileEncoder(const Napi::Env& env, std::unique_ptr<AllocationNode> node,
    uint32_t interval);
  void Execute() override;
  void OnOK() override;

 private:
  std::unique_ptr<AllocationNode> root;
  uint32_t intervalBytes;
  int64_t startTime;
  size_t externalMemory;
  std::string output;
};

Napi::Value StartSamplingHeapProfiler(const Napi::CallbackInfo &info);
Napi::Value StopSamplingHeapProfiler(const Napi::CallbackInfo &info);
Napi::Value GetAllocationProfile(const Napi::CallbackInfo &info);
