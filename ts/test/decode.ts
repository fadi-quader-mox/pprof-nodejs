const kTypeMask = 0b111;

const kTypeVarInt = 0;
const kTypeLengthDelim = 2;

function decodeFieldFlag(value: number) {
  const field = value >> 3;
  const mode = value & kTypeMask;
  return {field, mode};
}

function decodeBigNumber(buffer: Buffer): bigint {
  let value = BigInt(0);
  for (let i = 0; i < buffer.length; i++) {
    value |= BigInt(buffer[i] & 0b01111111) << BigInt(7 * i);
  }
  return value;
}

function decodeBigNumbers<T extends number & bigint>(buffer: Buffer): Array<T> {
  const values: Array<T> = [];
  let start = 0;

  for (let i = 0; i < buffer.length; i++) {
    if ((buffer[i] & 0b10000000) === 0) {
      values.push(decodeBigNumber(buffer.slice(start, i + 1)) as T);
      start = i + 1;
    }
  }

  return values;
}

function decodeNumber(buffer: Buffer): number {
  return Number(decodeBigNumber(buffer));
}

function decodeNumbers(buffer: Buffer): Array<number> {
  return decodeBigNumbers(buffer).map(Number);
}

function getValue(mode: number, buffer: Buffer) {
  switch (mode) {
    case kTypeVarInt:
      for (let i = 0; i < buffer.length; i++) {
        if (!(buffer[i] & 0b10000000)) {
          return buffer.slice(0, i + 1);
        }
      }
      return buffer;
    case kTypeLengthDelim:
      return buffer.slice(1, buffer[0] + 1);
    default:
      throw new Error(`Unrecognized value type: ${mode}`);
  }
}

function push<T>(value: T, list?: Array<T>) {
  return Array.isArray(list) ? list.concat(value) : [value];
}

export interface IValueType {
  type?: number;
  unit?: number;
}

export class ValueType implements IValueType {
  // 1
  type?: number;
  // 2
  unit?: number;

  constructor(data: IValueType) {
    this.type = data.type || 0;
    this.unit = data.unit || 0;
  }

  static decodeValue(data: IValueType, field: number, buffer: Buffer) {
    switch (field) {
      case 1:
        // TODO: parse numebers properly...
        data.type = decodeNumber(buffer);
        break;
      case 2:
        // TODO: parse numebers properly...
        data.unit = decodeNumber(buffer);
        break;
    }
  }

  static decode(buffer: Buffer): ValueType {
    const data: IValueType = {};
    let index = 0;

    while (index < buffer.length) {
      const {field, mode} = decodeFieldFlag(buffer[index]);
      index++;

      const value = getValue(mode, buffer.slice(index));
      index += value.length + (mode === kTypeLengthDelim ? 1 : 0);

      this.decodeValue(data, field, value);
    }

    return new ValueType(data);
  }
}

export interface ILabel {
  // 1
  key?: number;
  // 2
  str?: number;
  // 3
  num?: number;
  // 4
  num_unit?: number;
}

export class Label implements ILabel {
  key?: number;
  str?: number;
  num?: number;
  num_unit?: number;

  constructor(data: ILabel) {
    this.key = data.key || 0;
    this.str = data.str || 0;
    this.num = data.num || 0;
    this.num_unit = data.num_unit || 0;
  }

  static decodeValue(data: ILabel, field: number, buffer: Buffer) {
    switch (field) {
      case 1:
        data.key = decodeNumber(buffer);
        break;
      case 2:
        data.str = decodeNumber(buffer);
        break;
      case 3:
        data.num = decodeNumber(buffer);
        break;
      case 4:
        data.num_unit = decodeNumber(buffer);
        break;
    }
  }

