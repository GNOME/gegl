/* This file is an image processing operation for GEGL
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
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_string (path, _("File"), "/tmp/test.raw")
   description (_("Path of file to load."))

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_NAME     rawbayer_load
#define GEGL_OP_C_SOURCE rawbayer-load.c

#include "gegl-op.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PIPE_MODE "r"
#define MAX_SAMPLE 65535
#define ERROR -1

static void
load_buffer (GeglProperties *op_raw_load)
{
  if (!op_raw_load->user_data)
    {
      FILE  *pfp;
      gchar *command;

      gint width, height, val_max;
      char newline;

      command = g_strdup_printf ("dcraw -j -d -4 -c '%s'\n", op_raw_load->path);
      pfp = popen (command, PIPE_MODE);
      g_free (command);

      if (fscanf (pfp, "P6 %d %d %d %c",
                  &width, &height, &val_max, &newline) != 4)
        {
          pclose (pfp);
          g_warning ("not able to aquire raw data");
          return;
        }

      {
        GeglRectangle extent = { 0, 0, width, height };
        op_raw_load->user_data = (void*)gegl_buffer_new (&extent, babl_format ("Y u16"));
      }
         {
           guchar *buf = g_new (guchar, width * height * 3 * 2);
           fread (buf, 1, width * height * 3 * 2, pfp);
           if(strstr (op_raw_load->path, "rawbayerS")){
             gint i;
             for (i=0;i<width*height*3;i++)
               {
                guchar tmp = buf[i*2];
                buf[i*2] = buf[i*2+1];
                buf[i*2+1] = tmp;
               }
           }
           gegl_buffer_set (GEGL_BUFFER (op_raw_load->user_data),
                            NULL,
                            0,
                            babl_format_new (
                                 babl_model ("RGB"),
                                 babl_type ("u16"),
                                 babl_component ("R"),
                                 babl_component ("G"),
                                 babl_component ("B"),
                                 NULL),
                            buf,
                            GEGL_AUTO_ROWSTRIDE);
           g_free (buf);
         }
       pclose (pfp);
    }
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglProperties   *o = GEGL_PROPERTIES (operation);
  GeglRectangle result = {0,0,0,0};

  load_buffer (o);

  result.width  = gegl_buffer_get_width (GEGL_BUFFER (o->user_data));
  result.height = gegl_buffer_get_height (GEGL_BUFFER (o->user_data));
  return result;
}

static gboolean
process (GeglOperation        *operation,
         GeglOperationContext *context,
         const gchar          *output_pad,
         const GeglRectangle  *result,
         gint                  level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
#if 1
  g_assert (o->user_data);
  gegl_operation_context_take_object (context, "output", G_OBJECT (o->user_data));

  o->user_data = NULL;
#else
  if (o->user_data)
    {
      g_object_ref (o->user_data); /* Add an extra reference, since gegl_operation_set_data
                                      is stealing one.
                                    */

      /* override core behaviour, by resetting the buffer in the operation_context */
      gegl_operation_context_take_object (context, "output", G_OBJECT (o->user_data));
    }
#endif
  return TRUE;
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;

  gegl_operation_class_set_keys (operation_class,
  "name"        , "gegl:rawbayer-load",
  "categories"  , "hidden",
  "description" ,
        _("Raw image loader, wrapping dcraw with pipes, provides the raw bayer"
          " grid as grayscale, if the fileformat is .rawbayer it will use this"
          " loader instead of the normal dcraw loader, if the fileformat is"
          " .rawbayerS it will swap the returned 16bit numbers (the pnm loader"
          " is apparently buggy)"),
        NULL);

  gegl_operation_handlers_register_loader (
    ".rawbayer", "gegl:rawbayer-load");
  gegl_operation_handlers_register_loader (
    ".rawbayerS", "gegl:rawbayer-load");
}

#endif
