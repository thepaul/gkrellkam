/*
  gkrellkam.so -- image watcher plugin for GKrellM
  Copyright (C) 2001 paul cannon

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
      02111-1307, USA.

  To contact the author try:
  <paul@cannon.cs.usu.edu>
*/

#include <gkrellm/gkrellm.h>
#include <errno.h>

#define PLUGIN_NAME "GKrellKam"
#define PLUGIN_VER "0.1.1d"
#define PLUGIN_DESC "GKrellM Image Watcher plugin"
#define PLUGIN_URL "http://gkrellkam.sourceforge.net/"
#define PLUGIN_STYLE PLUGIN_NAME
#define PLUGIN_KEYWORD PLUGIN_NAME

static gchar *kkam_info_text[] = 
{
  "This plugin watches an image file (of practically any\n",
  "format and size) on the local hard disk and displays\n",
  "it, sized to fit, in a little box on your gkrellm.\n\n",
  
  "Before updating the picture, the plugin will call a\n",
  "script file in your path that can fetch a new image\n",
  "from the web (like a webcam picture), or do anything\n",
  "else it wants to do (you might rotate some of your\n",
  "own images).\n\n",

  "GKrellKam expects the script to output the full path\n",
  "of the image to load, after the image is ready, and\n",
  "then exit. Once GKrellKam starts the script, it will\n",
  "keep checking for its output every two seconds, until\n",
  "it either knows where the image is, or knows something\n",
  "has gone wrong. After that it will do as little as\n",
  "possible until it's time to load the next image.\n\n",

  "Changes to the image height setting don't take effect\n",
  "until you restart or reinitialize gkrellm.\n\n",

  "If you left-click on the thumbnail image, GKrellKam can\n",
  "start an image viewer to display the unscaled version.\n",
  "Right-clicking on it forces an immediate update, and\n",
  "the count is reset."
};

static gchar *kkam_about_text = _(
  PLUGIN_NAME " " PLUGIN_VER
  "\n" PLUGIN_DESC
  "\n\nCopyright (C) 2001 paul cannon\n"
  "paul@cannon.cs.usu.edu\n"
  "space software lab/utah state university\n\n"
  PLUGIN_NAME " comes with ABSOLUTELY NO WARRANTY;\n"
  "see the file COPYING for details.\n\n"
  PLUGIN_URL );

#define PARAMLEN 256

static Panel *img_panel = NULL;
static Decal *img_decal = NULL;
static GdkPixmap *img_pixmap = NULL;
static Style *img_style = NULL;
static gint style_id;

static int kkam_height = 50;
static int use_height;
static int kkam_period = 1;
static char imgfname[PARAMLEN] = "";
static char upd_script[PARAMLEN] = "krellkam_load";
static char viewer_prog[PARAMLEN] = "eeyes";

static GtkWidget *period_spinner;
static GtkWidget *height_spinner;
static GtkWidget *scriptbox;
static GtkWidget *viewerbox;

static FILE *cmd_pipe = NULL;

void do_nothing (char *format, ...) {}

/*
  start_img_update ()

  Open a pipe and spawn the update script. Return value
  indicates whether pipe should be checked for output every
  two seconds (1), or just ignored until next update (0)
*/
static void start_img_update ()
{
  if (cmd_pipe) /* already open */
    return;

  cmd_pipe = popen (upd_script, "r");
  if (cmd_pipe)
    fcntl (fileno (cmd_pipe), F_SETFL, O_NONBLOCK);
}

/*
  get_script_results (char *fname)

  Checks if results are available from update script. If they are,
  copies output (a string indicating the new file name) into
  fname, pcloses the pipe, and returns 1.

  On error, returns -1. On not yet ready, returns 0.
*/
static int get_script_results (char *fname)
{
  char buffer[PARAMLEN];
  int len;

  if (fread (buffer, sizeof (char), 1, cmd_pipe) < 1)
  {
    /* if we get EAGAIN, wait some more */
    if (ferror (cmd_pipe) && errno == EAGAIN)
      return 0;

    /* if we reach here something has gone wrong with the pipe.
       return error code */
    pclose (cmd_pipe);
    cmd_pipe = NULL;
    return -1;
  }
  
  /* if we get here the pipe is ready. */
  len = fread (&buffer[1], sizeof (char), PARAMLEN - sizeof (char), cmd_pipe);
  buffer[len] = '\0';
  strncpy (imgfname, buffer, PARAMLEN);
  pclose (cmd_pipe);
  cmd_pipe = NULL;
  return 1;
}

