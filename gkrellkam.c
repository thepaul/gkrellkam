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

#include <gkrellm.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>

/*
  Determine gkrellm version- if >= 1.2.0, we can add some extra
  features. Yippee!
*/
#if ((GKRELLM_VERSION_MAJOR == 1) && (GKRELLM_VERSION_MINOR >= 2))
# define GKRELLM_1_2_0
# define PLUGIN_VER "0.3.0b/s2"
#else
# define PLUGIN_VER "0.3.0b/s1"
#endif

#define PLUGIN_NAME "GKrellKam"
#define PLUGIN_DESC "GKrellM Image Watcher plugin"
#define PLUGIN_URL "http://gkrellkam.sourceforge.net/"
#define PLUGIN_STYLE PLUGIN_NAME
#define PLUGIN_KEYWORD PLUGIN_NAME

#define TEMPTEMPLATE "/tmp/krellkam"

#define DEBUGGING 0

static gchar *kkam_info_text[] = 
{
"<b>" PLUGIN_NAME " " PLUGIN_VER "\n\n",

"GKrellKam is a plugin that can watch a number of image files,\n",
"and display them sized to fit in panels on your gkrellm.\n\n",

"You can ", "<b>left-click", " on an image panel to display the\n",
"original, unsized image, or you can ", "<b>middle-click", " on it\n",
"to get an immediate update.\n\n",

"Scrolling the mouse wheel up over any of the panels increases\n",
"the number of visible panels, and scrolling it down decreases\n",
"the number. This is so you can hide panels you don't always want\n",
"open."

#ifdef GKRELLM_1_2_0
" Your GKrellKam was compiled for the new GKrellM 1.2.x, so\n",
"you can also ", "<b>right-click", " on a panel to open the configuration\n",
"window.",
#endif

"\n\n",
"<b>-- EASY START --\n\n",

"Just put the address of a webcam image in the Image Source\n",
"box on the configuration tab for one of the panels. The\n",
"address will probably start with \"http://\" and end in \".jpg\",\n",
"\".png\", or \".gif\".\n\n",

"<b>-- CONFIGURATION TABS --\n\n",

"<b>Global Options\n\n",

"<i>Path to image viewer program\n",
"When you left-click on a GKrellKam panel, it will start your\n",
"favorite image viewer and display the original, unsized image.\n",
"Put the name of the image viewer program in this box.\n\n",

"<i>Popup errors\n",
"When something goes wrong with an image download or the parsing\n",
"of a list, GKrellKam lets you know. When Popup errors is checked,\n",
"the message will appear as a popup window with an \"OK\" button.\n",
"When you turn this option off, error messages will be reported\n",
"in the tooltip for the image in question. They are not very\n",
"visible this way, but perhaps they are less annoying too.\n\n",

"<i>Number of panels\n",
"This lets you adjust the number of visible GKrellKam panels\n",
"between 0 and 5.\n\n",

"<b>Panel Config Tabs\n\n",

"<i>Default number of seconds per update\n",
"After an image has been successfully loaded, GKrellKam will\n",
"wait this many seconds before updating the image. Updating\n",
"might involve reloading the same image, or getting the next\n",
"image in a list. If you middle-click on a panel, it will do\n",
"the update immediately. This setting can be overridden by\n",
"specific items in lists.\n\n",

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

"<i>Select list images at random\n",
"When this is checked, and your image source is a list, then\n",
"images will be taken from the list at random rather than\n",
"cycled through one by one.\n\n",

"<i>Image Source\n",
"Each panel has a \"source\" associated with it. A source can be\n",
"a local picture, the web address of a picture, a list of\n",
"pictures, or a script to call to get a new picture. To give a\n",
"local picture file, list of files, or a script, enter the\n",
"_full_ filename in the \"Image Source\" box. To watch a webcam\n",
"or other online picture, or use an online list, just put its\n",
"address (beginning with http:// or ftp://) in the \"Image Source\n",
"box. Lists should end in \"-list\" or \".list\". You'll need GNU\n",
"wget installed to be able to get files from the internet.\n",
"Special case: when this field begins with \"-x\" followed by a\n",
"space and some more text, the remaining text is assumed to be a\n",
"script or other system commmand, and the whole path does not\n",
"need to be specified.\n\n",

"<i>Reread source button\n",
"When your image source is a list, GKrellKam will only read in\n",
"that list once. If it is changed, and you want GKrellKam to\n",
"follow the changes, you can hit the \"Reread source\" button.\n",
"This will restart the list. If the source is a script, it will\n",
"be re-executed. Other sources will be reread as well.\n\n",

"See the included example list, the ",
"<b>gkrellkam-list(5)",
" manpage,\nand ",
"<b>" PLUGIN_URL,
" online for more info."
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

static const char *default_source[] = {
  "http://www.usu.edu/webcam/fullsize.jpg",
  "",
  "",
  "",
  ""
};

#define wget_opts "--proxy=off --cache=off"
#define BUFLEN 256
#define MIN_NUMPANELS 0
#define MAX_NUMPANELS 5
#define MAX_DEPTH 64
#define MAX_SECONDS 604800 /* one week */

typedef enum {
  SOURCE_URL,
  SOURCE_FILE,
  SOURCE_SCRIPT,
  SOURCE_LIST,
  SOURCE_LISTURL
} SourceEnum;

/* source information structure- panels each have a GList of these */

typedef struct
{
  gchar *img_name;
  gchar *tooltip;
  SourceEnum type;
  int seconds;
  int next_dl;
  gchar *tfile;
  int tlife;
} KKamSource;

/* items that each panel needs */

typedef struct
{
  Panel *panel;
  Decal *decal;
  GdkPixmap *pixmap;
  FILE *cmd_pipe;
  int count;
  int height;
  int boundary;
  int default_period;
  gboolean maintain_aspect;
  gboolean random;
  gint visible;

  GtkWidget *period_spinner;
  GtkWidget *boundary_spinner;
  GtkWidget *height_spinner;
  GtkWidget *aspect_box;
  GtkWidget *random_box;
  GtkWidget *sourcebox;
  GdkImlibImage *imlibim;

  FILE *listurl_pipe;
  gchar *listurl_file;

  gchar *source;
  GList *sources;
} KKamPanel;

static Monitor *monitor;

static int created = 0;
static int numpanels = 0;
static int newnumpanels = 1;
static KKamPanel *panels = NULL;

static Style *img_style = NULL;
static gint style_id;

static char *viewer_prog = NULL;
static int popup_errors = 0;

static GtkWidget *viewerbox;
static GtkWidget *numpanel_spinner = NULL;
static GtkWidget *popup_errors_box = NULL;
static GtkWidget *tabs = NULL;
static GtkWidget *kkam_vbox = NULL;
static GtkWidget *filebox = NULL;

static GtkTooltips *tooltipobj;

static void change_num_panels ();
static void create_sources_list (KKamPanel *p);
static void kkam_read_list (KKamPanel *p, char *listname, int depth);

static KKamSource empty_source = { "", "No config",
                                   SOURCE_FILE,
                                   0, 0, NULL, 0 };

static char *IMG_EXTENSIONS[] = {
  ".jpg",
  ".jpeg",
  ".png",
  ".bmp",
  ".xpm",
  ".pbm",
  ".ppm",
  ".tif",
  ".tiff",
  ".gif"
};

/*
  report_error ()

  if popup errors are on, brings up a message window.
  if not, sets the tooltip text to a warning.
*/
static void report_error (KKamPanel *p, char *fmt, ...)
{
  va_list ap;
  char *str;
 
  va_start (ap, fmt);
  str = g_strdup_vprintf (fmt, ap);
  va_end (ap);

  if (popup_errors)
  {
    GtkWidget *label, *button, *vbox, *dialog;

    dialog = gtk_window_new (GTK_WINDOW_DIALOG);
    vbox = gtk_vbox_new (FALSE, 0);
  
    label = gtk_label_new (_("GKrellKam warning:"));
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    label = gtk_label_new (str);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

    button = gtk_button_new_with_label (_("  OK  "));  
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
    gtk_container_add (GTK_CONTAINER (dialog), vbox);
  
    gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                               GTK_SIGNAL_FUNC (gtk_widget_destroy),
                               (gpointer)dialog);
  
    gtk_container_set_border_width (GTK_CONTAINER (dialog), 15);
    gtk_widget_show_all (dialog);
  }
  else
  {
    if (p && tooltipobj && p->panel && p->panel->drawing_area)
      gtk_tooltips_set_tip (tooltipobj, p->panel->drawing_area, str, NULL);
  }
}

