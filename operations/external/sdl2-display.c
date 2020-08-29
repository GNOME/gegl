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
 * Copyright 2019 Stefan Brüns <stefan.bruens@rwth-aachen.de>
 *
 * Based on sdl-display.c:
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_string (window_title, _("Window title"), "window_title")
    description (_("Title to be given to output window"))
#else

#define GEGL_OP_SINK
#define GEGL_OP_NAME sdl2_display
#define GEGL_OP_C_SOURCE sdl2-display.c

#include "gegl-op.h"
#include <SDL.h>

typedef struct {
  SDL_Window   *window;
  SDL_Renderer *renderer;
  SDL_Texture  *texture;
  SDL_Surface  *screen;
  gint         width;
  gint         height;
} SDLState;

static void
init_sdl (void)
{
  static int inited = 0;

  if (!inited)
    {
      inited = 1;

      if (SDL_Init (SDL_INIT_VIDEO) < 0)
        {
          fprintf (stderr, "Unable to init SDL: %s\n", SDL_GetError ());
          return;
        }
      atexit (SDL_Quit);
    }
}

static gboolean idle (gpointer data)
{
  SDL_Event event;
  while (SDL_PollEvent  (&event))
    {
      switch (event.type)
        {
          case SDL_QUIT:
            exit (0);
        }
    }
  return TRUE;
}

static guint handle = 0;

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties   *o = GEGL_PROPERTIES (operation);
  SDLState     *state = NULL;

  if(!o->user_data)
      o->user_data = g_new0 (SDLState, 1);
  state = o->user_data;

  init_sdl ();

  if (!handle)
    handle = g_timeout_add (500, idle, NULL);

  if (!state->window ||
       state->width  != result->width ||
       state->height != result->height)
    {

      if (state->window)
        {
          SDL_SetWindowSize (state->window,
                  result->width, result->height);
        }
        else
        {
          if (SDL_CreateWindowAndRenderer (result->width,
                  result->height, 0,
                  &state->window, &state->renderer))
            {
              fprintf (stderr, "Unable to create window: %s\n",
                       SDL_GetError ());
              return -1;
            }
        }

      SDL_FreeSurface (state->screen);
      state->screen = SDL_CreateRGBSurfaceWithFormat (0,
              result->width, result->height, 32, SDL_PIXELFORMAT_RGBA32);
      if (!state->screen)
        {
          fprintf (stderr, "Unable to create surface: %s\n",
                   SDL_GetError ());
          return -1;
        }

      if (state->texture)
        SDL_DestroyTexture (state->texture);
      state->texture = SDL_CreateTextureFromSurface (state->renderer, state->screen);
      if (!state->texture)
        {
          fprintf (stderr, "Unable to create texture: %s\n",
                   SDL_GetError ());
          return -1;
        }

      state->width  = result->width ;
      state->height = result->height;
    }

  /*
   * There seems to be a valid faster path to the SDL desired display format
   * in B'G'R'A, perhaps babl should have been able to figure this out ito?
   *
   */
  gegl_buffer_get (input,
       NULL,
       1.0,
       babl_format ("R'G'B'A u8"),
       state->screen->pixels, GEGL_AUTO_ROWSTRIDE,
       GEGL_ABYSS_NONE);

  SDL_UpdateTexture (state->texture, NULL, state->screen->pixels, state->screen->pitch);

  SDL_RenderClear (state->renderer);
  SDL_RenderCopy (state->renderer, state->texture, NULL, NULL);
  SDL_RenderPresent (state->renderer);
  SDL_SetWindowTitle (state->window, o->window_title);

  return  TRUE;
}

static void
finalize (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);

  g_clear_pointer (&o->user_data, g_free);

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass           *object_class;
  GeglOperationClass     *operation_class;
  GeglOperationSinkClass *sink_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  object_class->finalize = finalize;

  sink_class->process = process;
  sink_class->needs_full = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name",         "gegl:sdl2-display",
    "title",        _("SDL2 Display"),
    "categories",   "display",
    "description",
        _("Displays the input buffer in an SDL2 window (restricted to one"
          " display op/process, due to SDL2 implementation issues)."),
        NULL);
}
#endif
