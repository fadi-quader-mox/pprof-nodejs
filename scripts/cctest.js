'use strict'

const { execSync } = require('child_process')
const { existsSync } = require('fs')
const { join } = require('path')

function findBuild (mode) {
  const path = join(__dirname, '..', 'build', mode, 'pprof-test')
  return existsSync(path) && path
}

const path = findBuild('Release') || findBuild('Debug')
if (!path) {
  // eslint-disable-next-line no-console
  console.error('No pprof-test build found')
  process.exit(1)
}

execSync(path, {
  stdio: 'inherit'
})
