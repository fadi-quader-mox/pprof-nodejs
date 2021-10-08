'use strict'

const { execSync } = require('child_process')
const { existsSync } = require('fs')
const { join } = require('path')

let path = join(__dirname, '../build/Release/pprof-test')

if (!existsSync(path)) {
  path = join(__dirname, '../build/Debug/pprof-test')
}

execSync(path, {
  stdio: 'inherit'
})
