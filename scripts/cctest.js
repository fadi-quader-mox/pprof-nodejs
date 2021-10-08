'use strict'

const { execSync } = require('child_process')
const { existsSync } = require('fs')
const { join } = require('path')

let path = join(__dirname, '../build/Release/pprof-test')

if (!existsSync(path)) {
  path = join(__dirname, '../build/Debug/pprof-test')
}

if (!existsSync(path)) {
  // eslint-disable-next-line no-console
  console.error('No pprof-test build found')
  process.exit(1)
}

execSync(path, {
  stdio: 'inherit'
})
