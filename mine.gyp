{
  "targets": [{
    "target_name": "mine.uv",
    "type": "executable",
    "dependencies": [
      "deps/uv/uv.gyp:libuv",
      "deps/openssl/openssl.gyp:openssl",
    ],
    "sources": [
      "src/main.c",

      "src/client.c",
      "src/common.c",
      "src/framer.c",
      "src/parser.c",
      "src/server.c",
    ],
  }]
}
