/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include "gegl-compression.h"
#include "gegl-compression-rle.h"


#define RLE_COMPRESS_VERBATIM_UNROLL    4
#define RLE_COMPRESS_REPEAT_UNROLL     16
#define RLE_DECOMPRESS_VERBATIM_UNROLL  3
#define RLE_DECOMPRESS_REPEAT_UNROLL   16


#ifdef G_GNUC_NO_INLINE
  #define RLE_NOINLINE G_GNUC_NO_INLINE
#elif defined (__GNUC__)
  #define RLE_NOINLINE __attribute__ ((noinline))
#else
  #define RLE_NOINLINE
#endif


#ifdef RLE_BITS


#ifdef RLE_COMPRESS_PASS


#define gegl_compression_rle_compress_pass      \
  RLE_CAT (gegl_compression_rle_compress,       \
           RLE_CAT (_pass_, RLE_COMPRESS_PASS))


/*  private functions  */

RLE_NOINLINE
static gboolean
gegl_compression_rle_compress_pass (const guint8 *data,
                                    gint          n,
                                    gint          shift,
                                    gint          bpp,
                                    guint8       *compressed,
                                    gint         *compressed_size,
                                    gint          max_compressed_size)
{
  typedef enum
  {
    STATE_UNKNOWN,
    STATE_VERBATIM,
    STATE_REPEAT
  } State;

  State  state;
  guint8 val;
  guint8 last_val;
  gint   count;
  gint   size;

#if RLE_BITS == 8
  #define PACK()        \
    G_STMT_START        \
      {                 \
        last_val = val; \
                        \
        val = *data;    \
                        \
        data += bpp;    \
                        \
        n--;            \
      }                 \
    G_STMT_END
#else
  guint8 mask;

  shift = 8 - (shift + 1) * RLE_BITS;
  mask  = ((1 << RLE_BITS) - 1) << shift;

  #define PACK()                          \
    G_STMT_START                          \
      {                                   \
        gint v = 0;                       \
        gint i;                           \
                                          \
        last_val = val;                   \
                                          \
        for (i = 0; i < 8; i += RLE_BITS) \
          {                               \
            v |= (*data & mask) << i;     \
                                          \
            data += bpp;                  \
          }                               \
                                          \
        val = v >> shift;                 \
                                          \
        n--;                              \
      }                                   \
    G_STMT_END
#endif

  size = 0;

  state = STATE_UNKNOWN;
  count = 0;

  last_val = 0; // compiler complains about possible unused otherwise
  val = 0;

  while (TRUE)
    {
      switch (state)
        {
        case STATE_UNKNOWN:
          {
            if (count == 0)
              {
                if (! n)
                  goto end;

                PACK ();
              }

            if (! n)
              {
                RLE_WRITE (compressed, 0, size, max_compressed_size);
                RLE_WRITE (compressed, val, size, max_compressed_size);

                goto end;
              }

            PACK ();

            if (val != last_val)
              {
                RLE_WRITE (compressed, 0, size, max_compressed_size);
                RLE_WRITE (compressed, last_val, size, max_compressed_size);

                state = STATE_VERBATIM;
                count = 1;
              }
            else
              {
                state = STATE_REPEAT;
                count = 2;
              }
          }
          break;

        case STATE_VERBATIM:
          {
            gint next_count = 1;

            state = STATE_UNKNOWN;

            #define INNER_LOOP()                                          \
              G_STMT_START                                                \
                {                                                         \
                  RLE_WRITE (compressed, val, size, max_compressed_size); \
                  count++;                                                \
                                                                          \
                  if (! n)                                                \
                    {                                                     \
                      next_count = 0;                                     \
                                                                          \
                      goto verbatim_end;                                  \
                    }                                                     \
                                                                          \
                  PACK ();                                                \
                                                                          \
                  if (val == last_val)                                    \
                    {                                                     \
                      if (! n || count >= 126)                            \
                        {                                                 \
                          count--;                                        \
                          size--;                                         \
                                                                          \
                          state      = STATE_REPEAT;                      \
                          next_count = 2;                                 \
                                                                          \
                          goto verbatim_end;                              \
                        }                                                 \
                      else                                                \
                        {                                                 \
                          PACK ();                                        \
                                                                          \
                          if (val == last_val)                            \
                            {                                             \
                              count--;                                    \
                              size--;                                     \
                                                                          \
                              state      = STATE_REPEAT;                  \
                              next_count = 3;                             \
                                                                          \
                              goto verbatim_end;                          \
                            }                                             \
                          else                                            \
                            {                                             \
                              RLE_WRITE (compressed, last_val,            \
                                         size, max_compressed_size);      \
                              count++;                                    \
                            }                                             \
                        }                                                 \
                    }                                                     \
                }                                                         \
              G_STMT_END

            while (count <= 128 - 2 * RLE_COMPRESS_VERBATIM_UNROLL)
              {
                gint i;

                for (i = 0; i < RLE_COMPRESS_VERBATIM_UNROLL; i++)
                  INNER_LOOP ();
              }

            while (count < 128)
              INNER_LOOP ();

            #undef INNER_LOOP

verbatim_end:
            compressed[size - count - 1] = count - 1;

            count = next_count;
          }
          break;

        case STATE_REPEAT:
          {
            gint next_count = 0;

            state = STATE_UNKNOWN;

            #define INNER_LOOP()       \
              G_STMT_START             \
                {                      \
                  PACK ();             \
                                       \
                  if (val != last_val) \
                    {                  \
                      next_count = 1;  \
                                       \
                      goto repeat_end; \
                    }                  \
                                       \
                  count++;             \
                }                      \
              G_STMT_END

            while (n     >= RLE_COMPRESS_REPEAT_UNROLL &&
                   count <= (1 << 16) - RLE_COMPRESS_REPEAT_UNROLL)
              {
                gint i;

                for (i = 0; i < RLE_COMPRESS_REPEAT_UNROLL; i++)
                  INNER_LOOP ();
              }

            while (n && count < (1 << 16))
              INNER_LOOP ();

            #undef INNER_LOOP

repeat_end:
            if (count < 128)
              {
                RLE_WRITE (compressed, 255 - count, size, max_compressed_size);
              }
            else
              {
                count--;

                RLE_WRITE (compressed, 255,          size, max_compressed_size);
                RLE_WRITE (compressed, count >> 8,   size, max_compressed_size);
                RLE_WRITE (compressed, count & 0xff, size, max_compressed_size);
              }

            RLE_WRITE (compressed, last_val, size, max_compressed_size);

            count = next_count;
          }
          break;
        }
    }

  #undef PACK

end:
  *compressed_size = size;

  return TRUE;
}


