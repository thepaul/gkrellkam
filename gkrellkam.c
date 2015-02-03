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

  A note on style here:

    I use (gchar *) for a string's type when the array is dynamically
    allocated with the g_* functions, and will eventually need to be
    g_free'd. Strings of type (char *) are static memory.
*/

#include <gkrellm/gkrellm.h>
#include <libgen.h>
#include <errno.h>

#define PLUGIN_NAME "GKrellKam"
#define PLUGIN_VER "0.2.2d"
#define PLUGIN_DESC "GKrellM Image Watcher plugin"
#define PLUGIN_URL "http://gkrellkam.sourceforge.net/"
#define PLUGIN_STYLE PLUGIN_NAME
#define PLUGIN_KEYWORD PLUGIN_NAME

static gchar *kkam_info_text[] = 
{
"<b>" PLUGIN_NAME " " PLUGIN_VER "\n\n",

"GKrellKam is a plugin that can watch a number of image files,\n",
"and display them sized to fit in panels on your gkrellm.\n\n",

"You can left-click on an image panel to start your favorite\n",
"image viewer and display the original, unsized image, or you\n",
"can right-click on it to get an immediate update.\n\n",

"<b>-- CONFIGURATION TABS --\n\n",

"<b>Global Options\n\n",

"<i>Path to image viewer program\n",
"When you left-click on a GKrellKam panel, it will start your\n",
"favorite image viewer and display the original, unsized image.\n",
"Put the name of the image viewer program in this box.\n\n",

"<i>Number of panels\n",
"This lets you adjust the number of visible GKrellKam panels\n",
"between 0 and 5.\n\n",

"<b>Panel Config Tabs\n\n",

"<i>Minutes per update\n",
"After an image has been successfully loaded, GKrellKam will\n",
"wait this many minutes before calling the panel's script\n",
"again. If you right-click on a panel, it will call the\n",
"script immediately for an update.\n\n",

"<i>Height of viewer, in pixels\n",
"You can adjust this for each panel to give your pictures a\n",
"nice-looking aspect.\n\n",

"<i>Border size\n",
"Adds a border space around the image to match better with\n",
"your GKrellM theme.\n\n",

"<i>Maintain aspect ratio\n",
"When checked, images are not sized to fit perfectly in this\n",
"panel. They maintain their aspect, and theme background is\n",
"shown on either the sides or the top and bottom.\n\n",

"<i>Image Source\n",
"Each panel has a `source' associated with it. A source can be\n",
"a local picture, the web address of a picture, a list of\n",
"pictures, or a script to call to get a new picture. To give a\n",
"local picture file, list of files, or a script, enter the\n",
"_full_ filename in the \"Image Source\" box. To watch a webcam\n",
"or other online picture, just put its address (beginning with\n",
"http:// or ftp://) in the \"Image Source\" box. GKrellKam uses\n",
"a script called krellkam_load, included with this distribution,\n",
"to manage lists of images and download remote ones. In fact,\n",
"anything you put in the \"Image Source\" box is simply given\n",
"to krellkam_load as its command line parameters. Run\n",
"\"krellkam_load --help\" for more detailed usage instructions.\n\n",

"<b>For more info, see " PLUGIN_URL
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

const char *default_source[] = {
  "http://www.usu.edu/webcam/usucam.jpg",
  "~/.krellkam2.list",
  "~/.krellkam3.list",
  "~/.krellkam4.list",
  "~/.krellkam5.list"
};

const char *default_viewer = "eeyes";

#define WGET_SCRIPT "krellkam_load"
#define BUFLEN 256
#define MIN_NUMPANELS 0
#define MAX_NUMPANELS 5

/* items that each panel needs */

typedef struct
{
  Panel *panel;
  Decal *decal;
  GdkPixmap *pixmap;
  FILE *cmd_pipe;
  int count;
  char *imgfname;
  char *source;
  int height;
  int period;
  int boundary;
  gboolean maintain_aspect;
  gint visible;
  GtkWidget *period_spinner;
  GtkWidget *boundary_spinner;
  GtkWidget *height_spinner;
  GtkWidget *aspect_box;
  GtkWidget *sourcebox;
  GdkImlibImage *imlibim;
} KKamPanel;

static int created = 0;
static int numpanels = 0;
static int newnumpanels = 1;
static KKamPanel *panels = NULL;

static Style *img_style = NULL;
static gint style_id;

static char *viewer_prog = NULL;

static GtkWidget *viewerbox;
static GtkWidget *numpanel_spinner;
static GtkWidget *tabs = NULL;
static GtkWidget *kkam_vbox = NULL;

static void change_num_panels ();

/*
  validnum ()

  returns TRUE if the given number is a valid index
  into the panels array
*/
static gboolean validnum (int num)
{
  return (panels && num >= 0 && num < MAX_NUMPANELS);
}

/*
  activenum ()

  returns TRUE if the given number is an active index
  into the panels array
*/
static gboolean activenum (int num)
{
  return (panels && num >= 0 && num < numpanels);
}

/*
  draw_imlibim ()
  
  renders the current image into the panel at the right size.
  aspect-scaling patch from Benjamin Johnson (benj@visi.com)- thanks
*/
static void draw_imlibim (KKamPanel *p)
{
  int pan_x, pan_y;       /* panel x and y sizes */
  int scale_x, scale_y;   /* size of scaled image */
  int loc_x, loc_y;       /* location for scaled image */

  if (p->imlibim == NULL)
    return;

  pan_x = gkrellm_chart_width () - 2 * p->boundary;
  pan_y = p->height - 2 * p->boundary;

  /* need to blank out the old image here by redrawing the background
     maybe there is a better way to do this?? */
  gkrellm_render_to_pixmap (gkrellm_bg_meter_image (style_id),
                            &(p->pixmap), NULL,
                            gkrellm_chart_width (), p->height);
  gkrellm_destroy_decal_list (p->panel);
  p->decal = gkrellm_create_decal_pixmap (p->panel, p->pixmap,
                                          NULL, 1, img_style, 0, 0);
  gkrellm_draw_decal_pixmap (p->panel, p->decal, 0);
  gkrellm_draw_layers (p->panel);

  if (p->maintain_aspect)
  {
    /* determine sizing here - maintain aspect ratio */

    if (pan_x >= p->imlibim->rgb_width && pan_y >= p->imlibim->rgb_height)
    {
      /* the image is smaller then the panel. do no sizing, just center. */

      loc_x = (pan_x - p->imlibim->rgb_width) / 2 + p->boundary;
      loc_y = (pan_y - p->imlibim->rgb_height) / 2 + p->boundary;

      scale_x = 0; /* scale of 0 defaults to use image size */
      scale_y = 0;
    }
    else if ((double)p->imlibim->rgb_width / (double)pan_x >
             (double)p->imlibim->rgb_height / (double)pan_y)
    {
      /* scale to width (image is wider compared to panel size) */

      scale_x = pan_x;
      scale_y = p->imlibim->rgb_height * pan_x / p->imlibim->rgb_width;

      loc_x = p->boundary;
      loc_y = (pan_y - scale_y) / 2 + p->boundary;
    }
    else
    {
      /* scale to height (image is taller compared to panel size) */

      scale_x = p->imlibim->rgb_width * pan_y / p->imlibim->rgb_height;
      scale_y = pan_y;

      loc_x = (pan_x - scale_x) / 2 + p->boundary;
      loc_y = p->boundary;
    }
  }
  else
  {
    /* Scale to size of panel */

    scale_x = pan_x;
    scale_y = pan_y;

    loc_x = p->boundary;
    loc_y = p->boundary;
  }
  
  gkrellm_render_to_pixmap (p->imlibim, &(p->pixmap), NULL,
                            scale_x, scale_y);

  gkrellm_destroy_decal_list (p->panel);
  p->decal = gkrellm_create_decal_pixmap (p->panel, p->pixmap,
                                          NULL, 1, img_style, loc_x, loc_y);
  gkrellm_draw_decal_pixmap (p->panel, p->decal, 0);
  gkrellm_draw_layers (p->panel);
}

/*
  start_img_update ()

  Open a pipe and spawn the update script.
*/
static void start_img_update (KKamPanel *p)
{
  gchar *krellkam_load_str;

  if (p->cmd_pipe) /* already open */
    return;

  if (!p->source)
  {
    fprintf (stderr, "error: source is null\n");
    return;
  }

  krellkam_load_str = g_strdup_printf ("%s %s", WGET_SCRIPT, p->source);
  p->cmd_pipe = popen (krellkam_load_str, "r");
  g_free (krellkam_load_str);
  if (p->cmd_pipe)
    fcntl (fileno (p->cmd_pipe), F_SETFL, O_NONBLOCK);
}

/*
  get_script_results (char *fname)

  Checks if results are available from update script. If they are,
  copies output (a string indicating the new file name) into
  fname, pcloses the pipe, and returns 1.

  On error, returns -1. On not yet ready, returns 0.
*/
static int get_script_results (KKamPanel *p)
{
  char buffer[BUFLEN];
  int len;

  if (fread (buffer, sizeof (char), 1, p->cmd_pipe) < 1)
  {
    /* if we get EAGAIN, wait some more */
    if (ferror (p->cmd_pipe) && errno == EAGAIN)
      return 0;

    /* if we reach here something has gone wrong with the pipe.
       return error code */
    pclose (p->cmd_pipe);
    p->cmd_pipe = NULL;
    return -1;
  }
  
  /* if we get here the pipe is ready. */
  len = fread (&buffer[1], sizeof (char), BUFLEN - sizeof (char),
               p->cmd_pipe);
  buffer[len] = '\0';
  if (p->imgfname)
    g_free (p->imgfname);
  p->imgfname = g_strdup (buffer);

  pclose (p->cmd_pipe);
  p->cmd_pipe = NULL;
  return 1;
}

/*
  checkscript ()
  
  If results are available from update script, load the new file and
  draw it onto the panel.
*/
static void checkscript (KKamPanel *p)
{
  struct stat img_st;

  if (!p->cmd_pipe)
    return;

  if (get_script_results (p) < 1)
    return;

  /* make sure file is really there. when the imlib stuff
     fails it's really loud */
  if (stat (p->imgfname, &img_st) == -1)
    return;

  if (p->imlibim)
    gdk_imlib_kill_image (p->imlibim);
  p->imlibim = gdk_imlib_load_image (p->imgfname);

  draw_imlibim (p);
}

/*
  kkam_update_plugin ()

  callback to gkrellm. Counts minutes until next update, and when
  count is reached, starts the update process.
*/
static void kkam_update_plugin ()
{
  int i;

  if (GK.two_second_tick)
    for (i = 0; i < numpanels; i++)
      if (panels[i].cmd_pipe)
      {
        checkscript (&panels[i]);
        panels[i].count = 0;
      }

  if (GK.minute_tick)
    for (i = 0; i < numpanels; i++)
      if ((panels[i].count = (panels[i].count + 1) % panels[i].period) == 0)
        start_img_update (&panels[i]);
}

/*
  show_curimage ()

  launches eeyes, or whatever viewer the user has configured, to
  display the unscaled version of the image (looking where
  we loaded it). If the user right-clicked, get a fresh image.
*/
static gint show_curimage (GtkWidget *widget, GdkEventButton *ev, gpointer gw)
{
  gchar *cmd;
  int which;
 
  which = GPOINTER_TO_INT (gw);
  if (!activenum (which))
    return FALSE;
  
  switch (ev->button)
  {
  case 1:
    if (panels[which].imgfname)
    {
      cmd = g_strdup_printf ("%s '%s' &", viewer_prog, panels[which].imgfname);
      system (cmd);
      g_free (cmd);
    }
    break;
  case 3:
    start_img_update (&panels[which]);
    break;
  }
  return FALSE;
}

static gint panel_expose_event (GtkWidget *widget,
                                GdkEventExpose *ev,
                                gpointer gw)
{
  int which;

  which = GPOINTER_TO_INT (gw);
  if (!activenum (which))
    return FALSE;

  gdk_draw_pixmap (widget->window,
       widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
       panels[which].panel->pixmap,
       ev->area.x, ev->area.y, ev->area.x, ev->area.y,
       ev->area.width, ev->area.height);

  return FALSE;
}

static void cb_boundary_spinner (gpointer s, KKamPanel *p)
{
  p->boundary = gtk_spin_button_get_value_as_int
                                 (GTK_SPIN_BUTTON (p->boundary_spinner));
  gkrellm_config_modified ();
  draw_imlibim (p);
}

static void cb_aspect_box (gpointer s, KKamPanel *p)
{
  p->maintain_aspect = GTK_TOGGLE_BUTTON (p->aspect_box)->active;
  gkrellm_config_modified ();
  draw_imlibim (p);
}

static void cb_height_spinner (gpointer s, KKamPanel *p)
{
  int newheight;

  newheight = gtk_spin_button_get_value_as_int (
                                   GTK_SPIN_BUTTON (p->height_spinner));
  
  if (newheight != p->height)
  {
    gkrellm_monitor_height_adjust (newheight - p->height);
      
    p->panel->label->h_panel = newheight;
    p->height = newheight;

    gkrellm_create_panel (kkam_vbox, p->panel,
                          gkrellm_bg_meter_image (style_id));
    gkrellm_pack_side_frames ();
    gkrellm_config_modified ();
                            
    draw_imlibim (p);
  }
}

/*
  create_configpanel_tab ()

  Creates a GtkVbox for the configuration of one of the panels
*/
static GtkWidget *create_configpanel_tab (int i)
{
  GtkWidget *vbox, *sourcehbox, *sourcelabel;

  vbox = gtk_vbox_new (FALSE, 0);

  gkrellm_spin_button (vbox, &panels[i].period_spinner,
                       (gfloat) panels[i].period,
                       1.0, 500.0, 1.0, 10.0, 0, 0, NULL, NULL,
                       FALSE, _("Minutes per update"));

  gkrellm_spin_button (vbox, &panels[i].height_spinner,
                       (gfloat) panels[i].height,
                       10.0, 100.0, 1.0, 5.0, 0, 0, cb_height_spinner, &panels[i],
                       FALSE, _("Height of viewer, in pixels"));

  gkrellm_spin_button (vbox, &panels[i].boundary_spinner,
                       (gfloat) panels[i].boundary,
                       0.0, 20.0, 1.0, 1.0, 0, 0, cb_boundary_spinner, &panels[i],
                       FALSE, _("Border size"));
  
  gkrellm_check_button (vbox, &panels[i].aspect_box,
                        panels[i].maintain_aspect,
                        TRUE, 0, _("Maintain aspect ratio"));
  
  gtk_signal_connect (GTK_OBJECT (panels[i].aspect_box), "toggled",
                      GTK_SIGNAL_FUNC (cb_aspect_box), &panels[i]);

  sourcehbox = gtk_hbox_new (FALSE, 0);
  sourcelabel = gtk_label_new (_("Image source:  "));
  panels[i].sourcebox = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (panels[i].sourcebox), panels[i].source);
  gtk_entry_set_editable (GTK_ENTRY (panels[i].sourcebox), TRUE);

  gtk_box_pack_start (GTK_BOX (sourcehbox), sourcelabel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (sourcehbox), panels[i].sourcebox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), sourcehbox, TRUE, TRUE, 0);

  gtk_widget_show_all (vbox);

  return vbox;
}

