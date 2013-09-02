#include <arpa/inet.h>  /* ntohs, ntohl */
#include <assert.h>  /* assert */
#include <stdlib.h>  /* NULL, malloc, free */
#include <string.h>  /* memcpy, memset */

#include "nbt.h"
#include "nbt-private.h"

struct mc_nbt__parser_s {
  unsigned char* data;
  int len;
  int depth;
  int name_len;
};

static const int kMaxDepth = 1024;
static const int kCompoundCapacity = 4;

typedef struct mc_nbt__parser_s mc_nbt__parser_t;

static int mc_nbt__decompress(const unsigned char* data,
                              int len,
                              mc_nbt_comp_t comp,
                              unsigned char** out);
static mc_nbt_value_t* mc_nbt__parse(mc_nbt__parser_t* parser);
static mc_nbt_value_t* mc_nbt__parse_payload(mc_nbt__tag_t tag,
                                             mc_nbt__parser_t* parser);
static mc_nbt_value_t* mc_nbt__alloc(mc_nbt__tag_t tag,
                                     mc_nbt__parser_t* parser,
                                     int additional_payload);
static mc_nbt_value_t* mc_nbt__parse_primitive(mc_nbt__tag_t tag,
                                               mc_nbt__parser_t* parser);
static mc_nbt_value_t* mc_nbt__parse_array(mc_nbt__tag_t tag,
                                           mc_nbt__parser_t* parser);
static mc_nbt_value_t* mc_nbt__parse_high_order(mc_nbt__tag_t tag,
                                                mc_nbt__parser_t* parser);

/* Used as tag end value */
static mc_nbt_value_t mc_nbt__end;


mc_nbt_value_t* mc_nbt_parse(const unsigned char* data,
                             int len,
                             mc_nbt_comp_t comp) {
  mc_nbt_value_t* res;
  mc_nbt__parser_t parser;
  unsigned char* uncompressed;

  if (comp == kNBTUncompressed) {
    parser.len = len;
    parser.data = (unsigned char*) data;
  } else {
    parser.len = mc_nbt__decompress(data, len, comp, &uncompressed);
    parser.data = uncompressed;
    if (parser.len < 0)
      return NULL;
  }

  parser.depth = 0;
  res = mc_nbt__parse(&parser);

  /* De-allocate uncompressed data */
  if (comp != kNBTUncompressed)
    free(uncompressed);

  return res;
}


int mc_nbt__decompress(const unsigned char* data,
                       int len,
                       mc_nbt_comp_t comp,
                       unsigned char** out) {
  return mc_nbt__zlib(data, len, comp, 1, out);
}


mc_nbt_value_t* mc_nbt__parse(mc_nbt__parser_t* parser) {
  mc_nbt__tag_t tag;
  mc_nbt_value_t* res;
  const char* name;
  int name_len;

  if (parser->depth >= kMaxDepth)
    return NULL;

  if (parser->len < 1)
    return NULL;

  tag = (mc_nbt__tag_t) parser->data[0];
  parser->data++;
  parser->len--;

  /* Tag end doesn't have a name */
  if (tag == kNBTTagEnd)
    return &mc_nbt__end;

  /* Parse tag's name */
  if (parser->len < 2)
    return NULL;

  name_len = ntohs(*(uint16_t*) parser->data);
  name = (const char*) parser->data + 2;

  parser->data += 2 + name_len;
  parser->len -= 2 + name_len;
  if (parser->len < 0)
    return NULL;

  parser->name_len = name_len;

  /* Parse tag's payload */
  parser->depth++;
  res = mc_nbt__parse_payload(tag, parser);
  parser->depth--;

  if (res != NULL) {
    assert(res->name.len == name_len);
    memcpy((char*) res->name.value, name, res->name.len);
  }

  return res;
}