/*
  destroy_viewer ()

  The delete_event callback for the internal viewer
*/
static gint destroy_viewer (GtkWidget *window, GdkEvent *ev, gpointer n)
{
  gtk_widget_destroy (window);
  return FALSE;
}

/*
  kkam_internal_viewer ()

  Opens a very simple full-size image viewer window for the image in
  question. Users can specify an external viewer if they don't like it :)
*/
static void kkam_internal_viewer (char *filename)
{
  GtkWidget *window, *pmap;
  GdkPixmap *pix;
  GdkBitmap *bit;

  if (gdk_imlib_load_file_to_pixmap (filename, &pix, &bit) == 0)
    return;
  
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), filename);
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
                      GTK_SIGNAL_FUNC (destroy_viewer), NULL);

  pmap = gtk_pixmap_new (pix, bit);
  gtk_container_add (GTK_CONTAINER (window), pmap);
  gtk_widget_show_all (window);
  gdk_imlib_free_pixmap (pix);
  gdk_imlib_free_pixmap (bit);
}

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
  panel_cursource ()

  returns a pointer to the source definition of the current image for
  the current panel, or &empty_source if none.
*/
static KKamSource *panel_cursource (KKamPanel *p)
{
  if (p->sources == NULL)
    return &empty_source;
  return (KKamSource *)(p->sources->data);
}

/*
  get_period ()

  returns the time that should elapse before the current image is
  changed. If the current source's seconds member is 0, use the
  default_period.
*/
static int get_period (KKamPanel *p)
{
  int per;
  
  if ((per = panel_cursource (p)->seconds) == 0)
    return p->default_period;
  return per;
}

/*
  tfile_release ()

  If a kkamsource has been loaded before, and a copy of the image
  is still attached to it, then unattach that copy. If it is a
  temporary file, then unlink it as well.
*/
static void tfile_release (KKamSource *ks)
{
  if (ks == NULL || ks->tfile == NULL)
    return;

  if (ks->type == SOURCE_URL)
    unlink (ks->tfile);
  g_free (ks->tfile);
  ks->tfile = NULL;
  ks->next_dl = 0;
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

#ifndef GKRELLM_1_2_0
  /* need to blank out the old image here for the old gkrellm. */
  gkrellm_render_to_pixmap (gkrellm_bg_meter_image (style_id),
                            &(p->pixmap), NULL,
                            gkrellm_chart_width (), p->height);
  gkrellm_destroy_decal_list (p->panel);
  p->decal = gkrellm_create_decal_pixmap (p->panel, p->pixmap,
                                          NULL, 1, img_style, 0, 0);
  gkrellm_draw_decal_pixmap (p->panel, p->decal, 0);
  gkrellm_draw_layers (p->panel);
#endif
  
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
  
#ifdef GKRELLM_1_2_0
  gkrellm_remove_and_destroy_decal (p->panel, p->decal);
  gkrellm_render_to_pixmap (p->imlibim, &(p->pixmap), NULL,
                            scale_x, scale_y);
#else
  gkrellm_render_to_pixmap (p->imlibim, &(p->pixmap), NULL,
                            scale_x, scale_y);
  gkrellm_destroy_decal_list (p->panel);
#endif

  p->decal = gkrellm_create_decal_pixmap (p->panel, p->pixmap,
                                          NULL, 1, img_style, loc_x, loc_y);
  gkrellm_draw_decal_pixmap (p->panel, p->decal, 0);
  gkrellm_draw_layers (p->panel);
}

