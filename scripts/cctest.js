'use strict'

const { execSync } = require('child_process')
const { join } = require('path')

execSync(join(__dirname, '../build/Release/pprof-test'), {
  stdio: 'inherit'
})
