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

import delay from 'delay';
import * as time from '../src/time-profiler';

import {Profile} from './decode';
import {verifySample, verifyValueType} from './verifiers';

const assert = require('assert');

const PROFILE_OPTIONS = {
  durationMillis: 500,
  intervalMicros: 1000,
};

describe('Time Profiler', () => {
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  let timer: any;

  function task() {
    let sum = 0;
    for (let i = 0; i < 1e6; i++) {
      sum += i;
    }
    timer = setTimeout(task, 10, sum);
  }

  beforeEach(() => {
    timer = setTimeout(task, 1);
  });

  afterEach(() => {
    clearTimeout(timer);
  });

  it('should profile during duration and finish profiling after duration', async () => {
    let isProfiling = true;
    time.profile(PROFILE_OPTIONS).then(() => {
      isProfiling = false;
    });
    await delay(2 * PROFILE_OPTIONS.durationMillis);
    assert.strictEqual(false, isProfiling, 'profiler is still running');
  });

  it('should return a profile equal to the expected profile', async () => {
    const buffer = await time.profile(PROFILE_OPTIONS);
    const profile = Profile.decode(buffer);

    // Verify nano timestamp is close to JS millis timestamp
    // This will lose resolution, but there's delays between here and the
    // capture point anyway, so it should be fine to verify this way.
    assert.ok(Date.now() - Number(profile.timeNanos! / BigInt(1000000)) < 100);
    assert.ok(profile.durationNanos! > 0);

    verifyValueType(profile, profile.periodType!, 'wall', 'nanoseconds');

    assert.strictEqual(profile.sampleType!.length, 2);
    verifyValueType(profile, profile.sampleType![0], 'sample', 'count');
    verifyValueType(profile, profile.sampleType![1], 'wall', 'nanoseconds');

    // This relies on internal naming from Node.js core. This could break if
    // the function is renamed or refactored away in Node.js core. The name
    // is a good fit because there's a high likelihood it will trigger often
    // in the time-based load loop we're using for test data.
    verifySample(profile, 'processTimers');
  });
});
