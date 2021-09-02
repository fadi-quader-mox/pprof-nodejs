const { createWriteStream } = require('fs')
const { dependencies, devDependencies } = require('../package.json')

const file = createWriteStream('LICENSE-3rdparty.csv')
file.write('Component,Origin,License,Copyright\n')

function copyright ({ author, contributors } = {}) {
  if (typeof author === 'object') {
    author = author.name
  }
  if (!author && Array.isArray(contributors)) {
    author = contributors.map(c => c.name)[0]
    if (contributors.length > 1) {
      author += ' and others'
    }
  }
  if (!author) {
    author = 'unknown authors'
  }
  return `Copyright ${author}`
}

for (const dependency of Object.keys(dependencies)) {
  const pkg = require(`../node_modules/${dependency}/package.json`)
  file.write(`require,${dependency},${pkg.license},${copyright(pkg)}\n`)
}

for (const dependency of Object.keys(devDependencies)) {
  const pkg = require(`../node_modules/${dependency}/package.json`)
  file.write(`dev,${dependency},${pkg.license},${copyright(pkg)}\n`)
}

file.end()
