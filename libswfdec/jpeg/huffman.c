
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <string.h>

#include "huffman.h"
#include "jpeg_debug.h"

/* misc helper function definitions */

static char *sprintbits (char *str, unsigned int bits, int n);

#undef JPEG_LOG
#define JPEG_LOG(...)


void
huffman_table_dump (HuffmanTable * table)
{
  unsigned int n_bits;
  unsigned int code;
  char str[33];
  int i;
  HuffmanEntry *entry;

  JPEG_LOG ("dumping huffman table %p", table);
  for (i = 0; i < table->len; i++) {
    entry = &g_array_index (table, HuffmanEntry, i);
    n_bits = entry->n_bits;
    code = entry->symbol >> (16 - n_bits);
    sprintbits (str, code, n_bits);
    JPEG_LOG ("%s --> %d", str, entry->value);
  }
}

HuffmanTable *
huffman_table_new (void)
{
  HuffmanTable *table;

  table = g_array_new (FALSE, TRUE, sizeof (HuffmanEntry));

  return table;
}

void
huffman_table_free (HuffmanTable * table)
{
  g_array_free (table, TRUE);
}

void
huffman_table_add (HuffmanTable * table, guint32 code, gint n_bits, gint value)
{
  HuffmanEntry entry;

  entry.value = value;
  entry.symbol = code << (16 - n_bits);
  entry.mask = 0xffff ^ (0xffff >> n_bits);
  entry.n_bits = n_bits;

  g_array_append_val (table, entry);
}

unsigned int
huffman_table_decode_jpeg (HuffmanTable * tab, bits_t * bits)
{
  unsigned int code;
  int i;
  char str[33];
  HuffmanEntry *entry;

  code = peekbits (bits, 16);
  for (i = 0; i < tab->len; i++) {
    entry = &g_array_index (tab, HuffmanEntry, i);
    if ((code & entry->mask) == entry->symbol) {
      code = getbits (bits, entry->n_bits);
      sprintbits (str, code, entry->n_bits);
      JPEG_LOG ("%s --> %d", str, entry->value);
      return entry->value;
    }
  }
  JPEG_ERROR ("huffman sync lost");

  return -1;
}

int
huffman_table_decode_macroblock (short *block, HuffmanTable * dc_tab,
    HuffmanTable * ac_tab, bits_t * bits)
{
  int r, s, x, rs;
  int k;
  //char str[33] = "NA";

  memset (block, 0, sizeof (short) * 64);

  s = huffman_table_decode_jpeg (dc_tab, bits);
  if (s < 0)
    return -1;
  x = getbits (bits, s);
  if ((x >> (s - 1)) == 0) {
    x -= (1 << s) - 1;
  }
  JPEG_LOG ("s=%d (block[0]=%d)", s, x);
  block[0] = x;

  for (k = 1; k < 64; k++) {
    rs = huffman_table_decode_jpeg (ac_tab, bits);
    if (rs < 0) {
      JPEG_ERROR ("huffman error");
      return -1;
    }
    if (bits->ptr > bits->end) {
      JPEG_ERROR ("overrun");
      return -1;
    }
    s = rs & 0xf;
    r = rs >> 4;
    if (s == 0) {
      if (r == 15) {
        JPEG_LOG ("r=%d s=%d (skip 16)", r, s);
        k += 15;
      } else {
        JPEG_LOG ("r=%d s=%d (eob)", r, s);
        break;
      }
    } else {
      k += r;
      if (k >= 64) {
        JPEG_ERROR ("macroblock overrun");
        return -1;
      }
      x = getbits (bits, s);
      //sprintbits (str, x, s);
      if ((x >> (s - 1)) == 0) {
        x -= (1 << s) - 1;
      }
      block[k] = x;
      //JPEG_LOG ("r=%d s=%d (%s -> block[%d]=%d)", r, s, str, k, x);
    }
  }
  return 0;
}

int
huffman_table_decode (HuffmanTable * dc_tab, HuffmanTable * ac_tab,
    bits_t * bits)
{
  short zz[64];
  int ret;
  int i;
  short *q;

  while (bits->ptr < bits->end) {
    ret = huffman_table_decode_macroblock (zz, dc_tab, ac_tab, bits);
    if (ret < 0)
      return -1;

    q = zz;
    for (i = 0; i < 8; i++) {
      JPEG_LOG ("%3d %3d %3d %3d %3d %3d %3d %3d",
          q[0], q[1], q[2], q[3], q[4], q[5], q[6], q[7]);
      q += 8;
    }
  }

  return 0;
}

/* misc helper functins */

static char *
sprintbits (char *str, unsigned int bits, int n)
{
  int i;
  int bit = 1 << (n - 1);

  for (i = 0; i < n; i++) {
    str[i] = (bits & bit) ? '1' : '0';
    bit >>= 1;
  }
  str[i] = 0;

  return str;
}