/*
  insert_configpanel_tab ()

  Creates a new configration tab and inserts it into the config box
*/
static void insert_configpanel_tab (int i)
{
  GtkWidget *label, *configpanel;
  gchar *labeltxt;
  
  if (!GTK_IS_OBJECT (tabs))
    return;

  configpanel = create_configpanel_tab (i);
  
  labeltxt = g_strdup_printf ("Panel #%i", i + 1);
  label = gtk_label_new (labeltxt);
  g_free (labeltxt);
  
  gtk_notebook_insert_page (GTK_NOTEBOOK (tabs), configpanel, label, i + 1);
}

/*
  remove_configpanel_tab ()

  Removes a configuration tab from the config box
*/
static void remove_configpanel_tab (int i)
{
  if (!GTK_OBJECT (tabs))
    return;

  gtk_notebook_remove_page (GTK_NOTEBOOK (tabs), i + 1);
}

/*
  change_num_panels ()

  called to move the number in `newnumpanels' into
  `numpanels' with all associated changes to the
  monitor
*/
static void change_num_panels ()
{
  int i;
  
  if (numpanels == newnumpanels)
    return;

  if (created)
  {
    for (i = numpanels - 1; i >= newnumpanels; i--)
    {
      remove_configpanel_tab (i);
      if (panels[i].cmd_pipe)
      {
        pclose (panels[i].cmd_pipe);
        panels[i].cmd_pipe = NULL;
      }
    }

    for (i = 0; i < MAX_NUMPANELS; i++)
    {
      gkrellm_enable_visibility (i < newnumpanels, &(panels[i].visible),
                                 panels[i].panel->drawing_area,
                                 panels[i].panel->h);
    }

    for (i = numpanels; i < newnumpanels; i++)
    {
      insert_configpanel_tab (i);
      start_img_update (&panels[i]);
    }
  }

  numpanels = newnumpanels;
}

