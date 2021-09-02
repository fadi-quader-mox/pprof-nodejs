{
  "targets": [
    {
      "target_name": "dd-pprof",
      "sources": [ 
        "bindings/profiler.cc",
        "bindings/promise-worker.cc",
        "bindings/time-profiler.cc",
        "bindings/heap-profiler.cc",
        "bindings/helpers.cc",
        "bindings/pprof.cc"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ],
      "xcode_settings": {
        "MACOSX_DEPLOYMENT_TARGET": "10.10",
        'CLANG_CXX_LIBRARY': 'libc++',
        "OTHER_CFLAGS": [
          "-std=c++14",
          "-stdlib=libc++",
          "-Wall",
          "-Werror"
        ]
      },
      "conditions": [
        ["OS == 'linux'", {
          "cflags": [
            "-std=c++14",
            "-Wall",
            "-Werror"
          ],
          "cflags_cc": [
            "-Wno-cast-function-type"
          ]
        }],
        ["OS == 'win'", {
          "cflags": [
            "/WX"
          ]
        }]
      ]
    },
    {
      "target_name": "pprof-test",
      'type': 'executable',
      "sources": [
        "bindings/pprof.cc",
        "bindings/pprof.test.cc"
      ],
      "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ],
      "xcode_settings": {
        "MACOSX_DEPLOYMENT_TARGET": "10.10",
        'CLANG_CXX_LIBRARY': 'libc++',
        "OTHER_CFLAGS": [
          "-std=c++14",
          "-stdlib=libc++",
          "-Wall",
          "-Werror"
        ]
      },
      "conditions": [
        ["OS == 'linux'", {
          "cflags": [
            "-std=c++14",
            "-Wall",
            "-Werror"
          ],
          "cflags_cc": [
            "-Wno-cast-function-type"
          ]
        }],
        ["OS == 'win'", {
          "cflags": [
            "/WX"
          ]
        }]
      ]
    }
  ]
}
