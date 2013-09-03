#include <stdlib.h>  /* malloc, free, NULL */
#include <string.h>  /* memset */

#include "nbt-private.h"
#include "zlib.h"

static const int kZlibIncrement = 64 * 1024;


int mc_nbt__zlib(const unsigned char* data,
                 int len,
                 mc_nbt_comp_t comp,
                 int decompress,
                 unsigned char** out) {
  int r;
  int out_len;
  unsigned char* tmp;
  z_stream stream;

  memset(&stream, 0, sizeof(stream));
  if (decompress) {
    r = inflateInit2(&stream, comp == kNBTGZip ? kGZipBits : kDeflateBits);
  } else {
    r = deflateInit2(&stream,
                     Z_DEFAULT_COMPRESSION,
                     Z_DEFLATED,
                     comp == kNBTGZip ? kGZipBits : kDeflateBits,
                     8,
                     Z_DEFAULT_STRATEGY);
  }
  if (r != Z_OK)
    return -1;

  stream.next_in = (unsigned char*) data;
  stream.avail_in = len;

  /* NBT should be really good at compressing */
  out_len = kZlibIncrement;
  *out = malloc(out_len);
  if (*out == NULL)
    goto fatal;

  stream.avail_out = out_len;
  stream.next_out = *out;

  do {
    if (decompress)
      r = inflate(&stream, stream.avail_in == 0 ? Z_FINISH : Z_SYNC_FLUSH);
    else
      r = deflate(&stream, stream.avail_in == 0 ? Z_FINISH : Z_SYNC_FLUSH);
 
    if (r == Z_STREAM_END)
      break;

    if (r != Z_OK)
      goto fatal;

    if (stream.avail_out != 0)
      continue;

    /* Buffer too small... */
    tmp = malloc(out_len + kZlibIncrement);
    if (tmp == NULL)
      goto fatal;
    memcpy(tmp, *out, out_len);
    free(*out);
    *out = tmp;

    stream.avail_out = kZlibIncrement;
    stream.next_out = *out + out_len;
    out_len += kZlibIncrement;
  } while (1);

  if (r != Z_STREAM_END)
    goto fatal;

  if (decompress)
    r = inflateEnd(&stream);
  else
    r = deflateEnd(&stream);
  if (r != Z_OK)
    goto fatal;

  return out_len - stream.avail_out;

fatal:
  if (decompress)
    inflateEnd(&stream);
  else
    deflateEnd(&stream);
  free(*out);
  return -1;
}
