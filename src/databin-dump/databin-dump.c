#include "../databin/databin.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int dump(FILE *fp, databin *bin)
{
  databin_key key;
  databin_value value;

  int indent = 0;

#define STRING_PADDING "9"

  for (;;) {
    int rc = databin_read_value(bin, &key, &value);
    if (rc < 0) {
      if (feof(fp)) {
        return 1;
      }

      fprintf(stderr, "Error reading value: %s\n", strerror(-rc));
      return rc;
    }

    if (value.type == DATABIN_CONTAINER_CLOSE) {
      if (indent > 0) {
        indent--;
      }

      continue;
    }

    printf("%*s", indent * 2, "");
    printf("% 3d: ", (int)key);

    switch (value.type) {
    case DATABIN_CONTAINER:
      puts("container:");
      indent++;
      break;
    case DATABIN_CONTAINER_CLOSE:
      if (indent > 0) {
        indent--;
      }
      break;
    case DATABIN_STRING:
      printf("%" STRING_PADDING "s: %*s\n", "string", (int)value.string.len,
             value.string.buffer);
      free(value.string.buffer);
      break;
#define INT_CASE(type, spec, hex, field)                                                        \
  case type:                                                                                    \
    printf("%" STRING_PADDING "s: %" spec " (0x%" hex ")\n", #field, value.field, value.field); \
    break;
      INT_CASE(DATABIN_BYTE, PRId8, PRIx8, byte)
      INT_CASE(DATABIN_INT16, PRId16, PRIx16, i16)
      INT_CASE(DATABIN_UINT16, PRIu16, PRIx16, u16)
      INT_CASE(DATABIN_INT32, PRId32, PRIx32, i32)
      INT_CASE(DATABIN_UINT32, PRIu32, PRIx32, u32)
      INT_CASE(DATABIN_INT64, PRId64, PRIx64, i64)
      INT_CASE(DATABIN_UINT64, PRIu64, PRIx64, u64)
    case DATABIN_FLOAT32:
      printf("%" STRING_PADDING "s: %f\n", "f32", value.f32);
      break;
    case DATABIN_FLOAT64:
      printf("%" STRING_PADDING "s: %lf\n", "f64", value.f64);
      break;
    default:
      printf("%" STRING_PADDING "s: %d\n", "unknown", value.type);
      break;
    }
  }
}

int main(int argc, char **argv)
{
  if (argc != 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
    fputs("usage: databin-dump <databin>\n", stderr);
    return argc != 2 ? 1 : 0;
  }

  FILE *fp = fopen(argv[1], "r");

  if (fp == NULL) {
    perror("opening file");
    return 1;
  }

  databin *bin = NULL;
  databin_io *io = databin_file_io_new(fp);
  if (io != NULL) {
    bin = databin_new(io);
  }

  if (bin == NULL) {
    puts("Out of memory.");
    return 1;
  }

  int rc = dump(fp, bin);

  databin_close(bin);
  databin_io_close(io);
  fclose(fp);

  return rc < 0 ? 1 : 0;
}