static void kkam_create_plugin (GtkWidget *vbox, gint first_create)
{
  int i;

  kkam_vbox = vbox;
  
  if (first_create)
  {
    change_num_panels ();
    created = 1;

    for (i = 0; i < MAX_NUMPANELS; i++)
      panels[i].panel = gkrellm_panel_new0 ();

    for (i = 0; i < numpanels; i++)
      start_img_update (&panels[i]);

    if (viewer_prog == NULL)
      viewer_prog = g_strdup (default_viewer);
  }

  img_style = gkrellm_meter_style (style_id);

  for (i = 0; i < MAX_NUMPANELS; i++)
  {
    panels[i].panel->textstyle = gkrellm_meter_textstyle (style_id);
    panels[i].panel->label->h_panel = panels[i].height;
    gkrellm_create_panel (vbox, panels[i].panel,
                          gkrellm_bg_meter_image (style_id));
    gkrellm_monitor_height_adjust (panels[i].panel->h);
    panels[i].visible = TRUE;
    if (i >= numpanels)
    {
      gkrellm_enable_visibility (FALSE, &(panels[i].visible),
                                 panels[i].panel->drawing_area,
                                 panels[i].panel->h);
    }
  }

  if (first_create)
  {
    for (i = 0; i < MAX_NUMPANELS; i++)
    {
      gtk_signal_connect (GTK_OBJECT (panels[i].panel->drawing_area),
            "expose_event", (GtkSignalFunc) panel_expose_event,
            GINT_TO_POINTER (i));
      gtk_signal_connect (GTK_OBJECT (panels[i].panel->drawing_area),
            "button_press_event", (GtkSignalFunc) show_curimage,
            GINT_TO_POINTER (i));
            
      gkrellm_draw_layers (panels[i].panel);
    }
  }
  else
  {
    for (i = 0; i < numpanels; i++)
      if (panels[i].decal && panels[i].decal->pixmap)
      {
        gkrellm_draw_decal_pixmap (panels[i].panel, panels[i].decal, 0);
        gkrellm_draw_layers (panels[i].panel);
      }
  }
}