  static decode(buffer: Buffer): Label {
    const data: ILabel = {};
    let index = 0;

    while (index < buffer.length) {
      const {field, mode} = decodeFieldFlag(buffer[index]);
      index++;

      const value = getValue(mode, buffer.slice(index));
      index += value.length + (mode === kTypeLengthDelim ? 1 : 0);

      this.decodeValue(data, field, value);
    }

    return new Label(data);
  }
}

export interface ISample {
  // 1
  locationId?: Array<number>;
  // 2
  value?: Array<bigint>;
  // 3
  label?: Array<Label>;
}

export class Sample implements ISample {
  locationId?: Array<number>;
  value?: Array<bigint>;
  label?: Array<Label>;

  constructor(data: ISample) {
    this.locationId = data.locationId;
    this.value = data.value;
    this.label = data.label;
  }

  static decodeValue(data: ISample, field: number, buffer: Buffer) {
    switch (field) {
      case 1:
        data.locationId = decodeNumbers(buffer);
        break;
      case 2:
        data.value = decodeBigNumbers(buffer);
        break;
      case 3:
        data.label = push(Label.decode(buffer), data.label);
        break;
    }
  }

  static decode(buffer: Buffer): Sample {
    const data: ISample = {};
    let index = 0;

    while (index < buffer.length) {
      const {field, mode} = decodeFieldFlag(buffer[index]);
      index++;

      const value = getValue(mode, buffer.slice(index));
      index += value.length + (mode === kTypeLengthDelim ? 1 : 0);

      this.decodeValue(data, field, value);
    }

    return new Sample(data);
  }
}

export interface IMapping {
  // 1
  id?: number;
  // 2
  memory_start?: number;
  // 3
  memory_limit?: number;
  // 4
  file_offset?: number;
  // 5
  filename?: number;
  // 6
  build_id?: number;
  // 7
  has_functions?: boolean;
  // 8
  has_filenames?: boolean;
  // 9
  has_line_numbers?: boolean;
  // 10
  has_inline_frames?: boolean;
}

export class Mapping implements IMapping {
  id?: number;
  memory_start?: number;
  memory_limit?: number;
  file_offset?: number;
  filename?: number;
  build_id?: number;
  has_functions?: boolean;
  has_filenames?: boolean;
  has_line_numbers?: boolean;
  has_inline_frames?: boolean;

  constructor(data: IMapping) {
    this.id = data.id || 0;
    this.memory_start = data.memory_start || 0;
    this.memory_limit = data.memory_limit || 0;
    this.file_offset = data.file_offset || 0;
    this.filename = data.filename || 0;
    this.build_id = data.build_id || 0;
    this.has_functions = !!data.has_functions;
    this.has_filenames = !!data.has_filenames;
    this.has_line_numbers = !!data.has_line_numbers;
    this.has_inline_frames = !!data.has_inline_frames;
  }

  static decodeValue(data: IMapping, field: number, buffer: Buffer) {
    switch (field) {
      case 1:
        data.id = decodeNumber(buffer);
        break;
      case 2:
        data.memory_start = decodeNumber(buffer);
        break;
      case 3:
        data.memory_limit = decodeNumber(buffer);
        break;
      case 4:
        data.file_offset = decodeNumber(buffer);
        break;
      case 5:
        data.filename = decodeNumber(buffer);
        break;
      case 6:
        data.build_id = decodeNumber(buffer);
        break;
      case 7:
        data.has_functions = !!decodeNumber(buffer);
        break;
      case 8:
        data.has_filenames = !!decodeNumber(buffer);
        break;
      case 9:
        data.has_line_numbers = !!decodeNumber(buffer);
        break;
      case 10:
        data.has_inline_frames = !!decodeNumber(buffer);
        break;
    }
  }

  static decode(buffer: Buffer): Mapping {
    const data: IMapping = {};
    let index = 0;

    while (index < buffer.length) {
      const {field, mode} = decodeFieldFlag(buffer[index]);
      index++;

      const value = getValue(mode, buffer.slice(index));
      index += value.length + (mode === kTypeLengthDelim ? 1 : 0);

      this.decodeValue(data, field, value);
    }

    return new Mapping(data);
  }
}

