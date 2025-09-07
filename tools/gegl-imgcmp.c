#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <gegl.h>
#include <glib.h>

#define DEFAULT_ERROR_DIFFERENCE 1.5
#define MAX_DIFFERENCE_THRESHOLD 0.1
#define SQR(x) ((x) * (x))

typedef enum {
    SUCCESS = 0,
    ERROR_WRONG_ARGUMENTS,
    ERROR_WRONG_SIZE,
    ERROR_PIXELS_DIFFERENT,
} ExitCode;

static gchar *
compute_image_checksum (const gchar *path)
{
  GeglNode      *gegl = NULL;
  GeglNode      *img  = NULL;
  GeglRectangle  comp_bounds;
  guchar        *buf  = NULL;
  gchar         *ret  = NULL;

  gegl = gegl_node_new ();
  img  = gegl_node_new_child (gegl,
                              "operation", "gegl:load",
                              "path", path,
                              NULL);

  comp_bounds = gegl_node_get_bounding_box (img);
  buf         = gegl_malloc (comp_bounds.width * comp_bounds.height * 4);

  gegl_node_blit (img, 1.0, &comp_bounds, babl_format("R'G'B'A u8"), buf, GEGL_AUTO_ROWSTRIDE, GEGL_BLIT_DEFAULT);
  ret = g_compute_checksum_for_data (G_CHECKSUM_MD5, buf, comp_bounds.width * comp_bounds.height * 4);

  gegl_free (buf);
  g_object_unref (gegl);

  return ret;
}

gint
main (gint    argc,
      gchar **argv)
{
  GeglNode      *gegl           = NULL;
  GeglNode      *imgA           = NULL;
  GeglNode      *imgB           = NULL;
  GeglNode      *comparison     = NULL;
  gchar         *md5A           = NULL;
  gchar         *md5B           = NULL;
  gdouble        avg_diff_total = 0;
  gdouble        avg_diff_wrong = 0;
  gdouble        error_diff     = DEFAULT_ERROR_DIFFERENCE;
  gdouble        max_diff       = 0;
  gint           exit_code      = SUCCESS;
  gint           total_pixels   = 0;
  gint           wrong_pixels   = 0;
  GeglRectangle  boundsA;
  GeglRectangle  boundsB;

  gegl_init (&argc, &argv);

  if ((argc != 3) && (argc != 4))
    {
      g_print ("Usage: %s <imageA> <imageB> [<error-difference>]\n"
               "\n"
               "%s is a simple image difference detection tool for use in regression testing.\n"
               "\n"
               "If the two compared images are equal, the exit code is zero.\n"
               "If the two compared images are not equal, the exit code is:\n"
               "  - %2.d - if the sizes of the two images differ,"
               "  - %2.d - if the pixels of the two images differ.",
               argv[0], argv[0], ERROR_WRONG_SIZE, ERROR_PIXELS_DIFFERENT);
      return ERROR_WRONG_ARGUMENTS;
    }

  if (argc == 4)
    {
      gdouble  t   = 0;
      char    *end = NULL;

      errno = 0;
      t = g_ascii_strtod (argv[3], &end);
      if ((errno != ERANGE) && (end != argv[3]) && (end != NULL) && (*end == 0))
        error_diff = t;
    }

  if (access(argv[1], F_OK) != 0)
    {
      g_print ("Missing reference, assuming SUCCESS\n");
      return SUCCESS;
    }

  md5A = compute_image_checksum (argv[1]);
  md5B = compute_image_checksum (argv[2]);

  if (md5A && md5B && g_strcmp0 (md5A, md5B) != 0)
    {
      g_print ("raster md5s differ: %s vs %s\n", md5A, md5B);
    }

  if (access(argv[2], F_OK) != 0)
    {
      g_print ("Missing output image, assuming FAILURE\n");
      return ERROR_PIXELS_DIFFERENT;
   }

  gegl = gegl_node_new ();
  imgA = gegl_node_new_child (gegl,
                              "operation", "gegl:load",
                              "path", argv[1],
                              NULL);
  imgB = gegl_node_new_child (gegl,
                              "operation", "gegl:load",
                              "path", argv[2],
                              NULL);

  boundsA = gegl_node_get_bounding_box (imgA);
  boundsB = gegl_node_get_bounding_box (imgB);
  total_pixels = boundsA.width * boundsA.height;

  if (boundsA.width != boundsB.width || boundsA.height != boundsB.height)
    {
      g_print ("%s and %s differ in size\n", argv[1], argv[2]);
      g_print ("  %ix%i vs %ix%i\n",
                  boundsA.width, boundsA.height, boundsB.width, boundsB.height);
      return ERROR_WRONG_SIZE;
    }

  comparison = gegl_node_create_child (gegl, "gegl:image-compare");
  gegl_node_link (imgA, comparison);
  gegl_node_connect (imgB, "output", comparison, "aux");
  gegl_node_process (comparison);
  gegl_node_get (comparison,
                 "max-diff", &max_diff,
                 "avg-diff-wrong", &avg_diff_wrong,
                 "avg-diff-total", &avg_diff_total,
                 "wrong-pixels", &wrong_pixels,
                 NULL);

  if (max_diff >= MAX_DIFFERENCE_THRESHOLD)
    {
      g_print ("%s and %s differ\n"
               "  wrong pixels   : %i/%i (%2.2f%%)\n"
               "  max Δe         : %2.3f\n"
               "  avg Δe (wrong) : %2.3f(wrong) %2.3f(total)\n",
               argv[1], argv[2],
               wrong_pixels, total_pixels, (wrong_pixels*100.0/total_pixels),
               max_diff,
               avg_diff_wrong, avg_diff_total);

      if (g_strstr_len (argv[2], -1, "broken") == NULL)
        {
          GeglNode *save       = NULL;
          gchar    *debug_path = g_strdup_printf ("%s-diff.png", argv[2]);

          save = gegl_node_new_child (gegl,
                                      "operation", "gegl:png-save",
                                      "path", debug_path,
                                      NULL);
          gegl_node_link (comparison, save);
          gegl_node_process (save);

          g_free (debug_path);
        }

      if (max_diff > error_diff)
        {
          exit_code = ERROR_PIXELS_DIFFERENT;
          g_print ("%s and %s are different.", argv[1], argv[2]);
        }
      else
        {
          g_print ("%s and %s are identical ", argv[1], argv[2]);
          if (strstr (argv[2], "broken"))
            g_print ("because the test is expected to fail.");
          else
            g_print ("because the max error %0.2f is smaller than %0.2f.", max_diff, error_diff);
        }
    }
  else
    {
      g_print ("%s and %s are identical\n", argv[1], argv[2]);
    }

  g_object_unref (gegl);

  gegl_exit ();

  return exit_code;
}
