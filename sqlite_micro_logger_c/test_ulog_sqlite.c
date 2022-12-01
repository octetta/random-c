/*
  Testing program for Sqlite Micro Logger

  Sqlite Micro Logger is a Fast and Lean
  Sqlite database logger targetting low RAM systems such as Microcontrollers.

  This Library can work on systems that have as little as 2kb,
  such as the ATMega328 MCU.  It is available for the Arduino platform.

  https://github.com/siara-in/sqlite_micro_logger

  Copyright @ 2019 Arundale Ramanathan, Siara Logics (cc)

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef ARDUINO

#include "src/ulog_sqlite.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>

//int fd;
FILE *fp;

// int32_t read_fn(struct dblog_write_context *ctx, void *buf, uint32_t pos, size_t len) {
//   if (lseek(fd, pos, SEEK_SET) == -1) {
//     ctx->err_no = errno;
//     return DBLOG_RES_SEEK_ERR;
//   }
//   ssize_t ret = read(fd, buf, len);
//   if (ret == -1) {
//     ctx->err_no = errno;
//     return DBLOG_RES_READ_ERR;
//   }
//   return ret;
// }

// int32_t read_fn_rctx(struct dblog_read_context *ctx, void *buf, uint32_t pos, size_t len) {
//   if (lseek(fd, pos, SEEK_SET) == -1)
//     return DBLOG_RES_SEEK_ERR;
//   ssize_t ret = read(fd, buf, len);
//   if (ret == -1)
//     return DBLOG_RES_READ_ERR;
//   return ret;
// }

// int32_t write_fn(struct dblog_write_context *ctx, void *buf, uint32_t pos, size_t len) {
//   if (lseek(fd, pos, SEEK_SET) == -1) {
//     ctx->err_no = errno;
//     return DBLOG_RES_SEEK_ERR;
//   }
//   ssize_t ret = write(fd, buf, len);
//   if (ret == -1) {
//     ctx->err_no = errno;
//     return DBLOG_RES_WRITE_ERR;
//   }
//   return ret;
// }

// int flush_fn(struct dblog_write_context *ctx) {
//   int ret = fsync(fd);
//   if (ret == -1) {
//     ctx->err_no = errno;
//     return DBLOG_RES_FLUSH_ERR;
//   }
//   return DBLOG_RES_OK;
// }

int32_t read_fn(struct dblog_write_context *ctx, void *buf, uint32_t pos, size_t len) {
  if (fseek(fp, pos, SEEK_SET)) {
    ctx->err_no = ferror(fp);
    return DBLOG_RES_SEEK_ERR;
  }
  size_t ret = fread(buf, 1, len, fp);
  if (ret != len) {
    ctx->err_no = ferror(fp);
    return DBLOG_RES_READ_ERR;
  }
  return ret;
}

int32_t read_fn_rctx(struct dblog_read_context *ctx, void *buf, uint32_t pos, size_t len) {
  if (fseek(fp, pos, SEEK_SET))
    return DBLOG_RES_SEEK_ERR;
  size_t ret = fread(buf, 1, len, fp);
  if (ret != len)
    return DBLOG_RES_READ_ERR;
  return ret;
}

int32_t write_fn(struct dblog_write_context *ctx, void *buf, uint32_t pos, size_t len) {
  if (fseek(fp, pos, SEEK_SET)) {
    ctx->err_no = ferror(fp);
    return DBLOG_RES_SEEK_ERR;
  }
  size_t ret = fwrite(buf, 1, len, fp);
  if (ret != len) {
    ctx->err_no = ferror(fp);
    return DBLOG_RES_WRITE_ERR;
  }
  return ret;
}

int flush_fn(struct dblog_write_context *ctx) {
  int ret = fflush(fp);
  if (ret) {
    ctx->err_no = ferror(fp);
    return DBLOG_RES_FLUSH_ERR;
  }
  return DBLOG_RES_OK;
}

int test_1byte_row(char *filename) {

  int32_t page_size = 512;
  byte buf[page_size];
  struct dblog_write_context ctx;
  ctx.buf = buf;
  ctx.col_count = 1;
  ctx.page_size_exp = 9;
  ctx.max_pages_exp = 0;
  ctx.page_resv_bytes = 0;
  ctx.read_fn = read_fn;
  ctx.flush_fn = flush_fn;
  ctx.write_fn = write_fn;

  unlink(filename);
  //fd = open(filename, O_CREAT | O_EXCL | O_TRUNC | O_RDWR | O_SYNC, S_IRUSR | S_IWUSR);
  fp = fopen(filename, "w+b");
  if (fp == NULL) {
    perror ("Open Error:");
    return -1;
  }

  struct tm *t;
  struct timeval tv;
  dblog_write_init(&ctx);

  int8_t ival;
  uint8_t types[] = {DBLOG_TYPE_INT};
  void *values[] = {&ival};
  uint16_t lengths[] = {1};

  int32_t max_rows = 1000000;
  for (int32_t i = 0; i < max_rows; i++) {
    ival = (int8_t) i % 255 / 2;
    dblog_append_row_with_values(&ctx, types, (const void **) values, lengths);
    if (i % 2)
      dblog_set_col_val(&ctx, 0, DBLOG_TYPE_INT, NULL, 1);
  }
  if (dblog_finalize(&ctx)) {
    printf("Error during finalize\n");
    fclose(fp);
    return -6;
  }

  fclose(fp);

  return 0;

}

int test_multilevel(char *filename) {

  int32_t page_size = 512;
  byte buf[page_size];
  struct dblog_write_context ctx;
  ctx.buf = buf;
  ctx.col_count = 5;
  ctx.page_size_exp = 9;
  ctx.max_pages_exp = 0;
  ctx.page_resv_bytes = 0;
  ctx.read_fn = read_fn;
  ctx.flush_fn = flush_fn;
  ctx.write_fn = write_fn;

  unlink(filename);
  //fd = open(filename, O_CREAT | O_EXCL | O_TRUNC | O_RDWR | O_SYNC, S_IRUSR | S_IWUSR);
  fp = fopen(filename, "w+b");
  if (fp == NULL) {
    perror ("Open Error:");
    return -1;
  }

  struct tm *t;
  struct timeval tv;
  dblog_write_init_with_script(&ctx, "test_log",
   "CREATE TABLE test_log (date_time TEXT, int_seq INTEGER, float_seq REAL, float_rand REAL, text_rand TEXT)");

  int32_t ival;
  double d1, d2;
  char txt[24];
  char txt1[11];
  uint8_t types[] = {DBLOG_TYPE_TEXT, DBLOG_TYPE_INT, DBLOG_TYPE_REAL, DBLOG_TYPE_REAL, DBLOG_TYPE_TEXT};
  void *values[] = {txt, &ival, &d1, &d2, txt1};
  uint16_t lengths[] = {23, sizeof(ival), sizeof(d1), sizeof(d2), 0};

  int32_t max_rows = 1000000;
  for (int32_t i = 0; i < max_rows; i++) {
    gettimeofday(&tv, NULL);
    t = localtime(&tv.tv_sec);
    t->tm_year += 1900;
    txt[0] = '0' + (t->tm_year / 1000) % 10;
    txt[1] = '0' + (t->tm_year / 100) % 10;
    txt[2] = '0' + (t->tm_year / 10) % 10;
    txt[3] = '0' + t->tm_year % 10;
    txt[4] = '-';
    t->tm_mon++;
    txt[5] = '0' + (t->tm_mon / 10) % 10;
    txt[6] = '0' + t->tm_mon % 10;
    txt[7] = '-';
    txt[8] = '0' + (t->tm_mday / 10) % 10;
    txt[9] = '0' + t->tm_mday % 10;
    txt[10] = ' ';
    txt[11] = '0' + (t->tm_hour / 10) % 10;
    txt[12] = '0' + t->tm_hour % 10;
    txt[13] = ':';
    txt[14] = '0' + (t->tm_min / 10) % 10;
    txt[15] = '0' + t->tm_min % 10;
    txt[16] = ':';
    txt[17] = '0' + (t->tm_sec / 10) % 10;
    txt[18] = '0' + t->tm_sec % 10;
    txt[19] = '.';
    txt[20] = '0' + (tv.tv_usec / 100000) % 10;
    txt[21] = '0' + (tv.tv_usec / 10000) % 10;
    txt[22] = '0' + (tv.tv_usec / 1000) % 10;
    ival = i - max_rows / 2;
    d1 = i;
    d1 /= 2;
    d2 = rand();
    d2 /= 1000;
    int txt_len = rand() % 10;
    lengths[4] = txt_len;
    while (txt_len--)
      txt1[txt_len] = 'a' + (char)(rand() % 26);
    dblog_append_row_with_values(&ctx, types, (const void **) values, lengths);
  }
  if (dblog_finalize(&ctx)) {
    printf("Error during finalize\n");
    fclose(fp);
    return -6;
  }

  fclose(fp);

  return 0;

}

int test_basic(char *filename) {

  byte buf[512];
  struct dblog_write_context ctx;
  ctx.buf = buf;
  ctx.col_count = 5;
  ctx.page_size_exp = 9;
  ctx.max_pages_exp = 0;
  ctx.page_resv_bytes = 0;
  ctx.read_fn = read_fn;
  ctx.flush_fn = flush_fn;
  ctx.write_fn = write_fn;

  unlink(filename);
  //fd = open(filename, O_CREAT | O_EXCL | O_TRUNC | O_RDWR | O_SYNC, S_IRUSR | S_IWUSR);
  fp = fopen(filename, "w+b");
  if (fp == NULL) {
    perror ("Open Error:");
    return -1;
  }

  dblog_write_init(&ctx);
  dblog_set_col_val(&ctx, 0, DBLOG_TYPE_TEXT, "Hello", 5);
  dblog_set_col_val(&ctx, 1, DBLOG_TYPE_TEXT, "World", 5);
  dblog_set_col_val(&ctx, 2, DBLOG_TYPE_TEXT, "How", 3);
  dblog_set_col_val(&ctx, 3, DBLOG_TYPE_TEXT, "Are", 3);
  dblog_set_col_val(&ctx, 4, DBLOG_TYPE_TEXT, "You", 3);
  dblog_append_empty_row(&ctx);
  dblog_set_col_val(&ctx, 0, DBLOG_TYPE_TEXT, "I", 1);
  dblog_set_col_val(&ctx, 1, DBLOG_TYPE_TEXT, "am", 2);
  dblog_set_col_val(&ctx, 2, DBLOG_TYPE_TEXT, "fine", 4);
  dblog_set_col_val(&ctx, 3, DBLOG_TYPE_TEXT, "thank", 5);
  dblog_set_col_val(&ctx, 4, DBLOG_TYPE_TEXT, "you", 3);
  if (dblog_finalize(&ctx)) {
    printf("Error during finalize\n");
    return -6;
  }

  fclose(fp);

  return 0;

}

void print_usage() {
  printf("\nTesting Sqlite Micro Logger\n");
  printf("---------------------------\n\n");
  printf("Sqlite Micro logger is a library that logs records in Sqlite format 3\n");
  printf("using as less memory as possible. This utility is intended for testing it.\n\n");
  printf("Usage\n");
  printf("-----\n\n");
  printf("test_ulog_sqlite -c <db_name.db> <page_size> <col_count> <csv_1> ... <csv_n>\n");
  printf("    Creates a Sqlite database with the given name and page size\n");
  printf("        and given records in CSV format (no comma in data)\n\n");
  printf("test_ulog_sqlite -a <db_name.db> <page_size> <col_count> <csv_1> ... <csv_n>\n");
  printf("    Appends to a Sqlite database created using -c above\n");
  printf("        with records in CSV format (page_size and col_count have to match)\n\n");
  printf("test_ulog_sqlite -v <db_name.db>\n");
  printf("    Attempts to recover <db_name.db> if not finalized\n\n");
  printf("test_ulog_sqlite -r <db_name.db> <rowid>\n");
  printf("    Searches <db_name.db> for given row_id and prints result\n\n");
  printf("test_ulog_sqlite -b <db_name.db> <col_idx> <value>\n");
  printf("    Searches <db_name.db> and column for given value using\n");
  printf("        binary search and prints result. col_idx starts from 0.\n\n");
  printf("test_ulog_sqlite -n\n");
  printf("    Runs pre-defined tests and creates databases (verified manually)\n\n");
}

extern byte get_page_size_exp(int32_t page_size);
byte validate_page_size(int32_t page_size) {
  return get_page_size_exp(page_size);
}

int add_col(struct dblog_write_context *ctx, int col_idx, char *data, byte isInt, byte isReal) {
  if (isInt) {
    int64_t ival = atoll(data);
    if (ival >= -128 && ival <= 127) {
      int8_t i8val = (int8_t) ival;
      return dblog_set_col_val(ctx, col_idx, DBLOG_TYPE_INT, &i8val, 1);
    } else
    if (ival >= -32768 && ival <= 32767) {
      int16_t i16val = (int16_t) ival;
      return dblog_set_col_val(ctx, col_idx, DBLOG_TYPE_INT, &i16val, 2);
    } else
    if (ival >= -2147483648 && ival <= 2147483647) {
      int32_t i32val = (int32_t) ival;
      return dblog_set_col_val(ctx, col_idx, DBLOG_TYPE_INT, &i32val, 4);
    } else {
      return dblog_set_col_val(ctx, col_idx, DBLOG_TYPE_INT, &ival, 8);
    }
  } else
  if (isReal) {
    //float dval = atof(data);
    double dval = atof(data);
    return dblog_set_col_val(ctx, col_idx, DBLOG_TYPE_REAL, &dval, sizeof(dval));
  }
  return dblog_set_col_val(ctx, col_idx, DBLOG_TYPE_TEXT, data, strlen(data));
}

int append_records(int argc, char *argv[], struct dblog_write_context *ctx) {
  if (dblog_append_empty_row(ctx)) {
    printf("Error during add row\n");
    return -5;
  }
  for (int i = 5; i < argc; i++) {
    char *col_data = argv[i];
    char *chr = col_data;
    int col_idx = 0;
    byte isInt = 1;
    byte isReal = 1;
    while (*chr != '\0') {
      if (*chr == ',') {
        *chr = '\0';
        if (add_col(ctx, col_idx++, col_data, isInt, isReal)) {
          printf("Error during add col\n");
          return -4;
        }
        chr++;
        col_data = chr;
        isInt = 1;
        isReal = 1;
        continue;
      }
      if ((*chr < '0' || *chr > '9') && *chr != '-' && *chr != '.') {
        isInt = 0;
        isReal = 0;
      } else {
        if (*chr == '.')
          isInt = 0;
        if (*chr == '-' && chr > col_data) {
          isInt = 0;
          isReal = 0;
        }
      }
      chr++;
    }
    if (add_col(ctx, col_idx++, col_data, isInt, isReal)) {
      printf("Error during add col\n");
      return -4;
    }
    if (i < argc - 1) {
      if (dblog_append_empty_row(ctx)) {
        printf("Error during add col\n");
        return -5;
      }
    }
  }
  if (dblog_finalize(ctx)) {
    printf("Error during finalize\n");
    return -6;
  }
  return 0;
}

int create_db(int argc, char *argv[]) {
  int32_t page_size = atol(argv[3]);
  byte page_size_exp = validate_page_size(page_size);
  if (!page_size_exp) {
    printf("Page size should be one of 512, 1024, 2048, 4096, 8192, 16384, 32768 or 65536\n");
    return -1;
  }
  byte col_count = atoi(argv[4]);
  byte buf[page_size];
  struct dblog_write_context ctx;
  ctx.buf = buf;
  ctx.col_count = col_count;
  ctx.page_size_exp = page_size_exp;
  ctx.max_pages_exp = 0;
  ctx.page_resv_bytes = 0;
  ctx.read_fn = read_fn;
  ctx.flush_fn = flush_fn;
  ctx.write_fn = write_fn;
  unlink(argv[2]);
  //fd = open(argv[2], O_CREAT | O_EXCL | O_TRUNC | O_RDWR | O_SYNC, S_IRUSR | S_IWUSR);
  //if (fd == -1) {
  //  perror("Error");
  //  return -2;
  //}
  fp = fopen(argv[2], "w+b");
  if (fp == NULL) {
    perror ("Open Error:");
    fclose(fp);
    return -1;
  }
  if (dblog_write_init(&ctx)) {
    printf("Error during init\n");
    fclose(fp);
    return -3;
  }
  int ret = append_records(argc, argv, &ctx);
  fclose(fp);
  return ret;
}

int append_db(int argc, char *argv[]) {
  int32_t page_size = atol(argv[3]);
  byte page_size_exp = validate_page_size(page_size);
  if (!page_size_exp) {
    printf("Page size should be one of 512, 1024, 2048, 4096, 8192, 16384, 32768 or 65536\n");
    return -1;
  }
  byte col_count = atoi(argv[4]);
  byte buf[page_size];
  struct dblog_write_context ctx;
  ctx.buf = buf;
  ctx.col_count = col_count;
  ctx.page_size_exp = page_size_exp;
  ctx.max_pages_exp = 0;
  ctx.page_resv_bytes = 0;
  ctx.read_fn = read_fn;
  ctx.flush_fn = flush_fn;
  ctx.write_fn = write_fn;
  //fd = open(argv[2], O_RDWR | O_SYNC, S_IRUSR | S_IWUSR);
  //if (fd == -1) {
  //  perror("Error");
  //  return -2;
  //}
  fp = fopen(argv[2], "r+b");
  if (fp == NULL) {
    perror ("Open Error:");
    fclose(fp);
    return -1;
  }
  if (dblog_init_for_append(&ctx)) {
    printf("Error during init\n");
    fclose(fp);
    return -3;
  }
  int ret = append_records(argc, argv, &ctx);
  fclose(fp);
  return ret;
}

int recover_db(int argc, char *argv[]) {
  byte initial_buf[72];
  struct dblog_write_context ctx;
  ctx.buf = initial_buf;
  ctx.read_fn = read_fn;
  ctx.flush_fn = flush_fn;
  ctx.write_fn = write_fn;
  //fd = open(argv[2], O_RDWR | O_SYNC, S_IRUSR | S_IWUSR);
  //if (fd == -1) {
  //  perror("Error");
  //  return -2;
  //}
  fp = fopen(argv[2], "r+b");
  if (fp == NULL) {
    perror ("Open Error:");
    return -1;
  }
  int32_t page_size = dblog_read_page_size(&ctx);
  if (page_size < 512) {
    printf("Error reading page size\n");
    fclose(fp);
    return -2;
  }
  byte buf[page_size];
  ctx.buf = buf;
  if (dblog_recover(&ctx)) {
    printf("Error during recover\n");
    fclose(fp);
    return -3;
  }
  fclose(fp);
  return 0;
}

int16_t read_int16(const byte *ptr) {
  return (*ptr << 8) | ptr[1];
}

int32_t read_int32(const byte *ptr) {
  int32_t ret;
  ret  = ((int32_t)*ptr++) << 24;
  ret |= ((int32_t)*ptr++) << 16;
  ret |= ((int32_t)*ptr++) << 8;
  ret |= *ptr;
  return ret;
}

int64_t read_int64(const byte *ptr) {
  int64_t ret;
  ret  = ((int64_t)*ptr++) << 56;
  ret |= ((int64_t)*ptr++) << 48;
  ret |= ((int64_t)*ptr++) << 40;
  ret |= ((int64_t)*ptr++) << 32;
  ret |= ((int64_t)*ptr++) << 24;
  ret |= ((int64_t)*ptr++) << 16;
  ret |= ((int64_t)*ptr++) << 8;
  ret |= *ptr;
  return ret;
}

double read_double(const byte *ptr) {
  double ret;
  int64_t ival = read_int64(ptr);
  memcpy(&ret, &ival, sizeof(double));
  return ret;
}

void display_row(struct dblog_read_context ctx) {
  int col_count = dblog_cur_row_col_count(&ctx);
  for (int i = 0; i < col_count; i++) {
    if (i)
      putchar('|');
    uint32_t col_type;
    const byte *col_val = (const byte *) dblog_read_col_val(&ctx, i, &col_type);
    if (!col_val) {
      printf("Error reading value\n");
      return;
    }
    switch (col_type) {
      case 0:
        printf("null");
        break;
      case 1:
        printf("%d", *((int8_t *)col_val));
        break;
      case 2:
        if (1) {
          int16_t ival = read_int16(col_val);
          printf("%d", ival);
        }
        break;
      case 4: {
        int32_t ival = read_int32(col_val);
        printf("%d", ival);
        break;
      }
      case 6: {
        int64_t ival = read_int64(col_val);
        printf("%lld", ival);
        break;
      }
      case 7: {
        double dval = read_double(col_val);
        printf("%lf", dval);
        break;
      }
      default: {
        uint32_t col_len = dblog_derive_data_len(col_type);
        for (int j = 0; j < col_len; j++) {
          if (col_type % 2)
            putchar(col_val[j]);
          else
            printf("x%2x ", col_val[j]);
        }
      }
    }
  }
}

extern void write_uint8(byte *ptr, uint8_t input);
extern void write_uint16(byte *ptr, uint16_t input);
extern void write_uint32(byte *ptr, uint32_t input);
extern void write_uint64(byte *ptr, uint64_t input);

int resolve_value(char *value, byte *out_val) {
  byte isInt = 1;
  byte isReal = 1;
  char *chr = value;
  while (*chr != '\0') {
    if ((*chr < '0' || *chr > '9') && *chr != '-' && *chr != '.') {
      isInt = 0;
      isReal = 0;
    } else {
      if (*chr == '.')
        isInt = 0;
      if (*chr == '-' && chr > value) {
        isInt = 0;
        isReal = 0;
      }
    }
    chr++;
  }
  if (isInt) {
    int64_t ival = (int64_t) atoll(value);
    memcpy(out_val, &ival, sizeof(ival));
    return DBLOG_TYPE_INT;
  } else
  if (isReal) {
    double dval = atof(value);
    memcpy(out_val, &dval, sizeof(dval));
    return DBLOG_TYPE_REAL;
  }
  memcpy(out_val, value, strlen(value));
  return DBLOG_TYPE_TEXT;
}

int bin_srch_db(int argc, char *argv[]) {
  int len = strlen(argv[4]);
  if (len > 71) {
    printf("Value too long\n");
    return -1;
  }
  byte buf[72];
  struct dblog_read_context ctx;
  ctx.buf = buf;
  ctx.read_fn = read_fn_rctx;
  //fd = open(argv[2], O_RDWR | O_SYNC, S_IRUSR | S_IWUSR);
  //if (fd == -1) {
  //  perror("Error");
  //  return -2;
  //}
  fp = fopen(argv[2], "r+b");
  if (fp == NULL) {
    perror ("Open Error:");
    return -1;
  }
  if (dblog_read_init(&ctx)) {
    printf("Error during init\n");
    fclose(fp);
    return -3;
  }
  byte page_buf[1 << (ctx.page_size_exp == 1 ? 16 : ctx.page_size_exp)];
  ctx.buf = page_buf;
  int col_idx = atol(argv[3]);
  int val_type = resolve_value(argv[4], buf);
  if (val_type == DBLOG_TYPE_INT || val_type == DBLOG_TYPE_REAL)
    len = 8;
  if (dblog_bin_srch_row_by_val(&ctx, col_idx, val_type, 
        buf, len, col_idx == -1 ? 1 : 0)) {
    printf("Not Found\n");
  } else {
    display_row(ctx);
  }
  putchar('\n');
  fclose(fp);
  return 0;
}

int read_db(int argc, char *argv[]) {
  byte buf[72];
  struct dblog_read_context ctx;
  ctx.buf = buf;
  ctx.read_fn = read_fn_rctx;
  //fd = open(argv[2], O_RDWR | O_SYNC, S_IRUSR | S_IWUSR);
  //if (fd == -1) {
  //  perror("Error");
  //  return -2;
  //}
  fp = fopen(argv[2], "r+b");
  if (fp == NULL) {
    perror ("Open Error:");
    return -1;
  }
  if (dblog_read_init(&ctx)) {
    printf("Error during init\n");
    fclose(fp);
    return -3;
  }
  byte page_buf[1 << (ctx.page_size_exp == 1 ? 16 : ctx.page_size_exp)];
  ctx.buf = page_buf;
  uint32_t rowid = atol(argv[3]);
  if (dblog_srch_row_by_id(&ctx, rowid)) {
    printf("Not Found\n");
  } else {
    display_row(ctx);
  }
  putchar('\n');
  fclose(fp);
  return 0;
}

int main(int argc, char *argv[]) {

  if (argc > 4 && strcmp(argv[1], "-c") == 0) {
    create_db(argc, argv);
  } else
  if (argc > 4 && strcmp(argv[1], "-a") == 0) {
    append_db(argc, argv);
  } else
  if (argc == 3 && strcmp(argv[1], "-v") == 0) {
    recover_db(argc, argv);
  } else
  if (argc == 4 && strcmp(argv[1], "-r") == 0) {
    read_db(argc, argv);
  } else
  if (argc == 5 && strcmp(argv[1], "-b") == 0) {
    bin_srch_db(argc, argv);
  } else
  if (argc == 2 && strcmp(argv[1], "-n") == 0) {
    test_basic("hello.db");
    test_multilevel("ml.db");
    test_1byte_row("1byte_row.db");
  } else
    print_usage();

  return 0;

}
#endif
