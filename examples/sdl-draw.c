
#include <gegl.h>
#include <SDL.h>
#include <math.h>

typedef struct
{
  SDL_Window   *window;
  SDL_Renderer *renderer;
  SDL_Surface  *surface;
  SDL_Texture  *texture;
  GeglBuffer *paint_buffer;
  GeglNode   *graph;
  GeglNode   *output_node;

  gboolean in_stroke;
  int last_x;
  int last_y;
} MainContext;

int run_main_loop (MainContext *context);
void init_main_context (MainContext *context);
void destroy_main_context (MainContext *context);
void draw_circle (GeglBuffer *buffer, int x, int y, float r);

const Babl *sdl_format = NULL;

int main(int argc, char *argv[])
{
  int retval;
  MainContext context = {0, };

  if((retval = SDL_Init (SDL_INIT_VIDEO | SDL_INIT_TIMER)) > 0)
    {
      printf("SDL failed with return value %d\n", retval);
      return retval;
    }

  if (SDL_CreateWindowAndRenderer (640, 480, 0,
              &context.window, &context.renderer))
    {
      printf("SDL failed to create a window\n");
      SDL_Quit();
      return 1;
    }

  context.surface = SDL_CreateRGBSurfaceWithFormat (0,
              640, 480, 24, SDL_PIXELFORMAT_RGB24);
  if (!context.surface)
    {
      fprintf (stderr, "Unable to create surface: %s\n",
               SDL_GetError ());
      return 1;
    }

  context.texture = SDL_CreateTextureFromSurface (context.renderer, context.surface);
  if (!context.surface)
    {
      fprintf (stderr, "Unable to create texture: %s\n",
               SDL_GetError ());
      return 1;
    }

  gegl_init (NULL, NULL);

  /* We don't have a native format that matches SDL, but we can use
   * babl to generate the needed conversions automatically.
   */

  sdl_format = babl_format_new (babl_model ("R'G'B'"),
                                babl_type ("u8"),
                                babl_component ("B'"),
                                babl_component ("G'"),
                                babl_component ("R'"),
                                NULL);

  init_main_context (&context);

  run_main_loop (&context);

  destroy_main_context (&context);

  gegl_exit ();
  SDL_Quit ();

  return 0;
}

/* init_main_context:
 * @context: The context.
 *
 * Initialize the main context object that will hold our graph.
 */
void
init_main_context (MainContext *context)
{
  GeglNode   *ptn = gegl_node_new ();
  GeglNode   *background_node, *over, *buffer_src;

  GeglColor  *color1 = gegl_color_new ("rgb(0.4, 0.4, 0.4)");
  GeglColor  *color2 = gegl_color_new ("rgb(0.6, 0.6, 0.6)");
  GeglBuffer *paint_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, 0, 0), babl_format ("RGBA float"));

  g_object_set (ptn, "cache-policy", GEGL_CACHE_POLICY_NEVER, NULL);

  /* Our graph represents a single drawing layer over a fixed background */
  background_node = gegl_node_new_child (ptn,
                                         "operation", "gegl:checkerboard",
                                         "color1", color1,
                                         "color2", color2,
                                         NULL);
  over            = gegl_node_new_child (ptn,
                                         "operation", "gegl:over",
                                         NULL);
  buffer_src      = gegl_node_new_child (ptn,
                                         "operation", "gegl:buffer-source",
                                         "buffer", paint_buffer,
                                         NULL);

  /* The "aux" node of "gegl:over" is the image on top */
  gegl_node_connect_to (background_node, "output", over, "input");
  gegl_node_connect_to (buffer_src, "output", over, "aux");

  g_object_unref (color1);
  g_object_unref (color2);

  context->graph = ptn;
  context->output_node  = over;
  context->paint_buffer = paint_buffer;

}

/* destroy_main_context:
 * @context: The context.
 *
 * Clean up the main context object.
 */
void
destroy_main_context (MainContext *context)
{
  g_object_unref (context->graph);
  g_object_unref (context->paint_buffer);

  SDL_FreeSurface (context->surface);
  SDL_DestroyTexture (context->texture);
  SDL_DestroyRenderer (context->renderer);

  context->graph = NULL;
  context->output_node  = NULL;
  context->paint_buffer = NULL;
}

/* invalidate_signal:
 * @node: The node that was invalidated.
 * @rect: The area that changed.
 * @SDL_Surface: Our user_data param, the window we will write to.
 *
 * Whenever the output of the graph changes this function will copy the new data
 * to the sdl window.
 */