export interface ILine {
  // 1
  function_id?: number;
  // 2
  line?: number;
}

export class Line implements ILine {
  function_id?: number;
  line?: number;

  constructor(data: ILine) {
    this.function_id = data.function_id || 0;
    this.line = data.line || 0;
  }

  static decodeValue(data: ILine, field: number, buffer: Buffer) {
    switch (field) {
      case 1:
        // TODO: parse numebers properly...
        data.function_id = decodeNumber(buffer);
        break;
      case 2:
        // TODO: parse numebers properly...
        data.line = decodeNumber(buffer);
        break;
    }
  }

  static decode(buffer: Buffer): Line {
    const data: ILine = {};
    let index = 0;

    while (index < buffer.length) {
      const {field, mode} = decodeFieldFlag(buffer[index]);
      index++;

      const value = getValue(mode, buffer.slice(index));
      index += value.length + (mode === kTypeLengthDelim ? 1 : 0);

      this.decodeValue(data, field, value);
    }

    return new Line(data);
  }
}

export interface ILocation {
  // 1
  id?: number;
  // 2
  mapping_id?: number;
  address?: number;
  line?: Array<Line>;
  is_folded?: boolean;
}

export class Location implements ILocation {
  id?: number;
  mapping_id?: number;
  address?: number;
  line?: Array<Line>;
  is_folded?: boolean;

  constructor(data: ILocation) {
    this.id = data.id || 0;
    this.mapping_id = data.mapping_id || 0;
    this.address = data.address || 0;
    this.line = data.line;
    this.is_folded = !!data.is_folded;
  }

  static decodeValue(data: ILocation, field: number, buffer: Buffer) {
    switch (field) {
      case 1:
        data.id = decodeNumber(buffer);
        break;
      case 2:
        data.mapping_id = decodeNumber(buffer);
        break;
      case 3:
        data.address = decodeNumber(buffer);
        break;
      case 4:
        data.line = push(Line.decode(buffer), data.line);
        break;
      case 5:
        data.is_folded = !!decodeNumber(buffer);
        break;
    }
  }

  static decode(buffer: Buffer): Location {
    const data: ILocation = {};
    let index = 0;

    while (index < buffer.length) {
      const {field, mode} = decodeFieldFlag(buffer[index]);
      index++;

      const value = getValue(mode, buffer.slice(index));
      index += value.length + (mode === kTypeLengthDelim ? 1 : 0);

      this.decodeValue(data, field, value);
    }

    return new Location(data);
  }
}

export interface IFunction {
  // 1
  id?: number;
  // 2
  name?: number;
  // 3
  system_name?: number;
  // 4
  filename?: number;
  // 5
  start_line?: number;
}

export class Function implements IFunction {
  id?: number;
  name?: number;
  system_name?: number;
  filename?: number;
  start_line?: number;

  constructor(data: IFunction) {
    this.id = data.id || 0;
    this.name = data.name || 0;
    this.system_name = data.system_name || 0;
    this.filename = data.filename || 0;
    this.start_line = data.start_line || 0;
  }

  static decodeValue(data: IFunction, field: number, buffer: Buffer) {
    switch (field) {
      case 1:
        data.id = decodeNumber(buffer);
        break;
      case 2:
        data.name = decodeNumber(buffer);
        break;
      case 3:
        data.system_name = decodeNumber(buffer);
        break;
      case 4:
        data.filename = decodeNumber(buffer);
        break;
      case 5:
        data.start_line = decodeNumber(buffer);
        break;
    }
  }

  static decode(buffer: Buffer): Function {
    const data: IFunction = {};
    let index = 0;

    while (index < buffer.length) {
      const {field, mode} = decodeFieldFlag(buffer[index]);
      index++;

      const value = getValue(mode, buffer.slice(index));
      index += value.length + (mode === kTypeLengthDelim ? 1 : 0);

      this.decodeValue(data, field, value);
    }

    return new Function(data);
  }
}

