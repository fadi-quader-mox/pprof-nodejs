// eslint-disable-next-line node/no-unsupported-features/node-builtins
import {Worker, isMainThread} from 'worker_threads';
import {time} from '../src/index';

import {Profile, ValueType} from './decode';

const assert = require('assert');

const {hasOwnProperty} = Object.prototype;

if (isMainThread) {
  new Worker(__filename);
}

function valueName(profile: Profile, value_type: ValueType) {
  const type = getAndVerifyString(profile.stringTable!, value_type, 'type');
  const unit = getAndVerifyString(profile.stringTable!, value_type, 'unit');
  return `${type}/${unit}`;
}

function sampleName(profile: Profile, sampleType: Array<ValueType>) {
  return sampleType.map(valueName.bind(null, profile));
}

// eslint-disable-next-line @typescript-eslint/no-explicit-any
function getAndVerifyPresence(list: any[], id: number, zeroIndex = false) {
  assert.strictEqual(typeof id, 'number', 'has id');
  const index = id - (zeroIndex ? 0 : 1);
  assert.ok(list.length > index, 'exists in list');
  return list[index];
}

// eslint-disable-next-line @typescript-eslint/no-explicit-any
function getAndVerifyString(stringTable: string[], source: any, field: string) {
  assert.ok(hasOwnProperty.call(source, field), 'has id field');
  const str = getAndVerifyPresence(stringTable, source[field] as number, true);
  assert.strictEqual(typeof str, 'string', 'is a string');
  return str;
}

let stopped = false;

function load() {
  let sum = 0;
  for (let i = 0; i < 100; i++) {
    sum += i;
  }
  if (!stopped) {
    setImmediate(load, sum);
  }
}
load();

time
  .profile({
    durationMillis: 2000,
    intervalMicros: 100
  })
  .finally(() => {
    stopped = true;
  })
  .then(buffer => {
    const profile = Profile.decode(buffer);

    assert.deepStrictEqual(
      sampleName(profile, profile.sampleType!),
      ['sample/count', 'wall/nanoseconds'],
      'sample types are equal'
    );
    assert.strictEqual(
      typeof profile.timeNanos,
      'bigint',
      'time nanos is a bigint'
    );
    assert.strictEqual(
      typeof profile.durationNanos,
      'bigint',
      'duration nanos is a bigint'
    );
    assert.strictEqual(typeof profile.period, 'number', 'period is a number');
    assert.strictEqual(
      valueName(profile, profile.periodType!),
      'wall/nanoseconds',
      'period type is wall/nanoseconds'
    );

    assert.ok(profile.sample, 'profile has a sample list');
    assert.ok(profile.sample!.length, 'profile has samples');
    for (const sample of profile.sample!) {
      assert.ok(sample.value, 'sample has a value list');
      assert.strictEqual(sample.value!.length, 2, 'sample has two values');
      for (const value of sample.value!) {
        assert.strictEqual(typeof value, 'bigint', 'sample value is a bigint');
      }

      assert.ok(sample.locationId, 'sample has a location id list');
      assert.ok(sample.locationId!.length, 'sample has location ids');
      for (const locationId of sample.locationId!) {
        const location = getAndVerifyPresence(
          profile.location!,
          locationId as number
        );
        assert.ok(location, 'found location by id');

        assert.ok(location.line, 'location has a line list');
        assert.strictEqual(location.line!.length, 1, 'location has one line');
        for (const {function_id, line} of location.line!) {
          const fn = getAndVerifyPresence(
            profile.function!,
            function_id as number
          );
          assert.ok(fn, 'found function by id');

          getAndVerifyString(profile.stringTable!, fn, 'name');
          getAndVerifyString(profile.stringTable!, fn, 'system_name');
          getAndVerifyString(profile.stringTable!, fn, 'filename');
          assert.strictEqual(typeof line, 'number', 'line has line number');
        }
      }
    }

    console.log('it works!');
  })
  .catch(err => {
    console.error(err.stack);
    process.exitCode = 1;
  });