#undef gegl_compression_rle_compress_pass

#undef RLE_COMPRESS_PASS
#undef RLE_WRITE


#elif defined (RLE_DECOMPRESS_PASS)


#define gegl_compression_rle_decompress_pass      \
  RLE_CAT (gegl_compression_rle_decompress,       \
           RLE_CAT (_pass_, RLE_DECOMPRESS_PASS))


/*  private functions  */

RLE_NOINLINE
static void
gegl_compression_rle_decompress_pass (guint8        *data,
                                      gint           n,
                                      gint           bpp,
                                      const guint8  *compressed,
                                      const guint8 **next_compressed)
{
#if RLE_BITS == 8
  #define UNPACK(val)  \
    G_STMT_START       \
      {                \
        *data = (val); \
                       \
        data += bpp;   \
      }                \
    G_STMT_END
#else
  #define UNPACK(val)                                 \
    G_STMT_START                                      \
      {                                               \
        guint8 v = (val);                             \
        gint   i;                                     \
                                                      \
        for (i = 0; i < 8 / RLE_BITS; i++)            \
          {                                           \
            *data   = (RLE_READ (data) << RLE_BITS) | \
                      (v & ((1 << RLE_BITS) - 1));    \
            v     >>= RLE_BITS;                       \
                                                      \
            data += bpp;                              \
          }                                           \
      }                                               \
    G_STMT_END
#endif

  while (n)
    {
      gint count = *compressed++;
      gint state = count & 0x80;

      if (state == 0)
        {
          count++;

          n -= count;

          for (;
               count >= RLE_DECOMPRESS_VERBATIM_UNROLL;
               count -= RLE_DECOMPRESS_VERBATIM_UNROLL)
            {
              gint i;

              for (i = 0; i < RLE_DECOMPRESS_VERBATIM_UNROLL; i++)
                UNPACK (*compressed++);
            }

          for (; count; count--)
            UNPACK (*compressed++);
        }
      else
        {
          guint8 val;

          count = 255 - count;

          if (! count)
            {
              count   = *compressed++;
              count <<= 8;
              count  |= *compressed++;

              count++;
            }

          n -= count;

          val = *compressed++;

          for (;
               count >= RLE_DECOMPRESS_REPEAT_UNROLL;
               count -= RLE_DECOMPRESS_REPEAT_UNROLL)
            {
              gint i;

              for (i = 0; i < RLE_DECOMPRESS_REPEAT_UNROLL; i++)
                UNPACK (val);
            }

          for (; count; count--)
            UNPACK (val);
        }
    }

  #undef UNPACK

  *next_compressed = compressed;
}