/*
  checkscript ()
  
  If results are available from update script, load the new file and
  draw it onto the panel.
*/
static void checkscript ()
{
  static GdkImlibImage *imlibim;
  struct stat img_st;

  if (!cmd_pipe)
    return;

  if (get_script_results (imgfname) < 1)
    return;

  /* make sure file is really there. when the imlib stuff
     fails it's really loud */
  if (stat (imgfname, &img_st) == -1)
    return;

  if ((imlibim = gdk_imlib_load_image (imgfname)) == NULL)
    return;

  gkrellm_destroy_decal_list (img_panel);
  if (img_pixmap)
  {
    gdk_pixmap_unref (img_pixmap);
    img_pixmap = NULL;
  }
  gkrellm_render_to_pixmap (imlibim, &img_pixmap, NULL,
                            gkrellm_chart_width (), use_height + 1);
  gdk_imlib_kill_image (imlibim);

  img_decal = gkrellm_create_decal_pixmap (img_panel, img_pixmap, NULL, 1,
                                           img_style, 0, 0);
  gkrellm_draw_decal_pixmap (img_panel, img_decal, 0);
  gkrellm_draw_layers (img_panel);
}

/*
  kkam_update_plugin ()

  callback to gkrellm. Counts minutes until next update, and when
  count is reached, starts the process.
*/
static void kkam_update_plugin ()
{
  static int count = 0;

  if (GK.two_second_tick && cmd_pipe)
  {
    checkscript ();
    count = 0;
  }

  if (GK.minute_tick && (count = (count + 1) % kkam_period) == 0)
    start_img_update ();
}

/*
  show_curimage ()

  launches eeyes, or whatever viewer the user has configured, to
  display the unscaled version of the image (looking where
  we left it)
*/
static gint show_curimage (GtkWidget *widget, GdkEventButton *ev)
{
  char cmd[PARAMLEN * 2];
  
  switch (ev->button)
  {
  case 1:
    snprintf (cmd, PARAMLEN * 2, "%s '%s' &", viewer_prog, imgfname);
    system (cmd);
    break;
  case 3:
    start_img_update ();
    break;
  }
  return FALSE;
}

static gint panel_expose_event (GtkWidget *widget, GdkEventExpose *ev)
{
  if (img_pixmap)
  {
    gdk_draw_pixmap (widget->window,
         widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
         img_pixmap, ev->area.x, ev->area.y, ev->area.x, ev->area.y,
         ev->area.width, ev->area.height);
  }
  return FALSE;
}

static void kkam_create_plugin (GtkWidget *vbox, gint first_create)
{
  if (first_create)
  {
    start_img_update ();
    img_panel = gkrellm_panel_new0 ();
  }

  img_style = gkrellm_meter_style (style_id);
  img_panel->textstyle = gkrellm_meter_textstyle (style_id);
  img_panel->label->h_panel = kkam_height;
  use_height = kkam_height;
  gkrellm_create_panel (vbox, img_panel, gkrellm_bg_meter_image (style_id));
  gkrellm_monitor_height_adjust(img_panel->h);

  if (first_create)
  {
    gtk_signal_connect (GTK_OBJECT (img_panel->drawing_area),
          "expose_event", (GtkSignalFunc) panel_expose_event, NULL);
    gtk_signal_connect (GTK_OBJECT (img_panel->drawing_area),
          "button_press_event", (GtkSignalFunc) show_curimage, NULL);
  }
  else if (img_decal && img_decal->pixmap)
  {
    gkrellm_draw_decal_pixmap (img_panel, img_decal, 0);
    gkrellm_draw_layers (img_panel);
  }
}

static void kkam_save_config (FILE *f)
{
  fprintf (f, "%s img_height %d\n", PLUGIN_KEYWORD, kkam_height);
  fprintf (f, "%s update_period %d\n", PLUGIN_KEYWORD, kkam_period);
  fprintf (f, "%s update_script %s\n", PLUGIN_KEYWORD, upd_script);
  fprintf (f, "%s viewer_prog %s\n", PLUGIN_KEYWORD, viewer_prog);
}

static void kkam_load_config (gchar *arg)
{
  gchar *config_item, *value;

  config_item = strtok (arg, " \n");
  if (!config_item)
    return;
  value = strtok (NULL, "\n");

  if (!strcmp (config_item, "img_height"))
  {
    kkam_height = atoi (value);
    if (kkam_height < 10)
      kkam_height = 10;
    if (kkam_height > 100)
      kkam_height = 100;
  }
  else if (!strcmp (config_item, "update_period"))
  {
    kkam_period = atoi (value);
    if (kkam_period < 1)
      kkam_period = 1;
  }
  else if (!strcmp (config_item, "update_script"))
  {
    strncpy (upd_script, value, PARAMLEN - 1);
    upd_script[PARAMLEN - 1] = '\0';
  }
  else if (!strcmp (config_item, "viewer_prog"))
  {
    strncpy (viewer_prog, value, PARAMLEN - 1);
    viewer_prog[PARAMLEN - 1] = '\0';
  }
}

