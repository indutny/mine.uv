#include <assert.h>  /* assert */
#include <stdint.h>  /* uint8_t */
#include <stdlib.h>  /* free */
#include <string.h>  /* memmove */

#include "client.h"
#include "uv.h"
#include "parser.h"
#include "server.h"

static void mc_client__on_close(uv_handle_t* handle);
static uv_buf_t mc_client__on_alloc(uv_handle_t* handle, size_t suggested_size);
static void mc_client__on_read(uv_stream_t* stream,
                               ssize_t nread,
                               uv_buf_t buf);
static void mc_client__cycle(mc_client_t* client);
static int mc_client__handle_frame(mc_client_t* client, mc_frame_t* frame);

int mc_client_init(mc_server_t* server, mc_client_t* client) {
  int r;
  client->server = server;
  client->tcp.data = client;

  r = uv_read_start((uv_stream_t*) &client->tcp,
                    mc_client__on_alloc,
                    mc_client__on_read);
  if (r != 0)
    return r;

  client->encrypted.len = 0;
  client->cleartext.len = 0;
  client->is_encrypted = 0;

  return 0;
}


void mc_client_destroy(mc_client_t* client) {
  uv_read_stop((uv_stream_t*) &client->tcp);
  uv_close((uv_handle_t*) &client->tcp, mc_client__on_close);
  client->encrypted.len = 0;
  client->cleartext.len = 0;
}


void mc_client__on_close(uv_handle_t* handle) {
  mc_client_t* client;

  client = handle->data;
  free(handle->data);
}


uv_buf_t mc_client__on_alloc(uv_handle_t* handle, size_t suggested_size) {
  mc_client_t* client;

  client = handle->data;
  return uv_buf_init((char*) client->encrypted.data + client->encrypted.len,
                     sizeof(client->encrypted.data) - client->encrypted.len);
}


void mc_client__on_read(uv_stream_t* stream, ssize_t nread, uv_buf_t buf) {
  mc_client_t* client;

  client = stream->data;

  if (nread > 0) {
    client->encrypted.len += nread;
    mc_client__cycle(client);

    return;
  } else {
    /* nread might be equal to UV_EOF, but who cares */
    mc_client_destroy(client);
  }
}


/*
 * Decipher (if needed) data from `encrypted` and put it into `cleartext`,
 * and run parser over the input
 */
void mc_client__cycle(mc_client_t* client) {
  uint8_t* data;
  ssize_t r;
  ssize_t len;
  mc_frame_t frame;

  while (client->encrypted.len != 0 || client->cleartext.len != 0) {
    if (client->is_encrypted) {
      /* NOT SUPPORTED YET */
      abort();
    } else {
      data = client->encrypted.data;
      len = client->encrypted.len;
    }

    /*
     * Parse one frame, note that frame has the same lifetime as data, and
     * directly depends on it
     */
    r = mc_parser_execute(data, len, &frame);

    /* Not enough data yet */
    if (r == 0)
      break;

    /* Malformed data */
    if (r < 0)
      return mc_client_destroy(client);

    /* Handle frame */
    r = mc_client__handle_frame(client, &frame);
    if (r != 0)
      return mc_client_destroy(client);

    /* Advance */
    assert(r <= len);
    memmove(data, data + r, len - r);
    if (client->is_encrypted)
      client->cleartext.len -= r;
    else
      client->encrypted.len -= r;
  }
}


int mc_client__handle_frame(mc_client_t* client, mc_frame_t* frame) {
  return 0;
}