#undef gegl_compression_rle_decompress_pass

#undef RLE_DECOMPRESS_PASS
#undef RLE_READ


#else /* ! RLE_COMPRESS_PASS && ! RLE_DECOMPRESS_PASS */


#define RLE_CAT(x, y)   RLE_CAT_I (x, y)
#define RLE_CAT_I(x, y) x ## y


#define gegl_compression_rle \
  RLE_CAT (gegl_compression_rle, RLE_BITS)
#define gegl_compression_rle_compress \
  RLE_CAT (gegl_compression_rle_compress, RLE_BITS)
#define gegl_compression_rle_compress_pass_nobounds \
  RLE_CAT (gegl_compression_rle_compress, _pass_nobounds)
#define gegl_compression_rle_compress_pass_bounds \
  RLE_CAT (gegl_compression_rle_compress, _pass_bounds)
#define gegl_compression_rle_decompress \
  RLE_CAT (gegl_compression_rle_decompress, RLE_BITS)
#define gegl_compression_rle_decompress_pass_noinit \
  RLE_CAT (gegl_compression_rle_decompress, _pass_noinit)
#define gegl_compression_rle_decompress_pass_init \
  RLE_CAT (gegl_compression_rle_decompress, _pass_init)


/*  local function prototypes  */

static gboolean   gegl_compression_rle_compress   (const GeglCompression *compression,
                                                   const Babl            *format,
                                                   gconstpointer          data,
                                                   gint                   n,
                                                   gpointer               compressed,
                                                   gint                  *compressed_size,
                                                   gint                   max_compressed_size);
static gboolean   gegl_compression_rle_decompress (const GeglCompression *compression,
                                                   const Babl            *format,
                                                   gpointer               data,
                                                   gint                   n,
                                                   gconstpointer          compressed,
                                                   gint                   compressed_size);


/*  local variables  */

static const GeglCompression gegl_compression_rle =
{
  .compress   = gegl_compression_rle_compress,
  .decompress = gegl_compression_rle_decompress
};


/*  private functions  */

#define RLE_COMPRESS_PASS nobounds
#define RLE_WRITE(dest, val, i, n) \
  ((dest)[(i)++] = (val))
#include "gegl-compression-rle.c"

#define RLE_COMPRESS_PASS bounds
#define RLE_WRITE(dest, val, i, n) \
  G_STMT_START                     \
    {                              \
      gint j = (i)++;              \
                                   \
      if (j == (n))                \
        return FALSE;              \
                                   \
      (dest)[j] = (val);           \
    }                              \
  G_STMT_END
#include "gegl-compression-rle.c"

