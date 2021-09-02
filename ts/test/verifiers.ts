const assert = require('assert');

import {Profile, ValueType} from './decode';

export function verifyValueType(
  profile: Profile,
  valueType: ValueType,
  type: string,
  unit: string
) {
  assert.strictEqual(valueType.type, profile.stringTable!.indexOf(type));
  assert.strictEqual(valueType.unit, profile.stringTable!.indexOf(unit));
}

export function verifySample(profile: Profile, functionName: string) {
  const functionNameId = profile.stringTable!.indexOf(functionName);

  const func = profile.function!.find(func => func.name === functionNameId)!;
  assert.ok(func);
  assert.strictEqual(func.system_name, func.name);
  assert.notStrictEqual(func.filename, '');

  const location = profile.location!.find(location => {
    return !!location.line!.find(line => line.function_id === func.id);
  })!;
  assert.ok(location);
  assert.ok(location.line!.length > 0);

  assert.ok(profile.sample!.length > 0);
  const sample = profile.sample!.find(sample => {
    return sample.locationId!.includes(location.id!);
  })!;
  assert.ok(sample);
  assert.ok(sample.locationId!.length > 0);
  assert.strictEqual(sample.value!.length, 2);
}
