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

#include <algorithm>
#include <utility>
#include <sstream>
#include <map>

#include "heap-profiler.h"

// Statically allocate some things that will never change
static const std::string native_string = "<native>";
static const std::string anonymous_string = "(anonymous)";
static const std::string external_string = "(external)";

static const pprof::ValueType objects_count("objects", "count");
static const pprof::ValueType space_bytes("space", "bytes");

static const pprof::Location external_location =
  GetLocation(external_string, external_string, "", 0);

AllocationNode::AllocationNode(v8::Isolate* isolate,
  v8::AllocationProfile::Node* node)
  : name(*v8::String::Utf8Value(isolate, node->name)),
    script_name(*v8::String::Utf8Value(isolate, node->script_name)),
    line_number(node->line_number),
    allocations(node->allocations) {
  std::transform(node->children.begin(),
                 node->children.end(),
                 std::back_inserter(children),
                 [isolate](v8::AllocationProfile::Node* child)
                 -> std::shared_ptr<AllocationNode> {
                   return std::make_shared<AllocationNode>(isolate, child);
                 });
}

HeapProfileEncoder::HeapProfileEncoder(const Napi::Env& env,
  std::unique_ptr<AllocationNode> node, uint32_t interval)
  : PromiseWorker(env),
    root(std::move(node)),
    intervalBytes(interval),
    startTime(now()) {
  v8::HeapStatistics v8_heap_stats;
  v8::Isolate::GetCurrent()->GetHeapStatistics(&v8_heap_stats);
  externalMemory = v8_heap_stats.external_memory();
}

void HeapProfileEncoder::Execute() {
  pprof::Profile profile(objects_count, space_bytes);
  profile.time_nanos = startTime;
  profile.period = intervalBytes;

  // Add root children to queue with empty stacks
  std::map<std::shared_ptr<AllocationNode>,
           std::vector<pprof::Location>> nodeStacks;
  std::vector<std::shared_ptr<AllocationNode>> children;
  for (auto child : root->children) {
    nodeStacks[child] = {};
    children.push_back(child);
  }

  // Capture (external) allocations
  profile.add_sample(MakeSample({external_location}, externalMemory, 1));

  // Process node queue
  while (children.size() > 0) {
    // Get last node
    auto node = children.back();
    children.pop_back();

    // Find function and script names
    auto scriptName = fallback(node->script_name, native_string);
    auto name = fallback(node->name, anonymous_string);

    // Create call location for this sample
    auto location =
      GetLocation(name, name, scriptName, node->line_number);

    // Add location to the stack for this node
    auto stack = nodeStacks[node];
    stack.emplace(stack.begin(), location);

    // Create heap sample
    for (auto alloc : node->allocations) {
      profile.add_sample(
        MakeSample(stack, alloc.count, alloc.size * alloc.count));
    }

    // Add node children to queue with a copy of the current stack
    for (auto child : node->children) {
      nodeStacks[child] = stack;
      children.push_back(child);
    }
  }

  // Encode to pprof buffer
  output = pprof::Encoder().encode(profile);
}

void HeapProfileEncoder::OnOK() {
  Napi::Env env = Env();
  Napi::HandleScope scope(env);
  Resolve(Napi::Buffer<char>::Copy(env, output.data(), output.size()));
}

Napi::Value StartSamplingHeapProfiler(const Napi::CallbackInfo& info) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  Napi::Env env = info.Env();

  if (info.Length() == 2) {
    if (!info[0].IsNumber()) {
      Napi::TypeError::New(env, "First argument type must be uint32.")
          .ThrowAsJavaScriptException();
      return env.Null();
    }
    if (!info[1].IsNumber()) {
      Napi::TypeError::New(env, "First argument type must be Integer.")
          .ThrowAsJavaScriptException();
      return env.Null();
    }

    uint64_t sample_interval = info[0].As<Napi::Number>().DoubleValue();
    int stack_depth = info[1].As<Napi::Number>().Int32Value();

    isolate->GetHeapProfiler()->StartSamplingHeapProfiler(
        sample_interval, stack_depth);
  } else {
    isolate->GetHeapProfiler()->StartSamplingHeapProfiler();
  }
  return env.Null();
}

Napi::Value StopSamplingHeapProfiler(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  isolate->GetHeapProfiler()->StopSamplingHeapProfiler();
  return env.Null();
}

Napi::Value GetAllocationProfile(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto isolate = v8::Isolate::GetCurrent();

  if (info.Length() != 1) {
    Napi::TypeError::New(env, "getAllocationProfile must have two arguments.")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  if (!info[0].IsNumber()) {
    Napi::TypeError::New(env, "Interval bytes must be a number.")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  uint32_t intervalBytes = info[0].As<Napi::Number>().Uint32Value();

  // Capture heap profile
  auto profiler = isolate->GetHeapProfiler();
  std::unique_ptr<v8::AllocationProfile> heap(
    profiler->GetAllocationProfile());
  if (heap == nullptr) {
    Napi::TypeError::New(env,
      "Must start heap profiler before capturing a profile")
      .ThrowAsJavaScriptException();
    return env.Null();
  }

  // Transform non-thread-safe allocation profile into a thread-safe type
  std::unique_ptr<AllocationNode> root =
    std::make_unique<AllocationNode>(isolate, heap->GetRootNode());

  // Create a promise worker to encoder the allocation profile
  // NOTE: This instance is managed, so don't `delete` it.
  HeapProfileEncoder* encoder =
    new HeapProfileEncoder(env, std::move(root), intervalBytes);

  encoder->Queue();

  return encoder->Promise();
}
