/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "databin.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct databin_file_io {
  databin_io io;
  FILE *fp;
};

static void databin_io_destroy(databin_io *io) {
  free(io);
}

static int databin_file_io_read(databin_io *io, void *buffer, size_t size) {
  databin_file_io *file_io = (databin_file_io *) io;

  if (buffer != NULL) {
    int ret = fread(buffer, size, 1, file_io->fp);
    if (ret == -1) {
      return feof(file_io->fp) ? -EINVAL : -errno;
    } else if (ret != 1) {
      return -EILSEQ;
    }
  } else {
    if (fseek(file_io->fp, size, SEEK_CUR) == -1) {
      return -errno;
    }
  }

  return 0;
}

static int databin_file_io_write(databin_io *io, const void *buffer, size_t size) {
  databin_file_io *file_io = (databin_file_io *)io;

  int ret = fwrite(buffer, size, 1, file_io->fp);
  if (ret == -1) {
    return -errno;
  } else if (ret != 1) {
    return -EILSEQ;
  } else {
    return 0;
  }
}

databin_io *databin_file_io_new(FILE *fp) {
  databin_file_io *file_io = malloc(sizeof(databin_file_io));
  if (file_io == NULL) {
    return NULL;
  }

  file_io->io.read = databin_file_io_read;
  file_io->io.write = databin_file_io_write;
  file_io->io.destroy = databin_io_destroy;
  file_io->fp = fp;

  return (databin_io *)file_io;
}

void databin_io_close(databin_io *io) {
  typedef void (* destroyer)(databin_io *io);
  destroyer destroy = io->destroy;
  destroy(io);
}

void databin_io_closep(databin_io **io) {
  if (*io != NULL) {
    databin_io_close(*io);
    *io = NULL;
  }
}

struct databin {
  databin_io *io;
  databin_type peek;
};

databin *databin_new(databin_io *io) {
  databin *bin = calloc(sizeof(databin), 1);
  if (bin == NULL) {
    return NULL;
  }

  bin->io = io;
  return bin;
}

void databin_close(databin *bin) {
  free(bin);
}

void databin_closep(databin **bin) {
  if (*bin != NULL) {
    databin_close(*bin);
    *bin = NULL;
  }
}

int databin_peek_type(databin *bin, databin_type *type) {
  if (bin->peek == '\0') {
    char c;
    int rc = bin->io->read(bin->io, &c, 1);
    if (rc < 0) {
      return rc;
    }

    switch (c) {
    case DATABIN_CONTAINER:
    case DATABIN_CONTAINER_CLOSE:
    case DATABIN_STRING:
    case DATABIN_BYTE:
    case DATABIN_INT16:
    case DATABIN_UINT16:
    case DATABIN_INT32:
    case DATABIN_UINT32:
    case DATABIN_INT64:
    case DATABIN_UINT64:
    case DATABIN_FLOAT32:
    case DATABIN_FLOAT64:
      bin->peek = (databin_type)c;
      break;
    default:
      return -EINVAL;
    }
  }

  if (type != NULL) {
    *type = bin->peek;
  }

  return 1;
}

static uint16_t get_type_size(databin_type type) {
  switch (type) {
  case DATABIN_ANY:
    return UINT16_MAX;
  case DATABIN_CONTAINER:
  case DATABIN_CONTAINER_CLOSE:
    return 0;
  case DATABIN_BYTE:
    return 1;
  case DATABIN_INT16:
  case DATABIN_UINT16:
    return 2;
  case DATABIN_INT32:
  case DATABIN_UINT32:
  case DATABIN_FLOAT32:
    return 4;
  case DATABIN_INT64:
  case DATABIN_UINT64:
  case DATABIN_FLOAT64:
  case DATABIN_STRING:
    return 8;
  }
}

int databin_append(databin *bin, databin_key key, const void *value, databin_type type) {
  int rc;

  uint16_t size = get_type_size(type);
  if (size == UINT16_MAX) {
    return -EINVAL;
  }

  uint8_t header[3];
  header[0] = (uint8_t)type;
  *((uint16_t *)&header[1]) = key;

  rc = bin->io->write(bin->io, header, sizeof(header));
  if (rc < 0) {
    return rc;
  }

  if (size != 0) {
    rc = bin->io->write(bin->io, value, size);
    if (rc < 0) {
      return rc;
    }
  }

  bin->peek = '\0';
  return 0;
}

int databin_append_continue_string(databin *bin, const char *value, size_t size) {
  return bin->io->write(bin->io, value, size);
}

int databin_read(databin *bin, databin_key *key, void *value, databin_type type) {
  int rc;

  databin_type actual_type;
  rc = databin_peek_type(bin, &actual_type);
  if (rc < 0) {
    return rc;
  } else if (actual_type != type) {
    return -EINVAL;
  }

  rc = bin->io->read(bin->io, key, sizeof(*key));
  if (rc < 0) {
    return rc;
  }

  databin_len size = get_type_size(type);

  if (type == DATABIN_STRING && value == NULL) {
    databin_len len;

    rc = bin->io->read(bin->io, &len, size);
    if (rc < 0) {
      return rc;
    }

    rc = bin->io->read(bin->io, NULL, len);
    if (rc < 0) {
      return rc;
    }
  } else {
    rc = bin->io->read(bin->io, value, size);
    if (rc < 0) {
      return rc;
    }
  }

  bin->peek = '\0';
  return 1;
}

int databin_read_continue_string(databin *bin, char *value, size_t size) {
  return bin->io->read(bin->io, value, size);
}

int databin_append_string(databin *bin, databin_key key, const char *value, databin_len len) {
  if (len == DATABIN_STRING_LEN_AUTO) {
    len = strlen(value);
  }

  int rc;

  rc = databin_append(bin, key, &len, DATABIN_STRING);
  if (rc < 0) {
    return rc;
  }

  return databin_append_continue_string(bin, value, len);
}

int databin_read_string(databin *bin, databin_key *key, char **value, databin_len *len) {
  int rc;

  if (len == NULL) {
    len = alloca(sizeof(databin_len));
  }

  rc = databin_read(bin, key, len, DATABIN_STRING);
  if (rc < 0) {
    return rc;
  }

  if (value != NULL) {
    *value = malloc(*len);
  }

  return databin_read_continue_string(bin, value != NULL ? *value : NULL, *len);
}

int databin_append_value(databin *bin, databin_key key, const databin_value *value) {
  if (value->type == DATABIN_STRING) {
    return databin_append_string(bin, key, value->string.buffer, value->string.len);
  } else {
    return databin_append(bin, key, &value->byte, value->type);
  }
}

int databin_read_value(databin *bin, databin_key *key, databin_value *value) {
  int rc;

  rc = databin_peek_type(bin, &value->type);
  if (rc < 0) {
    return rc;
  }

  if (value->type == DATABIN_STRING) {
    return databin_read_string(bin, key, &value->string.buffer, &value->string.len);
  } else {
    return databin_read(bin, key, &value->byte, value->type);
  }
}

int databin_open_container(databin *bin, databin_key key) {
  return databin_append(bin, key, NULL, DATABIN_CONTAINER);
}

int databin_close_container(databin *bin) {
  return databin_append(bin, 0, NULL, DATABIN_CONTAINER_CLOSE);
}

int databin_enter_container(databin *bin, databin_key *key) {
  return databin_read(bin, key, NULL, DATABIN_CONTAINER);
}

int databin_exit_container(databin *bin) {
  return databin_read(bin, 0, NULL, DATABIN_CONTAINER_CLOSE);
}
