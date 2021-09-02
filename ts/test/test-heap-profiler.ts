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

import * as heapProfiler from '../src/heap-profiler';

import {Profile} from './decode';
import {verifySample, verifyValueType} from './verifiers';

const assert = require('assert');

describe('HeapProfiler', () => {
  afterEach(() => {
    heapProfiler.stop();
  });

  it('should fail to profile if not started yet', () => {
    assert.rejects(
      () => heapProfiler.profile(),
      /^Error: Heap profiler is not enabled.$/
    );
  });

  it('should throw error when starting if already enabled', () => {
    assert.doesNotThrow(() => {
      heapProfiler.start(1024 * 128, 64);
    });

    assert.throws(() => {
      heapProfiler.start();
    }, /Error: Heap profiler is already started with intervalBytes \d+ and stackDepth \d+$/i);
  });

  it('should return a profile equal to the expected profile when external memory is allocated', async () => {
    heapProfiler.start();

    const buffer = await heapProfiler.profile();
    assert.ok(Buffer.isBuffer(buffer));

    const profile = Profile.decode(buffer);

    // Verify nano timestamp is close to JS millis timestamp
    // This will lose resolution, but there's delays between here and the
    // capture point anyway, so it should be fine to verify this way.
    assert.ok(Date.now() - Number(profile.timeNanos! / BigInt(1000000)) < 100);

    verifyValueType(profile, profile.periodType!, 'heap', 'bytes');

    assert.strictEqual(profile.sampleType!.length, 2);
    verifyValueType(profile, profile.sampleType![0], 'object', 'count');
    verifyValueType(profile, profile.sampleType![1], 'heap', 'bytes');

    assert.ok(profile.sample!.length > 0);
    verifySample(profile, '(external)');
  });
});
