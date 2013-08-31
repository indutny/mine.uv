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
      "src/framer.c",
      "src/parser.c",
      "src/server.c",
      "src/world.c",
      "src/nbt.c",
    ],
  }]
}
