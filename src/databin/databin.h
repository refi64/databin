/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DATABIN_H
#define DATABIN_H

#include <inttypes.h>
#include <stdio.h>

typedef uint16_t databin_key;
typedef uint16_t databin_len;

typedef struct databin_io databin_io;
typedef struct databin_file_io databin_file_io;
typedef struct databin_fd_io databin_fd_io;
typedef struct databin_buffer_io databin_buffer_io;

typedef struct databin databin;
typedef struct databin_string databin_string;
typedef struct databin_value databin_value;
typedef enum databin_type databin_type;

struct databin_io {
  int (*read)(databin_io *io, void *buffer, size_t size);
  int (*write)(databin_io *io, const void *buffer, size_t size);
  void (*destroy)(databin_io *io);
};

databin_io *databin_file_io_new(FILE *fp);
/* databin_io *databin_fd_io_new(int fd); */
/* databin_io *databin_buffer_io_new(void *buffer, size_t size); */

void databin_io_close(databin_io *io);
/* void databin_io_closep(databin_io **io); */

enum databin_type {
  DATABIN_ANY = '\0',
  DATABIN_CONTAINER = '(',
  DATABIN_CONTAINER_CLOSE = ')',
  DATABIN_STRING = 's',
  DATABIN_BYTE = 'y',
  DATABIN_INT16 = 'n',
  DATABIN_UINT16 = 'q',
  DATABIN_INT32 = 'i',
  DATABIN_UINT32 = 'u',
  DATABIN_INT64 = 'x',
  DATABIN_UINT64 = 't',
  DATABIN_FLOAT32 = 'f',
  DATABIN_FLOAT64 = 'd',
};

#define DATABIN_STRING_LEN_AUTO ((databin_len) 0)

#define DATABIN_PARTIAL_STRING_JUST_LEN ((databin_type) 'L')
#define DATABIN_PARTIAL_STRING_JUST_STRING ((databin_type) 'S')

struct __attribute__((__packed__)) databin_string {
  databin_len len;
  char c;
};

struct databin_value {
  databin_type type;

  union {
    struct {
      char *buffer;
      databin_len len;
    } string;

    int8_t byte;

    int16_t i16;
    uint16_t u16;
    int32_t i32;
    uint32_t u32;
    int64_t i64;
    uint64_t u64;

    float f32;
    double f64;
  };
};

databin *databin_new(databin_io *io);
void databin_close(databin *bin);
/* void databin_closep(databin **bin); */

int databin_peek_type(databin *bin, databin_type *type);

int databin_append(databin *bin, databin_key key, const void *value, databin_type type);
int databin_read(databin *bin, databin_key *key, void *value, databin_type type);

int databin_append_string(databin *bin, databin_key key, const char *value, databin_len len);
int databin_read_string(databin *bin, databin_key *key, char **value, databin_len *len);

int databin_append_value(databin *bin, databin_key key, const databin_value *value);
int databin_read_value(databin *bin, databin_key *key, databin_value *value);

int databin_open_container(databin *bin, databin_key key);
int databin_close_container(databin *bin);
int databin_enter_container(databin *bin, databin_key *key);
int databin_exit_container(databin *bin);

#endif
