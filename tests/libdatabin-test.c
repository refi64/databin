#define _POSIX_C_SOURCE 200809L

#include <alloca.h>

#include <libcut.h>

#include "../src/databin/databin.h"

#define TEST_SUCCESS(expr)                                       \
  do {                                                           \
    int rc = (expr);                                             \
    LIBCUT_TEST_BASE((rc >= 0), (void)(0), "%s", strerror(-rc)); \
  } while (0)

enum {
  KEY_ROOT,
  KEY_BYTE,
  KEY_INTS,
  KEY_INT32,
  KEY_UINT64,
  KEY_FLOAT32,
  KEY_STRINGS,
  KEY_S1,
  KEY_S2
};

static void freep(void *p) {
  free(*(void **)p);
}

#define cleanup_free __attribute__((cleanup(freep)))

LIBCUT_TEST(test_databin) {
  cleanup_free char *buf = NULL;
  size_t size;

  databin *bin = NULL;
  FILE *fp = NULL;
  databin_io *io = NULL;

  fp = open_memstream(&buf, &size);
  io = databin_file_io_new(fp);
  bin = databin_new(io);
  TEST_SUCCESS(databin_open_container(bin, KEY_ROOT));

  uint8_t byte = 'B';
  TEST_SUCCESS(databin_append(bin, KEY_BYTE, &byte, DATABIN_BYTE));

  TEST_SUCCESS(databin_open_container(bin, KEY_INTS));

  int32_t i32 = 1234;
  uint64_t u64 = UINT64_MAX - 12;

  TEST_SUCCESS(databin_append(bin, KEY_INT32, &i32, DATABIN_INT32));
  TEST_SUCCESS(databin_append(bin, KEY_UINT64, &u64, DATABIN_UINT64));

  TEST_SUCCESS(databin_close_container(bin));
  TEST_SUCCESS(databin_open_container(bin, KEY_STRINGS));

  databin_string *s1 = alloca(sizeof(databin_string) + 3);
  s1->len = 4;
  strcpy(&s1->c, "abc");
  TEST_SUCCESS(databin_append(bin, KEY_S1, s1, DATABIN_STRING));
  TEST_SUCCESS(databin_append_string(bin, KEY_S2, "abc123", DATABIN_STRING_LEN_AUTO));

  TEST_SUCCESS(databin_close_container(bin));
  TEST_SUCCESS(databin_close_container(bin));

  databin_close(bin);
  databin_io_close(io);
  fclose(fp);

  fp = fmemopen(buf, size, "r");
  io = databin_file_io_new(fp);
  bin = databin_new(io);

  databin_key key;

  TEST_SUCCESS(databin_enter_container(bin, &key));
  LIBCUT_TEST_EQ((int)key, KEY_ROOT);

  TEST_SUCCESS(databin_read(bin, &key, &byte, DATABIN_BYTE));
  LIBCUT_TEST_EQ((int)key, KEY_BYTE);
  LIBCUT_TEST_EQ((char)byte, (char)'B');

  TEST_SUCCESS(databin_enter_container(bin, &key));
  LIBCUT_TEST_EQ((int)key, KEY_INTS);

  TEST_SUCCESS(databin_read(bin, &key, &i32, DATABIN_INT32));
  LIBCUT_TEST_EQ((int)key, KEY_INT32);
  LIBCUT_TEST_EQ((int)i32, 1234);

  TEST_SUCCESS(databin_read(bin, &key, &u64, DATABIN_UINT64));
  LIBCUT_TEST_EQ((int)key, KEY_UINT64);
  LIBCUT_TEST_EQ((unsigned long)u64, UINT64_MAX - 12);

  TEST_SUCCESS(databin_exit_container(bin));
  TEST_SUCCESS(databin_enter_container(bin, &key));
  LIBCUT_TEST_EQ((int)key, KEY_STRINGS);

  TEST_SUCCESS(databin_read(bin, &key, s1, DATABIN_STRING));
  LIBCUT_TEST_EQ((int)key, KEY_S1);
  LIBCUT_TEST_EQ((int)s1->len, 4);
  LIBCUT_TEST_STREQ(&s1->c, "abc");

  cleanup_free char *s2 = NULL;
  databin_len s2_len;
  TEST_SUCCESS(databin_read_string(bin, &key, &s2, &s2_len));
  LIBCUT_TEST_EQ((int)key, KEY_S2);
  LIBCUT_TEST_STREQ(s2, "abc123");

  TEST_SUCCESS(databin_exit_container(bin));
  TEST_SUCCESS(databin_exit_container(bin));

  databin_close(bin);
  databin_io_close(io);
  fclose(fp);

  fp = fmemopen(buf, size, "r");
  io = databin_file_io_new(fp);
  bin = databin_new(io);

  TEST_SUCCESS(databin_enter_container(bin, NULL));
  TEST_SUCCESS(databin_read(bin, NULL, NULL, DATABIN_BYTE));
  TEST_SUCCESS(databin_enter_container(bin, NULL));
  TEST_SUCCESS(databin_read(bin, NULL, NULL, DATABIN_INT32));
  TEST_SUCCESS(databin_read(bin, NULL, NULL, DATABIN_UINT64));
  TEST_SUCCESS(databin_exit_container(bin));
  TEST_SUCCESS(databin_enter_container(bin, NULL));
  TEST_SUCCESS(databin_read(bin, NULL, NULL, DATABIN_STRING));
  TEST_SUCCESS(databin_read_string(bin, NULL, NULL, NULL));
  TEST_SUCCESS(databin_exit_container(bin));

  databin_close(bin);
  databin_io_close(io);
  fclose(fp);
}

LIBCUT_MAIN(test_databin)
