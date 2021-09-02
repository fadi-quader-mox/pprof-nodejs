# pprof support for Node.js

[![NPM Version][npm-image]][npm-url]
[![Build Status][circle-image]][circle-url]
[![Known Vulnerabilities][snyk-image]][snyk-url]

[pprof][pprof-url] support for Node.js.

## Prerequisites
1. Your application will need to be using Node.js 12 or greater. 

2. The `pprof` module has a native component that is used to collect profiles 
with v8's CPU and Heap profilers. Prebuilds are provided for Linux with both
glibc and musl, macOS, and both 32-bit and 64-bit Windows. No build tools
should be required to install and use this module

3. The [`pprof`][pprof-url] CLI can be used to view profiles collected with 
this module. Instructions for installing the `pprof` CLI can be found
[here][pprof-install-url].

## Basic Set-up

Install [`pprof`][npm-url] with `npm` or add to your `package.json`.
  ```sh
  # Install through npm while saving to the local 'package.json'
  npm install --save @datadog/pprof
  ```

## Using the Profiler

### Collect a Wall Time Profile

#### In code:
1. Update code to collect and save a profile:
    ```javascript
    const profile = await pprof.time.profile({
      durationMillis: 10000,    // time in milliseconds for which to 
                                // collect profile.
    });
    fs.writeFile('wall.pb.gz', profile, (err) => {
      if (err) throw err;
    });
    ```

2. View the profile with command line [`pprof`][pprof-url]:
    ```sh
    pprof -http=: wall.pb.gz
    ```

#### Requiring from the command line

1. Start program from the command line:
    ```sh
    node --require @datadog/pprof app.js
    ```

2. A wall time profile for the job will be saved in 
`pprof-profile-${process.pid}.pb.gz`. View the profile with command line 
[`pprof`][pprof-url]:
    ```sh
    pprof -http=: pprof-profile-${process.pid}.pb.gz
    ```

### Collect a Heap Profile
1. Enable heap profiling at the start of the application:
    ```javascript
    // The average number of bytes between samples.
    const intervalBytes = 512 * 1024;

    // The maximum stack depth for samples collected.
    const stackDepth = 64;

    heap.start(intervalBytes, stackDepth); 
    ```
2. Collect heap profiles:
  
    * Collecting and saving a profile in profile.proto format:
        ```javascript
        const profile = await pprof.heap.profile();
        fs.writeFile('heap.pb.gz', profile, (err) => {
          if (err) throw err;
        })
        ```

    * View the profile with command line [`pprof`][pprof-url].
        ```sh
        pprof -http=: heap.pb.gz
        ```
    
    * Collecting a heap profile with  V8 allocation profile format:
        ```javascript
          const profile = await pprof.heap.v8Profile();
        ``` 

[circle-image]: https://circleci.com/gh/datadog/pprof-nodejs.svg?style=svg
[circle-url]: https://circleci.com/gh/datadog/pprof-nodejs
[coveralls-image]: https://coveralls.io/repos/datadog/pprof-nodejs/badge.svg?branch=main&service=github
[npm-image]: https://badge.fury.io/js/pprof.svg
[npm-url]: https://npmjs.org/package/pprof
[pprof-url]: https://github.com/google/pprof
[pprof-install-url]: https://github.com/google/pprof#building-pprof
[snyk-image]: https://snyk.io/test/github/datadog/pprof-nodejs/badge.svg
[snyk-url]: https://snyk.io/test/github/datadog/pprof-nodejs
