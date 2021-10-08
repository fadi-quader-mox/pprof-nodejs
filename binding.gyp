{
  'variables': {
    'asan%': 0,
    'lsan%': 0,
    'ubsan%': 0,
  },

  'conditions': [
    # No macOS support for -fsanitize=leak
    ['lsan == "true" and OS != "mac"', {
      'cflags+': ['-fsanitize=leak'],
      'ldflags': ['-fsanitize=leak'],
    }],
    ['asan == "true" and OS != "mac"', {
      'cflags+': [
        '-fno-omit-frame-pointer',
        '-fsanitize=address',
        '-fsanitize-address-use-after-scope',
      ],
      'cflags!': [ '-fomit-frame-pointer' ],
      'ldflags': [ '-fsanitize=address' ],
    }],
    ['asan == "true" and OS == "mac"', {
      'xcode_settings': {
        'OTHER_CFLAGS+': [
          '-fno-omit-frame-pointer',
          '-gline-tables-only',
          '-fsanitize=address'
        ],
        'OTHER_CFLAGS!': [
          '-fomit-frame-pointer',
        ],
        'OTHER_LDFLAGS': [
          '-fsanitize=address'
        ],
      },
    }],
    # UBSAN
    ['ubsan == "true" and OS != "mac"', {
      'cflags+': [
        '-fsanitize=undefined,alignment,bounds',
        '-fno-sanitize-recover',
      ],
      'ldflags': ['-fsanitize=undefined,alignment,bounds'],
    }],
    ['ubsan == "true" and OS == "mac"', {
      'xcode_settings': {
        'OTHER_CFLAGS+': [
          '-fsanitize=undefined,alignment,bounds',
          '-fno-sanitize-recover'
        ],
        'OTHER_LDFLAGS': [
          '-fsanitize=undefined,alignment,bounds'
        ],
      },
    }],
  ],

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
        "MACOSX_DEPLOYMENT_TARGET": "10.15",
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
  ],
}
