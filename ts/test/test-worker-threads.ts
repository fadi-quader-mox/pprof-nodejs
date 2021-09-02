// eslint-disable-next-line node/no-unsupported-features/node-builtins
import {spawn} from 'child_process';

const assert = require('assert');

describe('Worker Threads', () => {
  // eslint-ignore-next-line prefer-array-callback
  it('should work when propagated to a worker through -r flag', function (done) {
    this.timeout(5000);
    const stdoutChunks: Array<Buffer> = [];
    const stderrChunks: Array<Buffer> = [];

    const proc = spawn('node', ['./out/test/worker.js']);

    proc.stdout.on('data', chunk => {
      stdoutChunks.push(chunk);
    });
    proc.stderr.on('data', chunk => {
      // Make sure to write errors to stderr, makes debugging less bad. :O
      process.stderr.write(chunk);
      stderrChunks.push(chunk);
    });

    proc.on('close', () => {
      const stdout = Buffer.concat(stdoutChunks).toString();
      assert.strictEqual(stdout, 'it works!\nit works!\n');

      const stderr = Buffer.concat(stderrChunks).toString();
      assert.strictEqual(stderr, '');

      done();
    });
  });
});
