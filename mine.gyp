{
  "targets": [{
    "target_name": "mine.uv",
    "type": "executable",
    "dependencies": [
      "deps/uv/uv.gyp:libuv",
      "deps/openssl/openssl.gyp:openssl",
      "deps/zlib/zlib.gyp:zlib",
    ],
    "sources": [
      "src/main.c",

      "src/client.c",
      "src/common.c",
      "src/framer.c",
      "src/parser.c",
      "src/server.c",
      "src/world.c",
      "src/nbt.c",
    ],
  }]
}
