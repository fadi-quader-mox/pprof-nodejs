/**
 * Copyright 2017 Google Inc. All Rights Reserved.
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

import {join} from 'path';

const findBinding = require('node-gyp-build');
const profiler = findBinding(join(__dirname, '..', '..'));

let enabled = false;
let heapIntervalBytes = 0;
let heapStackDepth = 0;

/**
 * Collects a profile and returns it serialized in pprof format.
 * Throws if heap profiler is not enabled.
 */
export function profile(): Promise<Buffer> {
  if (!enabled) {
    return Promise.reject(new Error('Heap profiler is not enabled.'));
  }
  return profiler.heapProfiler.getAllocationProfile(heapIntervalBytes);
}

/**
 * Starts heap profiling. If heap profiling has already been started with
 * the same parameters, this is a noop. If heap profiler has already been
 * started with different parameters, this throws an error.
 *
 * @param intervalBytes - average number of bytes between samples.
 * @param stackDepth - maximum stack depth for samples collected.
 */
export function start(intervalBytes = 1024 * 512, stackDepth = 32) {
  if (enabled) {
    throw new Error(
      `Heap profiler is already started with intervalBytes ${heapIntervalBytes} and stackDepth ${stackDepth}`
    );
  }
  heapIntervalBytes = intervalBytes;
  heapStackDepth = stackDepth;
  profiler.heapProfiler.startSamplingHeapProfiler(
    heapIntervalBytes,
    heapStackDepth
  );
  enabled = true;
}

/**
 * Stops heap profiling. If heap profiling has not been started, does nothing.
 */
export function stop() {
  if (enabled) {
    enabled = false;
    profiler.heapProfiler.stopSamplingHeapProfiler();
  }
}
