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

   /*load the image to be filtered*/
   GeglNode *image = gegl_node_new_child(gegl,
                                         "operation",
                                         "load",
                                         "path",
                                         "data/surfer.png",
                                         NULL);
   GeglNode *dft = gegl_node_new_child(gegl, "operation", "dft-forward", NULL);
   GeglNode *glpf_filter = gegl_node_new_child(gegl, "operation",
                                               "gaussian-lowpass-filter", "cutoff", 18,
					       "flag", 15, NULL);
   GeglNode *idft = gegl_node_new_child(gegl, "operation", "dft-backward", NULL);
   
   GeglNode *save = gegl_node_new_child(gegl,
                                        "operation",
                                        "png-save",
                                        "path",
                                        "test_result.png",
                                        NULL);
   
   gegl_node_link_many(image,dft,glpf_filter,idft,save,NULL);

   /* request that the save node is processed, all dependencies will
    * be processed as well
    */
   gegl_node_process(save);
   
   g_object_unref(gegl);
 } 

 gegl_exit();
 return 0;
}

