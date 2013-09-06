{
  "targets": [{
    "target_name": "mine.uv",
    "type": "executable",
    "dependencies": [
      "mine.uv-lib",
    ],
    "sources": [
      "src/main.c",
    ],
  }, {
    "target_name": "mine.uv-lib",
    "type": "<(library)",
    "include_dirs": [ "src" ],
    "dependencies": [
      "deps/uv/uv.gyp:libuv",
      "deps/openssl/openssl.gyp:openssl",
      "deps/zlib/zlib.gyp:zlib",
    ],
    "direct_dependent_settings": {
      "include_dirs": [ "src" ],
    },
    "sources": [
      "src/client.c",
      "src/client-handshake.c",
      "src/client-protocol.c",
      "src/common.c",
      "src/encoder.c",
      "src/format/anvil-encode.c",
      "src/format/anvil-parse.c",
      "src/format/nbt-common.c",
      "src/format/nbt-encode.c",
      "src/format/nbt-parse.c",
      "src/format/nbt-utils.c",
      "src/format/nbt-value.c",
      "src/protocol/framer.c",
      "src/protocol/parser.c",
      "src/server.c",
      "src/session.c",
      "src/world.c",
    ],
  }]
}