/*
  start_img_dl ()

  Open a pipe and spawn wget.
*/
static void start_img_dl (KKamPanel *p)
{
  gchar *wget_str;
  char tmpfile[] = TEMPTEMPLATE "XXXXXX";
  int tmpfd;

  if (p->cmd_pipe) /* already open */
    return;

  tmpfd = mkstemp (tmpfile); /* this will create the file, perm 0600 */
  if (tmpfd == -1)
  {
    report_error (p, _("Couldn't create temporary file for download: %s"),
                  strerror (errno));
    return;
  }
  close (tmpfd);

  wget_str = g_strdup_printf ("wget -q %s -O %s \"%s\"",
                              wget_opts, tmpfile,
                              panel_cursource (p)->img_name);

  p->cmd_pipe = popen (wget_str, "r");
  g_free (wget_str);
  if (p->cmd_pipe == NULL)
  {
    unlink (tmpfile);
    report_error (p, _("Couldn't start wget: %s"), strerror (errno));
    return;
  }
  
  panel_cursource (p)->tfile = g_strdup (tmpfile);
  fcntl (fileno (p->cmd_pipe), F_SETFL, O_NONBLOCK);
}

/*
  start_script_dl ()

  open a pipe for a user-defined script
*/
static void start_script_dl (KKamPanel *p)
{
  char *scriptname;

  if (p->cmd_pipe) /* already open */
    return;

  scriptname = panel_cursource (p)->img_name;
  if (!strncmp (scriptname, "-x", 2))
    scriptname += 2;

  p->cmd_pipe = popen (scriptname, "r");
  if (p->cmd_pipe == NULL)
  {
    report_error (p, _("Couldn't start script \"%s\": %s\n"),
                  panel_cursource (p)->img_name, strerror (errno));
    return;
  }
  fcntl (fileno (p->cmd_pipe), F_SETFL, O_NONBLOCK);
}

/*
  load_image_file ()

  If the file in p's cursource exists, loads it into p's imlibim
  member. Calls draw_imlibim. Also, sets image tooltip.

  Returns -1 on file missing, and 1 otherwise (not necessarily
    success, but probably)
*/
static int load_image_file (KKamPanel *p)
{
  struct stat img_st;
  KKamSource *ks;

  ks = panel_cursource (p);

  /* make sure file is really there. when the imlib stuff
     fails it's really loud */
  if (ks->tfile == NULL || stat (ks->tfile, &img_st) == -1)
  {
    ks->next_dl = 0;
    return -1;
  }
  
  if (p->imlibim)    
    gdk_imlib_kill_image (p->imlibim);
  p->imlibim = gdk_imlib_load_image (ks->tfile);
  draw_imlibim (p);  

  if (ks->tooltip)
    gtk_tooltips_set_tip (tooltipobj, p->panel->drawing_area,
                          ks->tooltip, NULL);
  else
    gtk_tooltips_set_tip (tooltipobj, p->panel->drawing_area,
                          ks->img_name, NULL);

  return 1;
}

/*
  cmd_results ()

  Checks if the command pipe is dead yet. If so, checks its output
  status and returns 1 if the command was successful.

  On error, returns -1. On not yet ready, returns 0.
*/
static int cmd_results (KKamPanel *p)
{
  int code, len;
  char buf[BUFLEN];
  KKamSource *ks;

  ks = panel_cursource (p);

  if (fread (buf, sizeof (char), 1, p->cmd_pipe) < 1)
  {
    /* if we get EAGAIN, wait some more */
    if (ferror (p->cmd_pipe) && errno == EAGAIN)
      return 0;

    /* if we reach here the pipe is dead- the command has finished. */
    code = pclose (p->cmd_pipe);
    p->cmd_pipe = NULL;

    /* pclose will return a -1 on a wait4 error. If that happens,
       we have no way to know whether wget succeeded. Just try */
    if (ks->type == SOURCE_URL && code <= 0)
    {
      ks->next_dl = time (NULL) + ks->tlife;
      load_image_file (p);
      return 1;
    }

    report_error (p, _("Error: wget gave bad code or script died. code %d"),
                  code);
  }
  
  len = fread (&buf[1], sizeof (char), BUFLEN - 2, p->cmd_pipe);
  buf[len + 1] = '\0';
  g_strstrip (buf);
  
  if (ks->type == SOURCE_SCRIPT)
  {
    ks->tfile = g_strdup (buf);
    ks->next_dl = time (NULL) + ks->tlife;
    load_image_file (p);
    return 1;
  }
  else
  {
    /* if we get here with wget, then wget said something. This is generally
       not good, since we passed -q. We'll have to wait for it to die */
    
    report_error (p, _("wget said: \"%s\""), buf);
    pclose (p->cmd_pipe);
    p->cmd_pipe = NULL;
    return -1;
  }
}

/*
  rotate_sources ()

  moves the current source item to the end of the list, unless
  p is set for random.
*/
static void rotate_sources (KKamPanel *p)
{
  GList *link;
  int times, i, len;

  if (p == NULL || p->sources == NULL ||
        (len = g_list_length (p->sources)) == 1)
    return;

  times = p->random ? (rand () % (len - 1) + 1) : 1;
  for (i = 0; i < times; i++)
  {
    link = p->sources;
    p->sources = g_list_remove_link (p->sources, link);
    p->sources = g_list_concat (p->sources, link);
  }
}

/*
  update_image ()

  determines what the current image source is, and updates or
  starts an update as appropriate
*/
static void update_image (KKamPanel *p)
{
  KKamSource *ks;
  
  p->count = get_period (p);

  ks = panel_cursource (p);
  if (ks->img_name == NULL || ks->img_name[0] == '\0')
    return;
 
  if (ks->next_dl > time (NULL))
    load_image_file (p);
  else
  {
    tfile_release (ks);

    switch (ks->type)
    {
    case SOURCE_SCRIPT:
      start_script_dl (p);
      break;
    case SOURCE_URL:
      start_img_dl (p);
      break;
    case SOURCE_FILE:
      ks->tfile = g_strdup (ks->img_name);
      ks->next_dl = 0; /* next_dl is meaningless for SOURCE_FILE */
      load_image_file (p);
      break;
    default:
      report_error (p, _("Invalid type %d found in sources list!"), ks->type);
    }
  }
}