static void
invalidate_signal (GeglNode *node, GeglRectangle *rect, MainContext *context)
{
  SDL_Surface *surface = context->surface;
  GeglRectangle output_rect = {0, 0, surface->w, surface->h};
  guchar *blit_origin = NULL;

  gegl_rectangle_intersect (&output_rect, &output_rect, rect);

  blit_origin = (guchar *)surface->pixels + (output_rect.x * surface->format->BytesPerPixel + output_rect.y * surface->pitch);

  gegl_node_blit (node,
                  1.0,
                  &output_rect,
                  sdl_format,
                  blit_origin,
                  surface->pitch,
                  0);

  SDL_UpdateTexture (context->texture, NULL, surface->pixels, surface->pitch);

  SDL_RenderClear (context->renderer);
  SDL_RenderCopy (context->renderer, context->texture, NULL, NULL);
  SDL_RenderPresent (context->renderer);
}

/* draw_circle:
 * @buffer: The buffer to draw on.
 * @x: Center of the circle.
 * @y: Center of the circle.
 * @r: Radius of the circle.
 *
 * Draw a black circle with soft edges on @buffer at @x, @y.
 */
void
draw_circle (GeglBuffer *buffer, int x, int y, float r)
{
  GeglRectangle roi;
  GeglBufferIterator *iter;

  float color_pixel[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  float r_sqr = r*r;

  roi.x = x - r - 0.5;
  roi.y = y - r - 0.5;
  roi.width  = 2.0 * r + 1.5;
  roi.height = 2.0 * r + 1.5;

  if (roi.width < 1)
    return;
  if (roi.height < 1)
    return;

  iter = gegl_buffer_iterator_new (buffer, &roi, 0,
                                   babl_format ("RGBA float"),
                                   GEGL_ACCESS_READWRITE,
                                   GEGL_ABYSS_NONE, 1);

  while (gegl_buffer_iterator_next (iter))
    {
      float *pixel = (float *)iter->items[0].data;
      int iy, ix;
      GeglRectangle roi = iter->items[0].roi;


      for (iy = roi.y; iy < roi.y + roi.height; iy++)
        {
          for (ix = roi.x; ix < roi.x +  roi.width; ix++)
            {
              float d_sqr = powf(x-ix, 2.0f) + powf(y-iy, 2.0f);
                if (d_sqr < r_sqr)
                {
                  float dist = sqrt(d_sqr);
                  if (dist < r - 1)
                    {
                      pixel[0] = color_pixel[0];
                      pixel[1] = color_pixel[1];
                      pixel[2] = color_pixel[2];

                      if (pixel[3] < color_pixel[3])
                        pixel[3] = color_pixel[3];
                    }
                  else
                    {
                      float alpha = (r - dist) * (color_pixel[3]);
                      float dst_alpha = pixel[3];

                      float a = alpha + dst_alpha * (1.0 - alpha);
                      float a_term = dst_alpha * (1.0 - alpha);
                      float r = color_pixel[0] * alpha + pixel[0] * a_term;
                      float g = color_pixel[1] * alpha + pixel[1] * a_term;
                      float b = color_pixel[2] * alpha + pixel[2] * a_term;

                      pixel[0] = r / a;
                      pixel[1] = g / a;
                      pixel[2] = b / a;

                      if (pixel[3] < alpha)
                        pixel[3] = alpha;
                    }
                }

              pixel += 4;
            }
        }
    }
}

int
run_main_loop (MainContext *context)
  {
    SDL_Surface *surface = context->surface;
    GeglRectangle initial_rect = {0, 0, surface->w, surface->h};

    gegl_buffer_set_extent (context->paint_buffer, GEGL_RECTANGLE (0, 0, surface->w, surface->h));

    /* initial buffers update */
    invalidate_signal (context->output_node, &initial_rect, context);

    /* This signal will trigger to update the surface when the output node's
     * contents change. Updating instantly is very inefficient but is good
     * enough for this example.
     */
    g_signal_connect (context->output_node, "invalidated",
                      G_CALLBACK (invalidate_signal), context);

    while(1)
      {
        SDL_Event event;

        SDL_WaitEvent (&event);

        switch (event.type)
          {
            case SDL_QUIT:
              return 0;
              break;
            case SDL_MOUSEBUTTONDOWN:
              if (event.button.button == SDL_BUTTON_LEFT)
                {
                  context->in_stroke = TRUE;
                  context->last_x = event.button.x;
                  context->last_y = event.button.y;
                  draw_circle (context->paint_buffer, event.button.x, event.button.y, 20);
                }
              break;
            case SDL_MOUSEMOTION:
              if (context->in_stroke &&
                  (context->last_x != event.motion.x || context->last_y != event.motion.y))
                {
                  context->last_x = event.motion.x;
                  context->last_y = event.motion.y;
                  draw_circle (context->paint_buffer, event.motion.x, event.motion.y, 20);
                }
              break;
            case SDL_MOUSEBUTTONUP:
              if (event.button.button == SDL_BUTTON_LEFT)
                {
                  context->in_stroke = FALSE;
                }
              break;
          }
      }
  }