static gchar *update_script_config (char *oldval)
{
  gchar *chopmeup, *ret;
  char *firstword, *rest;

  chopmeup = g_strdup_printf ("%s\n \n", g_strstrip (oldval));
  firstword = strtok (chopmeup, " \n");
  if (!firstword)
    return NULL;
  rest = strtok (NULL, "\n");
  if (!rest)
    return NULL;
  g_strstrip (rest);
  
  /* If the old update_script item was using krellkam_load, just keep its
     parameters. If it was using a different script, put it with the
     appropriate option into the krellkam_load parameters (-x) */

  if (!strcmp (basename (firstword), WGET_SCRIPT))
    ret = g_strdup (rest);
  else
    ret = g_strdup_printf ("-x %s", oldval);
  g_free (chopmeup);
  return ret;
}

static void kkam_save_config (FILE *f)
{
  int i;

  fprintf (f, "%s viewer_prog %s\n", PLUGIN_KEYWORD, viewer_prog);
  fprintf (f, "%s numpanels %d\n", PLUGIN_KEYWORD, numpanels);

  for (i = 0; i < MAX_NUMPANELS; i++)
  {
    fprintf (f, "%s %d source %s\n",
             PLUGIN_KEYWORD, i + 1, panels[i].source);
    fprintf (f, "%s %d img_height %d\n",
             PLUGIN_KEYWORD, i + 1, panels[i].height);
    fprintf (f, "%s %d update_period %d\n",
             PLUGIN_KEYWORD, i + 1, panels[i].period);
    fprintf (f, "%s %d boundary %d\n",
             PLUGIN_KEYWORD, i + 1, panels[i].boundary);
    fprintf (f, "%s %d maintain_aspect %d\n",
             PLUGIN_KEYWORD, i + 1, panels[i].maintain_aspect);
  }
}

