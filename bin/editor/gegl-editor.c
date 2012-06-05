#include <glib.h>
#include <gtk/gtk.h>
#include <cairio.h>


gint
main (gint argc,
      gchar **argv)
{
  GtkWidget *window;

  gtk_init(&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  g_signal_connect (window, "destroy", G_CALLBACK( gtk_main_quit), NULL);

  gtk_widget_show(window);

  gtk_main();
  
  return 0;
}