/*
  listurl_results ()

  when the listurl download is finished, reads the list and deletes it.
  Returns 0 if still waiting, 1 on success or error.
*/
static int listurl_results (KKamPanel *p)
{
  int code;
  char c;
  KKamSource *ks;

  ks = panel_cursource (p);

  if (fread (&c, sizeof (char), 1, p->listurl_pipe) < 1)
  {
    /* if we get EAGAIN, wait some more */
    if (ferror (p->listurl_pipe) && errno == EAGAIN)
      return 0;

    /* if we reach here the pipe is dead- the command has finished. */
    code = pclose (p->listurl_pipe);
    p->listurl_pipe = NULL;
  }
  else
    code = 256;

  /* pclose will return a -1 on a wait4 error. If that happens,
     we have no way to know whether wget succeeded. Just try */
  if (code <= 0)
  {
    kkam_read_list (p, p->listurl_file, 0);
    update_image (p);
  }
  else
    report_error (p, _("Error: wget listurl download died. code %d"), code);

  unlink (p->listurl_file);
  g_free (p->listurl_file);
  p->listurl_file = NULL;

  return 1;
}

/*
  kkam_update_plugin ()

  callback from gkrellm. Counts seconds until next update, and when
  count is reached, starts the update process.
*/
static void kkam_update_plugin ()
{
  int i;

  if (GK.second_tick)
    for (i = 0; i < numpanels; i++)
    {
      if (panels[i].listurl_pipe)
        listurl_results (&panels[i]);
      else if (panels[i].cmd_pipe)
        cmd_results (&panels[i]);
      else if (--panels[i].count <= 0)
      {
        rotate_sources (&panels[i]);
        update_image (&panels[i]);
      }
    }
}

void showsource (KKamSource *s)
{
  fprintf (stderr, "name %s, type %d, seconds %d, tooltip %s\n",
                   s->img_name, s->type, s->seconds, s->tooltip);
}

/*
  click_callback ()

  launches eeyes, or whatever viewer the user has configured, to
  display the unscaled version of the image (looking where
  we loaded it). If the user right-clicked, get a fresh image.
*/
static gint click_callback (GtkWidget *widget, GdkEventButton *ev, gpointer gw)
{
  gchar *cmd;
  int which;
  KKamSource *ks;
 
  which = GPOINTER_TO_INT (gw);
  if (!activenum (which))
    return FALSE;
  
  ks = panel_cursource (&panels[which]);

  switch (ev->button)
  {
  case 1: /* view image */
    if (ks->tfile)
    {
      if (viewer_prog == NULL || viewer_prog[0] == '\0')
        kkam_internal_viewer (ks->tfile);
      else
      {
        cmd = g_strdup_printf ("%s '%s' &", viewer_prog, ks->tfile);
        system (cmd);
        g_free (cmd);
      }
    }
    break;
  case 2: /* immediate update */
    panels[which].count = 0;
    ks->next_dl = 0;
    break;

#ifdef GKRELLM_1_2_0
  case 3:
    gkrellm_open_config_window (monitor);
    break;
#endif

  case 4:
    newnumpanels = MIN (numpanels + 1, MAX_NUMPANELS);
    change_num_panels ();
    break;
  case 5:
    newnumpanels = MAX (numpanels - 1, MIN_NUMPANELS);
    change_num_panels ();
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

static void cb_aspect_box (KKamPanel *p)
{
  p->maintain_aspect = GTK_TOGGLE_BUTTON (p->aspect_box)->active;
  gkrellm_config_modified ();
  draw_imlibim (p);
}

static void cb_boundary_spinner (gpointer w, KKamPanel *p)
{
  p->boundary = gtk_spin_button_get_value_as_int
                                 (GTK_SPIN_BUTTON (p->boundary_spinner));
  gkrellm_config_modified ();
  draw_imlibim (p);
}

static void cb_height_spinner (gpointer w, KKamPanel *p)
{
  int newheight;

  newheight = gtk_spin_button_get_value_as_int (
                                   GTK_SPIN_BUTTON (p->height_spinner));
  
  if (newheight != p->height)
  {
#ifdef GKRELLM_1_2_0
    gkrellm_panel_configure_add_height (p->panel, newheight - p->height);
    p->height = newheight;
    gkrellm_panel_create (kkam_vbox, monitor, p->panel);
#else
    gkrellm_monitor_height_adjust (newheight - p->height);
    p->panel->label->h_panel = newheight;
    p->height = newheight;
    gkrellm_create_panel (kkam_vbox, p->panel,
                          gkrellm_bg_meter_image (style_id));
    gkrellm_pack_side_frames ();
#endif

    gkrellm_config_modified ();                            
    draw_imlibim (p);
  }
}

/*
  src_set ()

  sets the image source from the file selection dialog
*/
static void src_set (KKamPanel *p)
{
  g_free (p->source);
  p->source = g_strdup (gtk_file_selection_get_filename (
                                            GTK_FILE_SELECTION (filebox)));
  gkrellm_config_modified ();
  gtk_entry_set_text (GTK_ENTRY (p->sourcebox), p->source);
  gtk_widget_destroy (GTK_WIDGET (filebox));
  create_sources_list (p);
  p->count = get_period (p);
  update_image (p);
}

/*
  srcbrowse ()

  brings up a file browser window to select a source
*/
static void srcbrowse (KKamPanel *p)
{
  filebox = gtk_file_selection_new ("Select Image Source");
  gtk_signal_connect_object (
                     GTK_OBJECT (GTK_FILE_SELECTION (filebox)->ok_button),
                     "clicked", GTK_SIGNAL_FUNC (src_set), (gpointer)p);
  gtk_signal_connect_object (
                     GTK_OBJECT (GTK_FILE_SELECTION (filebox)->cancel_button),
                     "clicked",
                     GTK_SIGNAL_FUNC (gtk_widget_destroy),
                     (gpointer)filebox);

  gtk_widget_show (filebox);
}

/*
  srcreread ()

  rereads the source of a panel- this is useful if, for example,
  one changes a list and wants the new list used
*/
static void srcreread (KKamPanel *p)
{
  g_free (p->source);
  p->source = gtk_editable_get_chars (GTK_EDITABLE (p->sourcebox), 0, -1);
  create_sources_list (p);
  p->count = get_period (p);
  update_image (p);
}

/*
  create_configpanel_tab ()

  Creates a GtkVbox for the configuration of one of the panels
*/
static GtkWidget *create_configpanel_tab (int i)
{
  GtkWidget *vbox, *hbox, *button, *sourcelabel;

  vbox = gtk_vbox_new (FALSE, 0);

  gkrellm_spin_button (vbox, &panels[i].period_spinner,
                       (gfloat) panels[i].default_period,
                       1.0, (gfloat)MAX_SECONDS, 1.0, 10.0, 0, 0, NULL, NULL,
                       FALSE, _("Default number of seconds per update"));

  gkrellm_spin_button (vbox, &panels[i].height_spinner,
                       (gfloat) panels[i].height,
                       10.0, 100.0, 1.0, 5.0, 0, 0, cb_height_spinner, &panels[i],
                       FALSE, _("Height of viewer, in pixels"));

  hbox = gtk_hbox_new (FALSE, 0);
  gkrellm_spin_button (hbox, &panels[i].boundary_spinner,
                       (gfloat) panels[i].boundary,
                       0.0, 20.0, 1.0, 1.0, 0, 0, cb_boundary_spinner, &panels[i],
                       FALSE, _("Border size"));
  
  gkrellm_check_button (hbox, &panels[i].aspect_box,
                        panels[i].maintain_aspect,
                        TRUE, 0, _("Maintain aspect ratio"));
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  
  gtk_signal_connect_object (GTK_OBJECT (panels[i].aspect_box), "toggled",
                        GTK_SIGNAL_FUNC (cb_aspect_box), (gpointer)&panels[i]);

  gkrellm_check_button (vbox, &panels[i].random_box,
                        panels[i].random,
                        TRUE, 0, _("Select list images at random"));

  hbox = gtk_hbox_new (FALSE, 0);
  sourcelabel = gtk_label_new (_("Image source:  "));
  panels[i].sourcebox = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (panels[i].sourcebox), panels[i].source);
  gtk_entry_set_editable (GTK_ENTRY (panels[i].sourcebox), TRUE);
  button = gtk_button_new_with_label (_("Browse.."));
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                             GTK_SIGNAL_FUNC (srcbrowse), (gpointer)&panels[i]);
  gtk_box_pack_start (GTK_BOX (hbox), sourcelabel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), panels[i].sourcebox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 5);
  button = gtk_button_new_with_label (_("Reread source"));
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                             GTK_SIGNAL_FUNC (srcreread), (gpointer)&panels[i]);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  gtk_widget_show_all (vbox);

  return vbox;
}

