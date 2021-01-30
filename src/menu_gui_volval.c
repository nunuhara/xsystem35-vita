#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

static void
on_checkbutton1_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data);
static void
on_adjustment1_value_changed           (GtkAdjustment *adjustment,
					gpointer       user_data);
static gint
on_window1_destroy                     (GtkWindow     *window,
					gpointer       user_data);



static GtkWidget* vval_win_open(struct _volval *vval, int max) {
  GtkWidget *window1;
  GtkWidget *frame1;
  GtkWidget *table2;
  GtkWidget *label1;
  GtkWidget *hscale1;
  GtkWidget *checkbutton1;
  GtkAdjustment *adjustment1;
  GtkWidget *hbox1;
  GtkWidget *label6;
  GtkWidget *label7;
  int i, j;

  window1 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_object_set_data(G_OBJECT (window1), "window1", window1);
  gtk_widget_set_size_request(window1, 320, -2);
  gtk_window_set_title (GTK_WINDOW (window1), _("window1"));

  frame1 = gtk_frame_new (_("VolumeValance"));
  g_object_ref(frame1);
  g_object_set_data_full(G_OBJECT(window1), "frame1", frame1,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (frame1);
  gtk_container_add (GTK_CONTAINER (window1), frame1);
  gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

  table2 = gtk_table_new (6, 3, FALSE);
  g_object_ref(table2);
  g_object_set_data_full(G_OBJECT(window1), "table2", table2,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (table2);
  gtk_container_add (GTK_CONTAINER (frame1), table2);
  gtk_container_set_border_width (GTK_CONTAINER (table2), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table2), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table2), 2);

  for (i = 0, j = 0; i <= max; i++) {
	  if (vval[i].label == NULL) continue;
	  label1 = gtk_label_new (vval[i].label);
	  g_object_ref(label1);
	  g_object_set_data_full(G_OBJECT(window1), "label1", label1,
				    (GDestroyNotify) g_object_unref);
	  gtk_widget_show (label1);
	  gtk_table_attach (GTK_TABLE (table2), label1, 0, 1, j+1, j+2,
			    (GtkAttachOptions) (0),
			    (GtkAttachOptions) (0), 0, 0);
	  gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_LEFT);
	  
	  adjustment1 = gtk_adjustment_new (vval[i].vol, 0, 100, 0, 10, 0);
	  g_signal_connect(adjustment1, "value-changed",
			      G_CALLBACK(on_adjustment1_value_changed),
			      &(vval[i].vol));
	  
	  hscale1 = gtk_hscale_new (GTK_ADJUSTMENT (adjustment1));
	  g_object_ref(hscale1);
	  g_object_set_data_full(G_OBJECT(window1), "hscale1", hscale1,
				    (GDestroyNotify) g_object_unref);
	  gtk_widget_show (hscale1);
	  gtk_table_attach (GTK_TABLE (table2), hscale1, 1, 2, j+1, j+2,
			    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			    (GtkAttachOptions) (GTK_FILL), 5, 2);
	  gtk_scale_set_draw_value (GTK_SCALE (hscale1), FALSE);
	  gtk_scale_set_digits (GTK_SCALE (hscale1), 0);

	  checkbutton1 = gtk_check_button_new_with_label (_("mute"));
	  g_object_ref(checkbutton1);
	  g_object_set_data_full(G_OBJECT(window1), "checkbutton1", checkbutton1,
				    (GDestroyNotify) g_object_unref);
	  gtk_widget_show (checkbutton1);
	  gtk_table_attach (GTK_TABLE (table2), checkbutton1, 2, 3, j+1, j+2,
			    (GtkAttachOptions) (0),
			    (GtkAttachOptions) (0), 0, 0);
	  g_signal_connect(checkbutton1, "toggled",
			      G_CALLBACK(on_checkbutton1_toggled),
			      &(vval[i].mute));
	  
	  j++;
  }


  hbox1 = gtk_hbox_new (FALSE, 0);
  g_object_ref(hbox1);
  g_object_set_data_full(G_OBJECT(window1), "hbox1", hbox1,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (hbox1);
  gtk_table_attach (GTK_TABLE (table2), hbox1, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

  label6 = gtk_label_new (_("small"));
  g_object_ref(label6);
  g_object_set_data_full(G_OBJECT(window1), "label6", label6,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (label6);
  gtk_box_pack_start (GTK_BOX (hbox1), label6, TRUE, FALSE, 3);
  gtk_label_set_justify (GTK_LABEL (label6), GTK_JUSTIFY_LEFT);

  label7 = gtk_label_new (_("large"));
  g_object_ref(label7);
  g_object_set_data_full(G_OBJECT(window1), "label7", label7,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (label7);
  gtk_box_pack_start (GTK_BOX (hbox1), label7, TRUE, FALSE, 3);
  gtk_label_set_justify (GTK_LABEL (label7), GTK_JUSTIFY_RIGHT);

  g_signal_connect(window1, "delete_event",
		      G_CALLBACK(on_window1_destroy),
		      NULL);

  return window1;
}

static void
on_checkbutton1_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	boolean *muted = (boolean *)user_data;
	
	if (gtk_toggle_button_get_active(togglebutton)) {
		*muted = TRUE; 
	} else {
		*muted = FALSE; 
	}
	s39ini_setvol();
}

static void
on_adjustment1_value_changed           (GtkAdjustment *adjustment,
					gpointer       user_data)
{
	int *vol = (int *)user_data;
	*vol = gtk_adjustment_get_value(adjustment);
	// printf("value chaned %d\n", *vol);
	s39ini_setvol();
}

static gint
on_window1_destroy                     (GtkWindow     *window,
					gpointer       user_data)
{
	s39ini_winclose();
	return TRUE;
}
