#ifndef SRC_UTILS_STRING_H_
#define SRC_UTILS_STRING_H_

typedef struct mc_string_s mc_string_t;

struct mc_string_s {
  const uint16_t* data;
  uint16_t len;
  int allocated;
};

void mc_string_init(mc_string_t* str);
void mc_string_destroy(mc_string_t* str);

void mc_string_set(mc_string_t* str, const uint16_t* data, int len);
int mc_string_copy(mc_string_t* to, mc_string_t* from);
char* mc_string_to_ascii(mc_string_t* str);
int mc_string_from_ascii(mc_string_t* to, const char* from);

#endif  /* SRC_UTILS_STRING_H_ */
