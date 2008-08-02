#include "gegl.h"

gint main(gint argc, gchar **argv)
{
  gegl_init(&argc, &argv); /* initialize the GEGL library */
/*
This is the graph we're going to construct:
 
.-----------.
| display   |
`-----------'
   |
.-------.
| idft  |
`-------'
   |   
.-------.
|filter |
`-------'
   |
.-------.
|  dft  |
`-------'
   |
.-------.
| image |
`-------.

*/
 {

    GeglNode *gegl = gegl_node_new();
    gint frame;
    gint x, y;


    GeglNode *image = gegl_node_new_child(gegl,
                                            "operation",
                                            "load",
                                            "path",
                                            "data/surfer.png",
                                            NULL);
    GeglNode *dft = gegl_node_new_child(gegl, "operation", "dft", NULL);
    GeglNode *idft = gegl_node_new_child(gegl,"operation","idft", NULL);
    GeglNode *glpf_filter = gegl_node_new_child(gegl,"operation",
                                             "lowpass-gaussian","cutoff",30,"flag",14,NULL);

    GeglNode *display    = gegl_node_create_child (gegl, "display");

    gegl_node_link_many (image, dft, glpf_filter, idft, display, NULL);

    gegl_node_process(display);

    g_object_unref(gegl);
 } 

  gegl_exit();
  return 0;
}

