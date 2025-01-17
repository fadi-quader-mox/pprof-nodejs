'use strict';

/* eslint-disable no-console */

const npmArgv = (() => {
  try {
    return JSON.parse(process.env.npm_config_argv);
  } catch (e) {
    return {original: []};
  }
})();

const path = require('path');
const requirePackageJson = require('./require-package-json.js');
const packageJson = requirePackageJson(path.join(__dirname, '..'));

const nodeMajor = Number(process.versions.node.split('.')[0]);

const min = Number(packageJson.engines.node.match(/\d+/)[0]);

const hasIgnoreEngines =
  npmArgv && npmArgv.original && npmArgv.original.includes('--ignore-engines');

if (nodeMajor < min && !hasIgnoreEngines) {
  process.exitCode = 1;
  console.error(
    '\n' +
      `
You're using Node.js v${process.versions.node}, which is not supported by
${packageJson.name}.

Please upgrade to a more recent version of Node.js.
  `.trim() +
      '\n'
  );
}
