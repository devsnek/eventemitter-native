{
  "targets": [
    {
      "target_name": "EventEmitter",
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "sources": [ "src/addon.cc", "src/EventEmitter.cc" ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      'dependencies': ["<!(node -p \"require('node-addon-api').gyp\")"],
      "defines": [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ],
    }
  ]
}
