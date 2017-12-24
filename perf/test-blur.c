#include "test-common.h"

void blur(GeglBuffer *buffer);

gint
main (gint    argc,
      gchar **argv)
{
  GeglBuffer *buffer;

  gegl_init(&argc, &argv);

  buffer = test_buffer(1024, 1024, babl_format("RGB float"));
  bench("gaussian-blur (RGB)", buffer, &blur);
  g_object_unref (buffer);

  buffer = test_buffer(1024, 1024, babl_format("RaGaBaA float"));
  bench("gaussian-blur (RaGaBaA)", buffer, &blur);
  g_object_unref (buffer);

  buffer = test_buffer(1024, 1024, babl_format("RGBA float"));
  bench("gaussian-blur (RGBA)", buffer, &blur);
  g_object_unref (buffer);

  buffer = test_buffer(1024, 1024, babl_format("Y float"));
  bench("gaussian-blur (Y)", buffer, &blur);
  g_object_unref (buffer);

  buffer = test_buffer(1024, 1024, babl_format("YaA float"));
  bench("gaussian-blur (YaA)", buffer, &blur);
  g_object_unref (buffer);

  buffer = test_buffer(1024, 1024, babl_format("YA float"));
  bench("gaussian-blur (YA)", buffer, &blur);
  g_object_unref (buffer);

  gegl_exit ();
  return 0;
}

void blur(GeglBuffer *buffer)
{
  GeglBuffer *buffer2;
  GeglNode   *gegl, *source, *node, *sink;

  gegl = gegl_node_new ();
  source = gegl_node_new_child (gegl, "operation", "gegl:buffer-source", "buffer", buffer, NULL);
  node = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur",
                                       "std-dev-x", 10.0,
                                       "std-dev-y", 10.0,
                                       NULL);
  sink = gegl_node_new_child (gegl, "operation", "gegl:buffer-sink", "buffer", &buffer2, NULL);

  gegl_node_link_many (source, node, sink, NULL);
  gegl_node_process (sink);
  g_object_unref (gegl);
  g_object_unref (buffer2);
}