mc_nbt_value_t* mc_nbt__parse_payload(mc_nbt__tag_t tag,
                                      mc_nbt__parser_t* parser) {
  switch (tag) {
    case kNBTTagByte:
    case kNBTTagShort:
    case kNBTTagInt:
    case kNBTTagLong:
    case kNBTTagFloat:
    case kNBTTagDouble:
      return mc_nbt__parse_primitive(tag, parser);
    case kNBTTagByteArray:
    case kNBTTagString:
    case kNBTTagIntArray:
      return mc_nbt__parse_array(tag, parser);
    case kNBTTagList:
    case kNBTTagCompound:
      return mc_nbt__parse_high_order(tag, parser);
    default:
      return NULL;
  }
}


mc_nbt_value_t* mc_nbt__alloc(mc_nbt__tag_t tag,
                              mc_nbt__parser_t* parser,
                              int additional_payload) {
  mc_nbt_value_t* res;

  res = malloc(sizeof(*res) + additional_payload + parser->name_len);
  if (res == NULL)
    return NULL;
  switch (tag) {
    case kNBTTagByte: res->type = kNBTByte; break;
    case kNBTTagShort: res->type = kNBTShort; break;
    case kNBTTagInt: res->type = kNBTInt; break;
    case kNBTTagLong: res->type = kNBTLong; break;
    case kNBTTagFloat: res->type = kNBTFloat; break;
    case kNBTTagDouble: res->type = kNBTDouble; break;
    case kNBTTagByteArray: res->type = kNBTByteArray; break;
    case kNBTTagString: res->type = kNBTString; break;
    case kNBTTagIntArray: res->type = kNBTIntArray; break;
    case kNBTTagList: res->type = kNBTList; break;
    case kNBTTagCompound: res->type = kNBTCompound; break;
    default:
      /* Unexpected */
      abort();
  }
  if (parser->name_len != 0)
    res->name.value = (char*) res + sizeof(*res) + additional_payload;
  else
    res->name.value = NULL;
  res->name.len = parser->name_len;
  parser->name_len = 0;

  return res;
}


mc_nbt_value_t* mc_nbt__parse_primitive(mc_nbt__tag_t tag,
                                        mc_nbt__parser_t* parser) {
  mc_nbt_value_t* res;

  res = mc_nbt__alloc(tag, parser, 0);
  if (res == NULL)
    return NULL;
  switch (tag) {
    case kNBTTagByte:
      if (parser->len < 1)
        goto fatal;
      res->value.i8 = parser->data[0];
      parser->data++;
      parser->len--;
      break;
    case kNBTTagShort:
      if (parser->len < 2)
        goto fatal;
      res->value.i16 = ntohs(*(int16_t*) parser->data);
      parser->data += 2;
      parser->len -= 2;
      break;
    case kNBTTagInt:
      if (parser->len < 4)
        goto fatal;
      res->value.i32 = ntohl(*(int32_t*) parser->data);
      parser->data += 4;
      parser->len -= 4;
      break;
    case kNBTTagLong:
      if (parser->len < 8)
        goto fatal;
      res->value.i64 = ((int64_t) ntohl(*(int32_t*) parser->data) << 32) |
                       ntohl(*(int32_t*) parser->data);
      parser->data += 8;
      parser->len -= 8;
      break;
    case kNBTTagFloat:
      if (parser->len < 4)
        goto fatal;
      res->value.f32 = *(float*) parser->data;
      parser->data += 4;
      parser->len -= 4;
      break;
    case kNBTTagDouble:
      if (parser->len < 8)
        goto fatal;
      res->value.f64 = *(double*) parser->data;
      parser->data += 8;
      parser->len -= 8;
      break;
    default:
      goto fatal;
  }

  return res;

fatal:
  free(res);
  return NULL;
}