static void kkam_create_tab (GtkWidget *tab_vbox)
{
  GtkWidget *tabs;
  GtkWidget *vbox, *hbox1, *hbox2;
  GtkWidget *scrolled;
  GtkWidget *text, *label1, *label2;
  GtkWidget *about, *aboutlabel;

  tabs = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (tabs), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (tab_vbox), tabs, TRUE, TRUE, 0);

  /* Setup Tab */
  vbox = gkrellm_create_tab(tabs, _("Options"));

  gkrellm_spin_button (vbox, &period_spinner,
                       (gfloat) kkam_period,
                       1.0, 500.0, 1.0, 10.0, 0, 60, NULL, NULL,
                       FALSE, _("Minutes per update"));

  gkrellm_spin_button (vbox, &height_spinner,
                       (gfloat) kkam_height,
                       10.0, 100.0, 1.0, 5.0, 0, 60, NULL, NULL,
                       FALSE, _("Height of viewer, in pixels"));

  hbox1 = gtk_hbox_new (FALSE, 0);
  hbox2 = gtk_hbox_new (FALSE, 0);
  label1 = gtk_label_new (_("Script to execute for image update:  "));
  label2 = gtk_label_new (_("Path to image viewer program:  "));
  scriptbox = gtk_entry_new ();
  viewerbox = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (scriptbox), upd_script);
  gtk_entry_set_editable (GTK_ENTRY (scriptbox), TRUE);
  gtk_entry_set_text (GTK_ENTRY (viewerbox), viewer_prog);
  gtk_entry_set_editable (GTK_ENTRY (viewerbox), TRUE);

  gtk_box_pack_start (GTK_BOX (hbox1), label1, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox1), scriptbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox1, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), label2, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), viewerbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, TRUE, TRUE, 0);

  /* Info tab */

  vbox = gkrellm_create_tab (tabs, _("Info"));
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
  text = gtk_text_new (NULL, NULL);
  gkrellm_add_info_text (text, kkam_info_text,
      sizeof (kkam_info_text) / sizeof (gchar *));
  gtk_text_set_editable (GTK_TEXT (text), FALSE);
  gtk_container_add (GTK_CONTAINER (scrolled), text);

  /* About tab */

  about = gtk_label_new (kkam_about_text);
  aboutlabel = gtk_label_new ("About");
  gtk_notebook_append_page (GTK_NOTEBOOK (tabs), about, aboutlabel);  
}

static void kkam_apply_config ()
{
  gchar *newval;

  newval = gtk_editable_get_chars (GTK_EDITABLE (scriptbox), 0, -1);
  strncpy (upd_script, newval, PARAMLEN);
  upd_script[PARAMLEN - 1] = '\0';
  g_free (newval);

  newval = gtk_editable_get_chars (GTK_EDITABLE (viewerbox), 0, -1);
  strncpy (viewer_prog, newval, PARAMLEN);
  viewer_prog[PARAMLEN - 1] = '\0';
  g_free (newval);

  kkam_period = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (period_spinner));
  kkam_height = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (height_spinner));
}

static Monitor kam_mon  =
{
  PLUGIN_NAME,         /* Name, for config tab.                    */
  0,                   /* Id,  0 if a plugin                       */
  kkam_create_plugin,  /* The create_plugin() function             */
  kkam_update_plugin,  /* The update_plugin() function             */
  kkam_create_tab,     /* The create_plugin_tab() config function  */
  kkam_apply_config,   /* The apply_plugin_config() function       */

  kkam_save_config,    /* The save_plugin_config() function        */
  kkam_load_config,    /* The load_plugin_config() function        */
  PLUGIN_KEYWORD,      /* config keyword                           */

  NULL,                /* Undefined 2                              */
  NULL,                /* Undefined 1                              */
  NULL,                /* private                                  */

  MON_CPU,             /* Insert plugin before this monitor.       */
  NULL,                /* Handle if a plugin, filled in by GKrellM */
  NULL                 /* path if a plugin, filled in by GKrellM   */
};

Monitor *init_plugin ()
{
  style_id = gkrellm_add_meter_style (&kam_mon, PLUGIN_STYLE);
  return &kam_mon;
}