export interface IProfile {
  // 1
  sampleType?: Array<ValueType>;
  // 2
  sample?: Array<Sample>;
  // 3
  mapping?: Array<Mapping>;
  // 4
  location?: Array<Location>;
  // 5
  function?: Array<Function>;
  // 6
  stringTable?: Array<string>;
  // 7
  dropFrames?: number;
  // 8
  keepFrames?: number;
  // 9
  timeNanos?: bigint;
  // 10
  durationNanos?: bigint;
  // 11
  periodType?: ValueType;
  // 12
  period?: number;
  // 13
  comment?: Array<number>;
  // 14
  default_sample_type?: number;
}

export class Profile implements IProfile {
  sampleType?: Array<ValueType>;
  sample?: Array<Sample>;
  mapping?: Array<Mapping>;
  location?: Array<Location>;
  function?: Array<Function>;
  stringTable?: Array<string>;
  dropFrames?: number;
  keepFrames?: number;
  timeNanos?: bigint;
  durationNanos?: bigint;
  periodType?: ValueType;
  period?: number;
  comment?: Array<number>;
  default_sample_type?: number;

  static fields = [
    'sampleType',
    'sample',
    'mapping',
    'location',
    'function',
    'stringTable',
    'dropFrames',
    'keepFrames',
    'timeNanos',
    'durationNanos',
    'periodType',
    'period',
    'comment',
    'default_sample_type',
  ];

  constructor(data: IProfile) {
    this.sampleType = data.sampleType;
    this.sample = data.sample;
    this.mapping = data.mapping;
    this.location = data.location;
    this.function = data.function;
    this.stringTable = data.stringTable;
    this.dropFrames = data.dropFrames || 0;
    this.keepFrames = data.keepFrames || 0;
    this.timeNanos = data.timeNanos || BigInt(0);
    this.durationNanos = data.durationNanos || BigInt(0);
    this.periodType = data.periodType;
    this.period = data.period || 0;
    this.comment = data.comment;
    this.default_sample_type = data.default_sample_type || 0;
  }

  static decodeValue(data: IProfile, field: number, buffer: Buffer) {
    switch (field) {
      case 1:
        data.sampleType = push(ValueType.decode(buffer), data.sampleType);
        break;
      case 2:
        data.sample = push(Sample.decode(buffer), data.sample);
        break;
      case 3:
        data.mapping = push(Mapping.decode(buffer), data.mapping);
        break;
      case 4:
        data.location = push(Location.decode(buffer), data.location);
        break;
      case 5:
        data.function = push(Function.decode(buffer), data.function);
        break;
      case 6:
        data.stringTable = push(buffer.toString('utf-8'), data.stringTable);
        break;
      case 7:
        data.dropFrames = decodeNumber(buffer);
        break;
      case 8:
        data.keepFrames = decodeNumber(buffer);
        break;
      case 9:
        data.timeNanos = decodeBigNumber(buffer) as bigint;
        break;
      case 10:
        data.durationNanos = decodeBigNumber(buffer);
        break;
      case 11:
        data.periodType = ValueType.decode(buffer);
        break;
      case 12:
        data.period = decodeNumber(buffer);
        break;
      case 13:
        data.comment = push(decodeNumber(buffer), data.comment);
        break;
      case 14:
        data.default_sample_type = decodeNumber(buffer);
        break;
    }
  }

  static decode(buffer: Buffer): Profile {
    const data: IProfile = {};
    let index = 0;

    while (index < buffer.length) {
      const {field, mode} = decodeFieldFlag(buffer[index]);
      index++;

      const value = getValue(mode, buffer.slice(index));
      index += value.length + (mode === kTypeLengthDelim ? 1 : 0);

      this.decodeValue(data, field, value);
    }

    return new Profile(data);
  }
}
