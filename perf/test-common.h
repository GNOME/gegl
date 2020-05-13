#include <stdlib.h>
#include <glib.h>
#include "gegl.h"
#include "opencl/gegl-cl-init.h"

#define ITERATIONS 2000
#define PERCENTILE 0.75  /* if we want to bias to the better results with
                           more noise, increase this number towards 1.0,
                           like 0.8 */
#define BAIL_THRESHOLD 0.001
#define BAIL_COUNT     30
#define MIN_ITER       30

static long ticks_start;

typedef void (*t_run_perf)(GeglBuffer *buffer);

long babl_ticks (void); /* using babl_ticks instead of gegl_ticks
                           to be able to go further back in time */

void test_start (void);
void test_end (const gchar *id,
               double       bytes);
void test_end_suffix (const gchar *id,
                      const gchar *suffix,
                      gdouble      bytes);
GeglBuffer *test_buffer (gint width,
                         gint height,
                         const Babl *format);
void do_bench(const gchar *id,
              GeglBuffer  *buffer,
              t_run_perf   test_func,
              gboolean     opencl);
void bench(const gchar *id,
           GeglBuffer  *buffer,
           t_run_perf   test_func );


int  converged = 0;
long ticks_iter_start = 0;
long iter_db[ITERATIONS];
int iter_no = 0;
float prev_median = 0.0;

void test_start (void)
{
  ticks_start = babl_ticks ();
  iter_no = 0;
  converged = 0;
  prev_median = 0.0;
}

static void test_start_iter (void)
{
  ticks_iter_start = babl_ticks ();
}


static int compare_long (const void * a, const void * b)
{
  return ( *(long*)a - *(long*)b );
}
static float compute_median (void)
{
  qsort(iter_db,iter_no,sizeof(long),compare_long);
  return iter_db[(int)(iter_no * (1.0-PERCENTILE))];
}

#include <math.h>
#include <stdio.h>

static void test_end_iter (void)
{
  long ticks = babl_ticks ()-ticks_iter_start;
  float median;
  iter_db[iter_no] = ticks;
  iter_no++;

  median = compute_median ();
  if (iter_no > MIN_ITER &&
      fabs ((median - prev_median) / median) < BAIL_THRESHOLD)
   {
     /* we've converged */
     converged++;
   }
  else
   {
     converged = 0;
   }
  prev_median = median;
}

void test_end_suffix (const gchar *id,
                      const gchar *suffix,
                      gdouble      bytes)
{
//  long ticks = babl_ticks ()-ticks_start;
  g_print ("@ %s%s: %.2f megabytes/second\n",
       id, suffix,
        (bytes / 1024.0 / ITERATIONS/ 1024.0)  / (compute_median()/1000000.0));
  //     (bytes / 1024.0 / 1024.0)  / (ticks / 1000000.0));
}

void test_end (const gchar *id,
               gdouble      bytes)
{
    test_end_suffix (id, "", bytes);
}

/* create a test buffer of random data in -0.5 to 2.0 range 
 */
GeglBuffer *test_buffer (gint width,
                         gint height,
                         const Babl *format)
{
  GeglRectangle bound = {0, 0, width, height};
  GeglBuffer *buffer;
  gfloat *buf = g_malloc0 (width * height * 16);
  gint i;
  buffer = gegl_buffer_new (&bound, format);
  for (i=0; i < width * height * 4; i++)
    buf[i] = g_random_double_range (-0.5, 2.0);
  gegl_buffer_set (buffer, NULL, 0, babl_format ("RGBA float"), buf, 0);
  g_free (buf);
  return buffer;
}

void do_bench (const gchar *id,
               GeglBuffer  *buffer,
               t_run_perf   test_func,
               gboolean     opencl)
{
  gchar* suffix = "";

  g_object_set(G_OBJECT(gegl_config()),
               "use-opencl", opencl,
               NULL);

  if (opencl) {
    if ( ! gegl_cl_is_accelerated()) {
      g_print("OpenCL is disabled. Skipping OpenCL test\n");
      return;
    }
    suffix = " (OpenCL)";
  }

  // warm up
  test_func(buffer);

  test_start ();
  for (int i=0; i<ITERATIONS && converged<4; ++i)
    {
      test_start_iter();
      test_func(buffer);
      test_end_iter();
    }
  test_end_suffix (id, suffix, ((double)gegl_buffer_get_pixel_count (buffer)) * 16 * ITERATIONS);
}

void bench (const gchar *id,
            GeglBuffer  *buffer,
            t_run_perf   test_func)
{
  do_bench(id, buffer, test_func, FALSE );
  do_bench(id, buffer, test_func, TRUE );
}