static void kkam_load_config (gchar *arg)
{
  gchar *config_item, *value;
  int which;

  config_item = strtok (arg, " \n");
  if (!config_item)
    return;
  which = atoi (config_item);
  if (which)
  {
    config_item = strtok (NULL, " \n");
    if (!config_item)
      return;
    which--;
  }
  value = strtok (NULL, "\n");
  if (value == NULL)
    value = "";

  if (!strcmp (config_item, "img_height"))
  {
    if (validnum (which))
      panels[which].height = CLAMP (atoi (value), 10, 100);
  }
  else if (!strcmp (config_item, "update_period"))
  {
    if (validnum (which))
      panels[which].period = MAX (atoi (value), 1);
  }
  else if (!strcmp (config_item, "source"))
  {
    if (validnum (which))
    {
      if (panels[which].source)
        g_free (panels[which].source);
      panels[which].source = g_strstrip (g_strdup (value));
    }
  }
  else if (!strcmp (config_item, "update_script"))
  {
    /* this is for backwards compatibility- update old config item */
    if (validnum (which))
    {
      if (panels[which].source)
        g_free (panels[which].source);
      panels[which].source = update_script_config (value);
    }
  }
  else if (!strcmp (config_item, "maintain_aspect"))
  {
    if (validnum (which))
      panels[which].maintain_aspect = (gboolean)CLAMP (atoi (value), 0, 1);
  }
  else if (!strcmp (config_item, "boundary"))
  {
    if (validnum (which))
      panels[which].boundary = CLAMP (atoi (value), 0, 20);
  }
  else if (!strcmp (config_item, "viewer_prog"))
  {
    if (viewer_prog)
      g_free (viewer_prog);
    viewer_prog = g_strdup (value);
  }
  else if (!strcmp (config_item, "numpanels"))
  {
    newnumpanels = CLAMP (atoi (value), MIN_NUMPANELS, MAX_NUMPANELS);
    change_num_panels ();
  }
}