/*
  insert_configpanel_tab ()

  Creates a new configuration tab and inserts it into the config box
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
  if (!GTK_IS_OBJECT (tabs))
    return;

  gtk_notebook_remove_page (GTK_NOTEBOOK (tabs), i + 1);
}

/*
  change_num_panels ()

  called to move the number in `newnumpanels' into
  `numpanels' with all associated changes to the monitor
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
#ifdef GKRELLM_1_2_0
      gkrellm_panel_enable_visibility (panels[i].panel, i < newnumpanels,
                                       &(panels[i].visible));
#else
      gkrellm_enable_visibility (i < newnumpanels, &(panels[i].visible),
                                 panels[i].panel->drawing_area,
                                 panels[i].panel->h);
#endif
    }

    for (i = numpanels; i < newnumpanels; i++)
    {
      insert_configpanel_tab (i);
      update_image (&panels[i]);
    }
  }

  numpanels = newnumpanels;
  gkrellm_config_modified ();
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

    tooltipobj = gtk_tooltips_new ();
    gtk_tooltips_set_delay (tooltipobj, 1000);

    srand (time (NULL)); /* randomize from timer */
  }

  img_style = gkrellm_meter_style (style_id);

  for (i = 0; i < MAX_NUMPANELS; i++)
  {
#ifdef GKRELLM_1_2_0
    gkrellm_panel_configure_add_height (panels[i].panel, panels[i].height);
    gkrellm_panel_create (vbox, monitor, panels[i].panel);
    gkrellm_panel_keep_lists (panels[i].panel, TRUE);
#else
    panels[i].panel->textstyle = gkrellm_meter_textstyle (style_id);
    panels[i].panel->label->h_panel = panels[i].height;
    gkrellm_create_panel (vbox, panels[i].panel,
                          gkrellm_bg_meter_image (style_id));
    gkrellm_monitor_height_adjust (panels[i].panel->h);
#endif

    panels[i].visible = TRUE;
    if (i >= numpanels)
    {
#ifdef GKRELLM_1_2_0
      gkrellm_panel_enable_visibility (panels[i].panel, FALSE,
                                       &(panels[i].visible));
#else
      gkrellm_enable_visibility (FALSE, &(panels[i].visible),
                                 panels[i].panel->drawing_area,
                                 panels[i].panel->h);
#endif
    }
  }

  if (first_create)
  {
    for (i = 0; i < MAX_NUMPANELS; i++)
    {
      gtk_signal_connect (GTK_OBJECT (panels[i].panel->drawing_area),
            "expose_event", GTK_SIGNAL_FUNC (panel_expose_event),
            GINT_TO_POINTER (i));
      gtk_signal_connect (GTK_OBJECT (panels[i].panel->drawing_area),
            "button_press_event", GTK_SIGNAL_FUNC (click_callback),
            GINT_TO_POINTER (i));
            
      gkrellm_draw_layers (panels[i].panel);
      if (i < numpanels)
        update_image (&panels[i]);
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

static KKamSource *kkam_source_new ()
{
  return g_new0 (KKamSource, 1);
}

static void kkam_source_free (KKamSource *ks)
{
  if (ks->tfile)
    tfile_release (ks);
  g_free (ks->img_name);
  g_free (ks->tooltip);
  g_free (ks);
}

static void destroy_sources_list (KKamPanel *p)
{
  g_list_foreach (p->sources, (GFunc)kkam_source_free, NULL);
  g_list_free (p->sources);
  p->sources = NULL;
}

/*
  addto_sources_list ()

  Adds a component to the sources list for panel p. The name
  and type are copied from name and type. The new component
  is returned, in case it needs to be modified further.
*/
static KKamSource *addto_sources_list (KKamPanel *p, char *name, SourceEnum type)
{
  KKamSource *ks;

  ks = kkam_source_new ();
  ks->type = type;
  ks->img_name = g_strdup (name);
  ks->tfile = NULL;
  ks->next_dl = 0;
  p->sources = g_list_append (p->sources, (gpointer)ks);

  return ks;
}

/*
  endswith ()

  returns TRUE if the end of str matches endstr
*/
static gboolean endswith (char *str, char *endstr)
{
  int len, lenend;

  if ((len = strlen (str)) < (lenend = strlen (endstr)))
    return FALSE;
  return !strcmp (&str[len - lenend], endstr);
}

/*
  source_type_of ()

  determines what type of source is represented by the given
  string, according to these rules, checked in order:

  - A source beginning with 'http:' or 'ftp:' and ending in '.list'
     is a SOURCE_LISTURL.
  - A source beginning with 'http:' or 'ftp:' is a SOURCE_URL.
  - A source that begins with '-x' is a SOURCE_SCRIPT.
  - A source with an extension in IMG_EXTENSIONS is a SOURCE_FILE.
  - A source that matches a filename that is executable by the user is
     a SOURCE_SCRIPT.
  - A source ending in [.-]list is a SOURCE_LIST.
  - If none of the above apply, and the source is a filename, the
     file is opened. If it 'looks' like a GKrellKam list configuration
     file, it is a SOURCE_LIST.
  - Otherwise, it is a SOURCE_FILE.

FIXME: spaghetti!
*/
static SourceEnum source_type_of (char *def)
{
  int i, p, len;
  FILE *test;
  gchar **words;
  unsigned char buf[BUFLEN];
  
  words = g_strsplit (def, " ", 2);
  if (!words || !words[0]) /* wish I didn't need this */
    return SOURCE_FILE;

  if (!strncmp (words[0], "http:", 5) || !strncmp (words[0], "ftp:", 4))
  {
    if (endswith (words[0], ".list") || endswith (words[0], "-list"))
    {
      g_strfreev (words);
      return SOURCE_LISTURL;
    }

    g_strfreev (words);
    return SOURCE_URL;
  }
  if (!strcmp (words[0], "-x"))
  {
    g_strfreev (words);
    return SOURCE_SCRIPT;
  }
  for (i = 0; i < (sizeof (IMG_EXTENSIONS) / sizeof (IMG_EXTENSIONS[0])); i++)
  {
    if (endswith (words[0], IMG_EXTENSIONS[i]))
    {
      g_strfreev (words);
      return SOURCE_FILE;
    }
  }
  if (access (words[0], X_OK) == 0)
  {
    g_strfreev (words);
    return SOURCE_SCRIPT;
  }
  if (endswith (words[0], ".list") || endswith (words[0], "-list"))
  {
    g_strfreev (words);
    return SOURCE_LIST;
  }
  
  if ((test = fopen (words[0], "r")) != NULL)
  {
    len = fread (buf, sizeof (buf[0]), BUFLEN, test);
    for (p = 0; p < len; p++)
      if (!isgraph (buf[p]) && !isspace (buf[p]))
      {
        fclose (test);
        g_strfreev (words);
        return SOURCE_FILE;
      }
    g_strfreev (words);
    fclose (test);
    return SOURCE_LIST;
  }

  g_strfreev (words);
  return SOURCE_FILE;
}

/*
  nextword ()

  convenience function for the list reader; returns a pointer in s
  to the beginning of the next parameter (non-space following a colon)
*/
char *nextword (char *s)
{
  char *ret;

  for (ret = s; *ret != ':'; ret++) ;
  ret++;
  for ( ; isspace (*ret); ret++) ;
  return ret;
}

/*
  kkam_read_list ()

  reads a GKrellKam list configuration file and loads its
  data into the panel's source GList
*/
static void kkam_read_list (KKamPanel *p, char *listname, int depth)
{
  KKamSource *ks = NULL;
  SourceEnum typ;
  FILE *listfile;
  char buf[BUFLEN];
  int thislist_error = 0;

  if (depth > MAX_DEPTH)
  {
    report_error (p, _("Maximum recursion depth exceeded reading list %s; "
                  "perhaps a list is trying to load itself?"), listname);
    return;
  }

  if ((listfile = fopen (listname, "r")) == NULL)
    return;
  
  while (fgets (buf, BUFLEN, listfile))
  {
    g_strchomp (buf);

    switch (buf[0])
    {
    case '\0':
    case '#':
      ks = NULL;
      break;

    case '\t':
      if (ks != NULL)
      {
        if (!strncmp (&buf[1], "tooltip:", 8))
          ks->tooltip = g_strdup (nextword (&buf[1]));
        else if (!strncmp (&buf[1], "seconds:", 8))
          ks->seconds = CLAMP (atoi (nextword (&buf[1])), 1, MAX_SECONDS);
        else if (!strncmp (&buf[1], "refresh:", 8))
          ks->tlife = CLAMP (atoi (nextword (&buf[1])), 1, MAX_SECONDS);
      }
      else if (thislist_error == 0) /* only show one error per file */
      {
        thislist_error = 1;
        report_error (p, _("In list %s, property line \"%s\" isn't associated"
                           " with any source!"), listname, &buf[1]);
      }
      break;
      
    default:
      if (!strncmp (buf, "image:", 6))
        ks = addto_sources_list (p, nextword (buf), SOURCE_FILE);
      else if (!strncmp (buf, "script:", 7))
        ks = addto_sources_list (p, nextword (buf), SOURCE_SCRIPT);
      else if (!strncmp (buf, "url:", 4))
        ks = addto_sources_list (p, nextword (buf), SOURCE_URL);
      else if (!strncmp (buf, "list:", 5))
      {
        kkam_read_list (p, nextword (buf), depth + 1);
        ks = NULL;
      }
      else
      {
        typ = source_type_of (buf);
        if (typ == SOURCE_LIST)
        {
          kkam_read_list (p, buf, depth + 1);
          ks = NULL;
        }
        else
          ks = addto_sources_list (p, buf, typ);
      }
    }
  }
}

static void kkam_read_listurl (KKamPanel *p, char *source)
{
  gchar *wget_str;
  char tmpfile[] = TEMPTEMPLATE "-urllistXXXXXX";
  int tmpfd;

  if (p->listurl_pipe) /* already open */
    return;

  tmpfd = mkstemp (tmpfile); /* this will create the file, perm 0600 */
  if (tmpfd == -1)
  {
    report_error (p, _("Couldn't create temporary file for list download: %s"),
                  strerror (errno));
    return;
  }
  close (tmpfd);

  wget_str = g_strdup_printf ("wget -q %s -O %s \"%s\"",
                              wget_opts, tmpfile, source);

  p->listurl_pipe = popen (wget_str, "r");
  g_free (wget_str);
  if (p->listurl_pipe == NULL)
  {
    unlink (tmpfile);
    report_error (p, _("Couldn't start wget for list download: %s"),
                  strerror (errno));
    return;
  }
  
  p->listurl_file = g_strdup (tmpfile);
  fcntl (fileno (p->listurl_pipe), F_SETFL, O_NONBLOCK);

  gtk_tooltips_set_tip (tooltipobj, p->panel->drawing_area,
                        _("Downloading list.."), NULL);
}

/*
  create_sources_list ()

  Takes the 'source' of a panel- which can be a web address,
  local image, script, list, etc.. and translates that into
  a GList of sources. For most source types, obviously, the
  sources GList will only have one entry.
*/
static void create_sources_list (KKamPanel *p)
{
  SourceEnum s;

  if (p->sources)
    destroy_sources_list (p);

  if (p->source && p->source[0])
  {
    switch (s = source_type_of (p->source))
    {
    case SOURCE_URL:
    case SOURCE_FILE:
    case SOURCE_SCRIPT:
      addto_sources_list (p, p->source, s);
      break;
    case SOURCE_LIST:
      kkam_read_list (p, p->source, 0);
      break;
    case SOURCE_LISTURL:
      kkam_read_listurl (p, p->source);
      break;
    }
  }
}

/*
  update_source_config ()

  Translates a 0.2.2[bcd]-style config into an appropriate source item
*/
static void update_source_config (KKamPanel *p, char *val)
{
  SourceEnum t;
  gchar **words;
  gchar *scr;
  int i;

  g_strdelimit (val, " \t\n", '\n');
  words = g_strsplit (val, "\n", 0);
  
  for (i = 0; words[i]; i++)
  {
    if (!strcmp (words[i], "-l") || !strcmp (words[i], "--list"))
    {
      g_free (words[i]);
      words[i] = g_strdup ("");
    }
    else if (!strcmp (words[i], "-x") || !strcmp (words[i], "--execute"))
    {
      g_free (words[i]);
      words[i] = g_strdup ("-x");
      scr = g_strjoinv (" ", &words[i]);
      addto_sources_list (p, scr, SOURCE_SCRIPT);
      g_free (p->source);
      p->source = scr;
      break;
    }
    else if (!strcmp (words[i], "-r") || !strcmp (words[i], "--random"))
      p->random = TRUE;
    else
    {
      t = source_type_of (words[i]);
      g_free (p->source);
      p->source = g_strdup (words[i]);
      if (t == SOURCE_LIST)
        kkam_read_list (p, words[i], 0);
      else
        addto_sources_list (p, words[i], source_type_of (val));
    }
  }
  g_strfreev (words);
}

/*
  update_script_config ()

  Translates a pre-0.2.2-style config into an appropriate source item
*/
static void update_script_config (KKamPanel *p, char *val)
{
  gchar *chopmeup;
  char *firstword, *rest;

  chopmeup = g_strdup_printf ("%s\n \n", g_strstrip (val));
  firstword = strtok (chopmeup, " \n");
  if (!firstword)
    return;
  rest = strtok (NULL, "\n");
  if (!rest)
    return;
  g_strstrip (rest);
  
  /* If the old update_script item was using krellkam_load, parse
     its parameters. If it was using a different script, put it with
     the appropriate type into the source list */

  if (!strcmp (basename (firstword), "krellkam_load"))
    update_source_config (p, rest);
  else
  {
    g_free (p->source);
    p->source = g_strdup_printf ("-x %s", val);
    addto_sources_list (p, val, SOURCE_SCRIPT);
  }
  g_free (chopmeup);
}

static void kkam_save_config (FILE *f)
{
  int i;

  if (viewer_prog && viewer_prog[0])
    fprintf (f, "%s viewer_prog %s\n", PLUGIN_KEYWORD, viewer_prog);

  fprintf (f, "%s popup_errors %d\n", PLUGIN_KEYWORD, popup_errors);
  fprintf (f, "%s numpanels %d\n", PLUGIN_KEYWORD, numpanels);

  for (i = 0; i < MAX_NUMPANELS; i++)
  {
    fprintf (f, "%s %d sourcedef %s\n",
             PLUGIN_KEYWORD, i + 1, panels[i].source);
    fprintf (f, "%s %d options %d.%d.%d.%d.%d\n",
             PLUGIN_KEYWORD, i + 1,
             panels[i].height,
             panels[i].default_period,
             panels[i].boundary,
             panels[i].maintain_aspect,
             panels[i].random);
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

  if (!strcmp (config_item, "options"))
  {
    if (validnum (which))
    {
      sscanf (value, "%d.%d.%d.%d.%d",
              &(panels[which].height),
              &(panels[which].default_period),
              &(panels[which].boundary),
              &(panels[which].maintain_aspect),
              &(panels[which].random));
      
      panels[which].height = CLAMP (panels[which].height, 10, 100);
      panels[which].default_period =
                           CLAMP (panels[which].default_period, 1, MAX_SECONDS);
      panels[which].boundary = CLAMP (panels[which].boundary, 0, 20);
      panels[which].maintain_aspect =
                          (gboolean)CLAMP (panels[which].maintain_aspect, 0, 1);
      panels[which].random = (gboolean)CLAMP (panels[which].random, 0, 1);
    }
  }
  else if (!strcmp (config_item, "sourcedef"))
  {
    if (validnum (which))
    {
      g_free (panels[which].source);
      panels[which].source = g_strstrip (g_strdup (value));
      create_sources_list (&panels[which]);
    }
  }
  else if (!strcmp (config_item, "viewer_prog"))
  {
    g_free (viewer_prog);
    viewer_prog = g_strdup (value);
  }
  else if (!strcmp (config_item, "popup_errors"))
  {
    popup_errors = atoi (value);
  }
  else if (!strcmp (config_item, "numpanels"))
  {
    newnumpanels = CLAMP (atoi (value), MIN_NUMPANELS, MAX_NUMPANELS);
    change_num_panels ();
  }
  /* backwards compatibility */
  else if (!strcmp (config_item, "img_height"))
  {
    if (validnum (which))
      panels[which].height = CLAMP (atoi (value), 10, 100);
  }
  else if (!strcmp (config_item, "period"))
  {
    if (validnum (which))
      panels[which].default_period = CLAMP (atoi (value), 1, MAX_SECONDS);
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
  else if (!strcmp (config_item, "update_period"))
  {
    /* this is in minutes */
    if (validnum (which))
      panels[which].default_period = MAX (atoi (value) * 60, 1);
  }
  else if (!strcmp (config_item, "update_script"))
  {
    /* update old config item */
    if (validnum (which))
      update_script_config (&panels[which], value);
  }
  else if (!strcmp (config_item, "source"))
  {
    /* backwards compat for 0.2.2[bcd] */
    if (validnum (which))
      update_source_config (&panels[which], value);
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
  GtkWidget *hbox, *configpanel, *about;
  GtkAdjustment *numadj;
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
  
  hbox = gtk_hbox_new (FALSE, 0);
  viewerbox = gtk_entry_new ();
  if (viewer_prog)
    gtk_entry_set_text (GTK_ENTRY (viewerbox), viewer_prog);
  gtk_entry_set_editable (GTK_ENTRY (viewerbox), TRUE);

  gtk_box_pack_start (GTK_BOX (hbox),
                      gtk_label_new (_("Path to image viewer program:")),
                      FALSE, FALSE, 10);
  gtk_box_pack_start (GTK_BOX (hbox), viewerbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);
  
  hbox = gtk_hbox_new (FALSE, 0);
  popup_errors_box = gtk_check_button_new_with_label (_("Popup errors"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (popup_errors_box),
                                popup_errors);
  gtk_box_pack_start (GTK_BOX (hbox), popup_errors_box, FALSE, FALSE, 10);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  numadj = (GtkAdjustment *) gtk_adjustment_new ((gfloat) numpanels,
                             (gfloat) MIN_NUMPANELS,
                             (gfloat) MAX_NUMPANELS,
                             1.0, 1.0, 0);
  numpanel_spinner = gtk_spin_button_new (numadj, 1.0, 0);
  gtk_signal_connect (GTK_OBJECT (numpanel_spinner), "changed",
                      GTK_SIGNAL_FUNC (cb_numpanel_spinner), NULL);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), numpanel_spinner, FALSE, FALSE, 10);
  gtk_box_pack_start (GTK_BOX (hbox),
                      gtk_label_new (_("Number of panels")),
                      FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);
  
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
  int i, diff;
  gchar *newsource;

  for (i = 0; i < numpanels; i++)
  {
    newsource = gtk_editable_get_chars
                         (GTK_EDITABLE (panels[i].sourcebox), 0, -1);
    diff = strcmp (newsource, panels[i].source);
    g_free (panels[i].source);
    panels[i].source = newsource;
    if (diff)
      create_sources_list (&panels[i]);

    panels[i].default_period = gtk_spin_button_get_value_as_int
                               (GTK_SPIN_BUTTON (panels[i].period_spinner));
    panels[i].maintain_aspect = GTK_TOGGLE_BUTTON (panels[i].aspect_box)->active;
    panels[i].random = GTK_TOGGLE_BUTTON (panels[i].random_box)->active;
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

  popup_errors = gtk_toggle_button_get_active
                                       (GTK_TOGGLE_BUTTON (popup_errors_box));
}

void kkam_cleanup ()
{
  int i;

  for (i = 0; i < MAX_NUMPANELS; i++)
    destroy_sources_list (&panels[i]);
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
  
  /* the g_new0 initialized everything to 0- pretty convenient for
     almost everything.. */

  for (i = 0; i < MAX_NUMPANELS; i++)
  {
    panels[i].height = 50;
    panels[i].source = g_strdup (default_source[i]);
    panels[i].default_period = 60;
  }

  /* gkrellm now hooks INT and QUIT and exits nicely. This will work. */

  g_atexit (kkam_cleanup);
 
  return (monitor = &kam_mon);
}