mc_nbt_value_t* mc_nbt__parse_array(mc_nbt__tag_t tag,
                                    mc_nbt__parser_t* parser) {
  mc_nbt_value_t* res;
  int32_t i;
  int additional_len;
  int32_t len;

  /* Read header */
  switch (tag) {
    case kNBTTagByteArray:
      if (parser->len < 4)
        return NULL;

      len = ntohl(*(uint32_t*) parser->data);
      parser->data += 4;
      parser->len += 4;
      if (parser->len < len)
        return NULL;

      additional_len = len - 1;
      break;
    case kNBTTagString:
      if (parser->len < 2)
        return NULL;

      len = ntohs(*(uint16_t*) parser->data);
      parser->data += 2;
      parser->len += 2;
      if (parser->len < len)
        return NULL;

      additional_len = len - 1;
      break;
    case kNBTTagIntArray:
      if (parser->len < 4)
        return NULL;

      len = ntohl(*(uint32_t*) parser->data);
      parser->data += 4;
      parser->len -= 4;
      if (parser->len < len * 4)
        return NULL;

      additional_len = (len - 1) * 4;
      break;
    default:
      return NULL;
  }

  res = mc_nbt__alloc(tag, parser, additional_len);
  if (res == NULL)
    return NULL;

  /*
   * NOTE: all those arrays have `len` field, so it doesn't matter -
   * which one to choose in union
   */
  res->value.str.len = len;

  /* Read items */
  switch (tag) {
    case kNBTTagByteArray:
    case kNBTTagString:
      memcpy(res->value.str.value, parser->data, len);
      parser->data += len;
      parser->len -= len;
      break;
    case kNBTTagIntArray:
      if (parser->len < 4)
        goto fatal;

      for (i = 0; i < len; i++) {
        res->value.i32_list.list[i] = ntohl(*(uint32_t*) parser->data);
        parser->data += 4;
        parser->len -= 4;
      }
      break;
    default:
      return NULL;
  }

  return res;

fatal:
  free(res);
  return NULL;
}


mc_nbt_value_t* mc_nbt__parse_high_order(mc_nbt__tag_t tag,
                                         mc_nbt__parser_t* parser) {
  mc_nbt_value_t* res;
  mc_nbt_value_t* tmp;
  mc_nbt_value_t* child;
  mc_nbt__tag_t child_tag;
  int len;
  int i;
  int name_len;

  if (tag == kNBTTagList) {
    if (parser->len < 5)
      return NULL;

    child_tag = (mc_nbt__tag_t) parser->data[0];
    len = ntohl(*(uint32_t*) (parser->data + 1));

    parser->data += 5;
    parser->len -= 5;

    res = mc_nbt__alloc(tag,
                        parser,
                        (len - 1) * sizeof(*res->value.values.list));
    if (res == NULL)
      return NULL;
    for (i = 0; i < len; i++) {
      res->value.values.list[i] = mc_nbt__parse_payload(child_tag, parser);

      /* Failure - dealloc all previous items */
      if (res->value.values.list[i] == NULL)
        goto fatal;
    }
  } else {
    assert(tag == kNBTTagCompound);

    /* Remember name_len as we might want to grow initial res */
    name_len = parser->name_len;

    /* Allocate compound with initial capacity */
    len = kCompoundCapacity;
    res = mc_nbt__alloc(tag, parser, len * sizeof(*res->value.values.list));
    if (res == NULL)
      return res;

    i = 0;
    while (1) {
      /* Not enough space */
      if (i == len) {
        parser->name_len = name_len;
        len += kCompoundCapacity;
        tmp = mc_nbt__alloc(tag, parser, len * sizeof(*res->value.values.list));
        if (tmp == NULL)
          goto fatal;

        /* Restore old data */
        memcpy(tmp,
               res,
               sizeof(*res) +
                  (len - kCompoundCapacity) * sizeof(*res->value.values.list));
        free(res);
        res = tmp;
        tmp = NULL;
      }

      /* Parse child */
      child = mc_nbt__parse(parser);

      /* End tag */
      if (child == &mc_nbt__end)
        break;

      /* Error, free all previous items */
      if (child == NULL)
        goto fatal;

      /* Insert child in the list */
      res->value.values.list[i++] = child;
    }
  }
  res->value.values.len = i;

  return res;

fatal:
  i--;
  for (; i >= 0; i--)
    mc_nbt_destroy(res->value.values.list[i]);
  free(res);
  return NULL;
}
