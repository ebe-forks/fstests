/*
  test-pango.c -- Measure fullscreen write speed under X. 

  Copyright (C) 2005 Matthew Allum, Openedhand Ltd. 

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Matthew Allum mallum@openedhand.com

  ===
 
  Compile with

  gcc -Wall -O2 ` pkg-config --libs --cflags pango pangoxft` test-pango.c -o test-pango

*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <pango/pango.h>
#include <pango/pangoxft.h>


#define VERSION "0.1"

#define DEFAULT_TEXT_STR "abcdefghijklmnopqrstuvwxyz"
#define DEFAULT_FONT     "sans serif 18"
#define DEFAULT_N_LINES  20
#define DEFAULT_N_CYCLES 100

static char *TextStr     = DEFAULT_TEXT_STR;
static char *TextFont    = DEFAULT_FONT;
static int   TextNLines  = DEFAULT_N_LINES;
static int   TotalCycles = 100;

static Display *dpy;
static int      scr;	
static Visual  *vis;
static Window   root, win;
static int      depth;
static GC       gc;


static int      width, height;
static Bool     Verbose;


static unsigned long long
GetTimeInMillis(void)
{
  struct timeval  tp;

  gettimeofday(&tp, 0);
  return (unsigned long long)(tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}

static int
x_error_handler(Display     *display,
		XErrorEvent *e)
{
  char msg[255];

  XGetErrorText(display, e->error_code, msg, 255);

  fprintf(stderr, "test-x: X error (%#lx): %s (opcode: %i)\n",
	  e->resourceid, msg, e->request_code);
  fprintf(stderr, "test-x: Test Aborted!\n");

  exit(1);
}

static void
x_open(void)
{
  XGCValues gcv;
  Atom atoms_WINDOW_STATE, atoms_WINDOW_STATE_FULLSCREEN;

  /* Setup various X params */
  scr      = DefaultScreen(dpy);
  root     = DefaultRootWindow(dpy);
  depth    = DefaultDepth(dpy, scr);
  vis      = DefaultVisual(dpy, scr); 
  root     = RootWindow(dpy, scr);
  width    = DisplayWidth(dpy, scr);
  height   = DisplayHeight(dpy, scr);

  gcv.foreground = BlackPixel(dpy, scr);
  gcv.background = WhitePixel(dpy, scr);
  gcv.graphics_exposures = False;

  gc = XCreateGC( dpy, root, GCForeground | GCBackground | GCGraphicsExposures
		  , &gcv);

  /* Hints for a fullscreen window */
  atoms_WINDOW_STATE
    = XInternAtom(dpy, "_NET_WM_STATE",False);
  atoms_WINDOW_STATE_FULLSCREEN
    = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN",False);

  /* Create the Window */
  win = XCreateSimpleWindow(dpy, root, 0, 0,
			    width, height, 0,
			    BlackPixel(dpy, scr),
			    WhitePixel(dpy, scr));

  /* Set the hints needed */
  XChangeProperty(dpy, win, atoms_WINDOW_STATE, XA_ATOM, 32, 
		  PropModeReplace, 
		  (unsigned char *) &atoms_WINDOW_STATE_FULLSCREEN, 1);

  XMapWindow(dpy, win);

  XSync(dpy, False);
}

