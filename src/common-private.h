#ifndef SRC_COMMON_PRIVATE_H_
#define SRC_COMMON_PRIVATE_H_

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define offsetof(s, m) \
    ((size_t) (&(((s *)0)->m)))
#define container_of(ptr, type, member) \
    ((type *) ((char *) (ptr) - offsetof(type, member)))

#endif  /* SRC_COMMON_PRIVATE_H_ */