static void cb_numpanel_spinner ()
{
  newnumpanels = gtk_spin_button_get_value_as_int
                                       (GTK_SPIN_BUTTON (numpanel_spinner));
  change_num_panels ();
}

static void kkam_create_tab (GtkWidget *tab_vbox)
{
  GtkWidget *vbox, *tablabel;
  GtkWidget *scrolled, *text;
  GtkWidget *viewerlabel, *viewerhbox;
  GtkWidget *configpanel;
  GtkWidget *about;
  gchar *tabname;
  int i;

  if (tabs)
    gtk_object_unref (GTK_OBJECT (tabs));
  
  tabs = gtk_notebook_new ();  
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (tabs), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (tab_vbox), tabs, TRUE, TRUE, 0);
  gtk_object_ref (GTK_OBJECT (tabs));

  /* main options tab */
  vbox = gkrellm_create_tab (tabs, _("Options"));
  
  viewerhbox = gtk_hbox_new (FALSE, 0);
  viewerlabel = gtk_label_new (_("Path to image viewer program:  "));
  viewerbox = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (viewerbox), viewer_prog);
  gtk_entry_set_editable (GTK_ENTRY (viewerbox), TRUE);

  gtk_box_pack_start (GTK_BOX (viewerhbox), viewerlabel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (viewerhbox), viewerbox, FALSE, FALSE, 0);
  
  gtk_box_pack_start (GTK_BOX (vbox), viewerhbox, TRUE, FALSE, 0);
  
  gkrellm_spin_button (vbox, &numpanel_spinner,
                       (gfloat) numpanels,
                       (gfloat) MIN_NUMPANELS,
                       (gfloat) MAX_NUMPANELS,
                       1.0, 1.0, 0, 0, cb_numpanel_spinner, NULL,
                       FALSE, _("Number of panels"));
  
  /* individual panel options tabs */
  for (i = 0; i < MAX_NUMPANELS; i++)
  {
    configpanel = create_configpanel_tab (i);

    tabname = g_strdup_printf (_("Panel #%d"), i + 1);
    tablabel = gtk_label_new (tabname);
    g_free (tabname);
    
    if (i < numpanels)
      gtk_notebook_append_page (GTK_NOTEBOOK (tabs), configpanel, tablabel);
  }

  /* Info tab */
  vbox = gkrellm_create_tab (tabs, _("Info"));
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
  text = gtk_text_new (NULL, NULL);
  gkrellm_add_info_text (text, kkam_info_text,
                          sizeof (kkam_info_text) / sizeof (gchar *));
  gtk_text_set_editable (GTK_TEXT (text), FALSE);
  gtk_container_add (GTK_CONTAINER (scrolled), text);

  /* About tab */
  vbox = gkrellm_create_tab (tabs, _("About"));
  about = gtk_label_new (kkam_about_text);
  gtk_box_pack_start (GTK_BOX (vbox), about, TRUE, TRUE, 0);
}

