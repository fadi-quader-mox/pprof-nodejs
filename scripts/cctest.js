'use strict'

const { execSync } = require('child_process')
const { existsSync } = require('fs')
const { platform } = require('os')
const { join } = require('path')

function findBuild (mode) {
  let path = join(__dirname, '..', 'build', mode, 'pprof-test')
  if (platform() === 'win32') path += '.exe'
  if (!existsSync(path)) {
  // eslint-disable-next-line no-console
    console.warn(`No pprof-test binary found at: ${path}`)
  }
  return path
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
