# Contributing to @datadog/pprof

Please reach out before starting work on any major code changes.
This will ensure we avoid duplicating work, or that your code can't be merged due to a rapidly changing
base. If you would like support for a module that is not listed, [contact support][1] to share a request.

[1]: https://docs.datadoghq.com/help

## Local setup

To set up the project locally, you can install it with:
```
$ npm install
```

### Build

Build TypeScript code with:
```
$ npm run compile
```

Build C++ code with:
```
$ npm run rebuild
```

Test the JavaScript interface with:
```
$ npm test
```

Test the C++ pprof encoder with:
```
$ npm run cctest
```