static void kkam_apply_config ()
{
  int i;

  for (i = 0; i < numpanels; i++)
  {
    if (panels[i].source)
      g_free (panels[i].source);
    panels[i].source = gtk_editable_get_chars
                               (GTK_EDITABLE (panels[i].sourcebox), 0, -1);
    panels[i].period = gtk_spin_button_get_value_as_int
                               (GTK_SPIN_BUTTON (panels[i].period_spinner));
    panels[i].maintain_aspect = GTK_TOGGLE_BUTTON (panels[i].aspect_box)->active;
    panels[i].boundary = gtk_spin_button_get_value_as_int
                               (GTK_SPIN_BUTTON (panels[i].boundary_spinner));
  }

  newnumpanels = gtk_spin_button_get_value_as_int
                                 (GTK_SPIN_BUTTON (numpanel_spinner));
  
  change_num_panels ();

  if (viewer_prog)
    g_free (viewer_prog);
  viewer_prog = g_strdup
                   (gtk_editable_get_chars (GTK_EDITABLE (viewerbox), 0, -1));
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
  int i;
  
  style_id = gkrellm_add_meter_style (&kam_mon, PLUGIN_STYLE);
  panels = g_new0 (KKamPanel, MAX_NUMPANELS);
  
  for (i = 0; i < MAX_NUMPANELS; i++)
  {  
    panels[i].source = g_strdup (default_source[i]);
    panels[i].height = 50;
    panels[i].period = 1;
    panels[i].boundary = 0;
  }

  return &kam_mon;
}