static void
x_blit(void)
{
  int          x = 0, y = 0, i, j, nchars;

  PangoContext         *pgo_context;
  PangoFontMap         *pgo_fontmap;
  PangoFontDescription *fontdes;
  PangoFont            *font;
  PangoFontMetrics     *metrics;

  XftDraw              *xftdraw;
  XftColor              xftcol;    
  XRenderColor          colortmp;

  unsigned long long start_clock, finish_clock, diff_clock; 

  start_clock = GetTimeInMillis(); 

  colortmp.red   = 0x0;
  colortmp.green = 0x0;
  colortmp.blue  = 0x0;
  colortmp.alpha = 0xffff; 

  XftColorAllocValue(dpy, DefaultVisual(dpy, scr), 
		     DefaultColormap(dpy, scr),
		     &colortmp, &xftcol);


  pgo_context = pango_xft_get_context (dpy, scr);
  pgo_fontmap = pango_xft_get_font_map (dpy, scr);

  if ((fontdes = pango_font_description_from_string(TextFont)) == NULL)
    {
      fprintf(stderr, "Failed to load font '%s', exiting.", TextFont);
      exit(-1);
    }

  pango_context_set_font_description(pgo_context, fontdes);

  if ((font = pango_font_map_load_font (pgo_fontmap, 
				       pgo_context, fontdes)) == NULL)
    {
      fprintf(stderr, "Failed to load font '%s', exiting.", TextFont);
      exit(-1);
    }

  metrics = pango_font_get_metrics(font, NULL);

  xftdraw = XftDrawCreate(dpy, (Drawable) win,DefaultVisual(dpy, scr),
			  DefaultColormap(dpy, scr));
  
 finish_clock = GetTimeInMillis();

  if (Verbose)
    {
      printf("test-pango: Font loaded, drawable + color created in %lli ms\n",
	     finish_clock - start_clock);
    }

  start_clock = GetTimeInMillis(); 
  
  for (j=0; j<TotalCycles; j++)
    {
      y = PANGO_PIXELS(pango_font_metrics_get_ascent(metrics));
      
      for (i=0; i<TextNLines; i++)
	{
	  unsigned char *str = NULL;
	  GList *items_head = NULL, *items = NULL;
	  PangoAttrList *attr_list = NULL;

	  /* XXX We dont do markup ? 
	     GError *error;

	     pango_parse_markup (text, strlen(text), 
	     0,
	     &attr_list,
	     (char **)&str,
	     NULL,
	     &error);
	  */

	  x = 0;

	  attr_list = pango_attr_list_new (); /* no markup - empty attributes */
	  str       = strdup(TextStr);

	  /* analyse string, breaking up into items */
	  items_head = items = pango_itemize (pgo_context, str, 
					      0, strlen(TextStr),
					      attr_list, NULL);

	  while (items)
	    {
	      PangoItem        *this   = (PangoItem *)items->data;
	      PangoGlyphString *glyphs = pango_glyph_string_new ();
	      PangoRectangle    rect;
       
	      /* shape current item into run of glyphs */
	      pango_shape  (&str[this->offset], this->length, 
			    &this->analysis, glyphs);

	      /* render the glyphs */
	      pango_xft_render (xftdraw, 
				&xftcol,
				this->analysis.font,
				glyphs,
				x, y);

	      /* calculate rendered area */
	      pango_glyph_string_extents (glyphs,
					  this->analysis.font,
					  &rect,
					  NULL);

	      x += ( rect.x + rect.width ) / PANGO_SCALE;
       
	      pango_item_free (this);
	      pango_glyph_string_free (glyphs);

	      items = items->next;
	    }

	  if (attr_list)  pango_attr_list_unref (attr_list);
	  if (str)        free(str); 
	  if (items_head) g_list_free (items_head);

	  y += ( PANGO_PIXELS(pango_font_metrics_get_ascent(metrics)) + PANGO_PIXELS(pango_font_metrics_get_descent(metrics) ));
	}

      XSync(dpy, False);
    }

  /* render fonts here */

  finish_clock = GetTimeInMillis();

  diff_clock  = finish_clock - start_clock;

  nchars = strlen(TextStr) * TextNLines * TotalCycles;

  printf("test-pango: Total time %lli ms, %i glyphs rendered = approx %lli glyphs per second\n",
	 diff_clock, nchars, ( 1000 * nchars ) / diff_clock);

}

static void
x_close(void)
{
  XCloseDisplay(dpy);
}

static void
usage(void)
{
  fprintf(stderr, 
	  "test-pango " VERSION "\n"
	  "usage: test-pango [options..]\n"
          "Options are;\n"
          "-display <X display>\n"
	  "--verbose\n"
	  "--text-str <str> text to render ( defaults to alphabet )\n"
	  "--font <str> Xft font to use ( defaults to " DEFAULT_FONT ")\n"
	  "--nlines <int> Number of lines to draw per cycle\n"
	  "--cycles <int>  number of times to runs the test ( default 100)\n"
	  );


  exit(1);
} 

int 
main (int argc, char **argv)
{
  char *dpy_name = getenv("DISPLAY");
  int i;

  for (i = 1; i < argc; i++) {

    if (!strcmp ("-display", argv[i]) || !strcmp ("-d", argv[i])) {
      if (++i>=argc) usage ();
      dpy_name = argv[i++];
      continue;
    }

    if (!strcmp ("--verbose", argv[i]) || !strcmp ("-v", argv[i])) {
      Verbose = True;
      continue;
    }

    if (!strcmp ("--text-str", argv[i]) ) {
      if (++i>=argc) usage ();
      TextStr = argv[i];
      continue;
    }

    if (!strcmp ("--font", argv[i]) ) {
      if (++i>=argc) usage ();
      TextFont = argv[i];
      continue;
    }

    if (!strcmp ("--nlines", argv[i]) ) {
      if (++i>=argc) usage ();
      TextNLines = atoi(argv[i]);
      if (TextNLines < 1) usage();
      continue;
    }

    if (!strcmp ("--cycles", argv[i]))  {
      if (++i>=argc) usage ();
      TotalCycles = atoi(argv[i]);
      if (TotalCycles < 1) usage();
      continue;
    }

    usage();
  }

  if ((dpy = XOpenDisplay(dpy_name)) == NULL) {
    fprintf(stderr, "Cannot connect to X server on display %s.",
	    dpy_name);
    exit(1);
  }

  XSetErrorHandler(x_error_handler);

  x_open();

  x_blit();

  x_close();

  return 0;
}