static gboolean
gegl_compression_rle_compress (const GeglCompression *compression,
                               const Babl            *format,
                               gconstpointer          data,
                               gint                   n,
                               gpointer               compressed,
                               gint                  *compressed_size,
                               gint                   max_compressed_size)
{
  const guint8 *data8               = data;
  guint8       *compressed8         = compressed;
  gint          bpp                 = babl_format_get_bytes_per_pixel (format);
  gint          m                   = 8 / RLE_BITS;
  gint          rem_compressed_size = max_compressed_size;
  gint          i;

  for (i = 0; i < m * bpp; i++)
    {
      gint max_pass_compressed_size;
      gint pass_compressed_size;

      max_pass_compressed_size = n / m + (n / m + 127) / 128;

      if (max_pass_compressed_size <= rem_compressed_size)
        {
          gegl_compression_rle_compress_pass_nobounds (
            data8 + i / m, n / m, i % m, bpp,
            compressed8, &pass_compressed_size, rem_compressed_size);
        }
      else
        {
          if (! gegl_compression_rle_compress_pass_bounds (
                  data8 + i / m, n / m, i % m, bpp,
                  compressed8, &pass_compressed_size, rem_compressed_size))
            {
              return FALSE;
            }
        }

      compressed8         += pass_compressed_size;
      rem_compressed_size -= pass_compressed_size;
    }

  if (m > 1)
    {
      gint rem = (n % m) * bpp;

      if (rem > rem_compressed_size)
        return FALSE;

      memcpy (compressed8, data8 + n * bpp - rem, rem);

      compressed8         += rem;
      rem_compressed_size -= rem;
    }

  *compressed_size = max_compressed_size - rem_compressed_size;

  return TRUE;
}

#define RLE_DECOMPRESS_PASS noinit
#define RLE_READ(src) 0
#include "gegl-compression-rle.c"

#define RLE_DECOMPRESS_PASS init
#define RLE_READ(src) (*(src))
#include "gegl-compression-rle.c"

static gboolean
gegl_compression_rle_decompress (const GeglCompression *compression,
                                 const Babl            *format,
                                 gpointer               data,
                                 gint                   n,
                                 gconstpointer          compressed,
                                 gint                   compressed_size)
{
  guint8       *data8       = data;
  const guint8 *compressed8 = compressed;
  gint          bpp         = babl_format_get_bytes_per_pixel (format);
  gint          m           = 8 / RLE_BITS;
  gint          i;

  for (i = 0; i < m * bpp; i++)
    {
      if (i % m)
        {
          gegl_compression_rle_decompress_pass_init   (data8 + i / m, n / m,
                                                       bpp,
                                                       compressed8,
                                                       &compressed8);
        }
      else
        {
          gegl_compression_rle_decompress_pass_noinit (data8 + i / m, n / m,
                                                       bpp,
                                                       compressed8,
                                                       &compressed8);
        }
    }

  if (m > 1)
    {
      gint rem = (n % m) * bpp;

      memcpy (data8 + n * bpp - rem, compressed8, rem);
    }

  return TRUE;
}


#undef gegl_compression_rle
#undef gegl_compression_rle_compress
#undef gegl_compression_rle_compress_pass_bounds
#undef gegl_compression_rle_compress_pass_nobounds
#undef gegl_compression_rle_decompress
#undef gegl_compression_rle_decompress_pass_noinit
#undef gegl_compression_rle_decompress_pass_init

#undef RLE_BITS


#endif /* ! RLE_COMPRESS_PASS && ! RLE_DECOMPRESS_PASS */


#else /* ! RLE_BITS */


#define RLE_BITS 1
#include "gegl-compression-rle.c"

#define RLE_BITS 2
#include "gegl-compression-rle.c"

#define RLE_BITS 4
#include "gegl-compression-rle.c"

#define RLE_BITS 8
#include "gegl-compression-rle.c"


/*  public functions  */

void
gegl_compression_rle_init (void)
{
  gegl_compression_register ("rle1", &gegl_compression_rle1);
  gegl_compression_register ("rle2", &gegl_compression_rle2);
  gegl_compression_register ("rle4", &gegl_compression_rle4);
  gegl_compression_register ("rle8", &gegl_compression_rle8);
}


#endif /* ! RLE_BITS */
