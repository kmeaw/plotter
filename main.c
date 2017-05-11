#include <errno.h>
#include <SDL/SDL.h>
#include <SDL/SDL_rotozoom.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int SCREEN_BPP = 32;

SDL_Surface *screen = NULL;
SDL_Surface *target = NULL;
SDL_Event event;

double min = 0.0, max = 1.0;

char font_data[13][8] = {
  {0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00},
  {0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00},
  {0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00},
  {0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00},
  {0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00},
  {0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00},
  {0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00},
  {0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00},
  {0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00},
  {0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00},

  {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00},
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
  {0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00},
};

static int
init ()
{
  if (SDL_Init (SDL_INIT_EVERYTHING) == -1)
    return 0;

  screen =
    SDL_SetVideoMode (SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP,
		      SDL_SWSURFACE | SDL_RESIZABLE);

  if (screen == NULL)
    return 0;

  target =
    SDL_CreateRGBSurface (SDL_SWSURFACE, SCREEN_HEIGHT, SCREEN_WIDTH,
			  SCREEN_BPP, screen->format->Rmask,
			  screen->format->Gmask, screen->format->Bmask,
			  screen->format->Amask);

  if (target == NULL)
    return 0;

  SDL_WM_SetCaption ("Plotter: SDL output", NULL);

  return 1;
}

static int
handle_events ()
{
  SDL_Surface *resized;

  if (event.type == SDL_VIDEORESIZE)
    {
      resized =
	zoomSurface (target, (double) event.resize.h / target->w,
		     (double) event.resize.w / target->h, SMOOTHING_ON);
      if (resized == NULL)
	{
	  abort ();
	  return 2;
	}

      screen =
	SDL_SetVideoMode (event.resize.w, event.resize.h, SCREEN_BPP,
			  SDL_HWSURFACE | SDL_RESIZABLE);

      if (screen == NULL)
	{
	  abort ();
	  return 2;
	}

      SDL_FreeSurface (target);
      target = resized;
    }
  else if (event.type == SDL_VIDEOEXPOSE)
    {
      puts ("Exposure!");
      if (SDL_Flip (screen) == -1)
	{
	  abort ();
	  return 2;
	}
    }
  else if (event.type == SDL_QUIT)
    {
      return 2;
    }

  return 0;
}

static void
feed (double value)
{
  uint8_t *pixels = (uint8_t *) target->pixels;
  SDL_Rect src = {.x = 0,.y = 0,.w = target->w,.h = target->h - 1 };
  SDL_Rect dst = {.x = 0,.y = 1,.w = target->w,.h = target->h - 1 };
  SDL_Rect line = {.x = 0,.y = 0,.w = target->w,.h = 1 };
  double eps = (max - min) / target->w;

  SDL_BlitSurface (target, &src, target, &dst);
  SDL_FillRect (target, &line, 0);
  if (value < min || value >= max)
    {
      double w0 = max - min;
      double w1 = w0;
      double x = 0.0;
      SDL_Surface *resized;
      SDL_Rect zdst = {.x = 0,.y = 0,.w = target->w,.h = target->h };

      if (value < min)
	{
	  x = min - value;
	  w1 += min - value;
	  min = value;
	}
      if (value >= max)
	{
	  w1 += value - max + eps;
	  max = value + eps;
	}

      zdst.w = (w0 / w1) * target->w;
      zdst.x = x;

      resized = zoomSurface (target, w0 / w1, 1.0, SMOOTHING_ON);
      if (!resized)
	return;

      SDL_FillRect (target, NULL, 0);
      SDL_BlitSurface (resized, NULL, target, &zdst);
      SDL_FreeSurface (resized);
    }
  value = (value - min) / (max - min);
  pixels[(((int) (target->pitch * value)) & ~3) + 0] = 0xFF;
  pixels[(((int) (target->pitch * value)) & ~3) + 1] = 0xFF;
  pixels[(((int) (target->pitch * value)) & ~3) + 2] = 0xFF;
  pixels[(((int) (target->pitch * value)) & ~3) + 3] = 0xFF;
}

static int
process_input ()
{
  char buf[512];
  char *rbuf = buf;
  char *wbuf = buf;
  char *endptr = NULL;
  char *lf = NULL;
  double value;

  int n;

  n = read (0, wbuf, sizeof (buf) - (wbuf - buf));
  if (n <= 0)
    return 1;
  n += wbuf - buf;
  for (rbuf = buf; n > 0;)
    {
      while (rbuf[0] == ' ' && n > 0)
	{
	  rbuf++;
	  n--;
	}
      lf = memchr (rbuf, '\n', n);
      if (!lf)
	{
	  if (rbuf != buf)
	    {
	      memmove (buf, rbuf, n);
	      wbuf = buf + n;
	    }
	  break;
	}
      *lf = 0;
      endptr = memchr (rbuf, ' ', lf - buf);
      if (endptr)
	{
	  *endptr = 0;
	  endptr = NULL;
	}

      if (!strcmp (rbuf, "end"))
	return 2;

      value = strtod (rbuf, &endptr);
      if (endptr != lf && endptr && *endptr)
	fprintf (stderr, "Cannot parse: ``%s''!\n", rbuf);
      else
	feed (value);

      lf++;
      n -= lf - buf;
      rbuf = lf;
    }

  return 0;
}

int quit = 0;

static void
inthandler (int __attribute__ ((unused)) sig)
{
  quit = 2;
}

static void
font (int x, int y, const char *text)
{
  char c;
  uint32_t *dst =
    (uint32_t *) (&((uint8_t *) screen->pixels)[screen->pitch * y]);
  for (x = 0; (c = text[x >> 3]); x++)
    {
      c = (c >= '0' && c <= '9') ? (c - '0') :
	(c == '.') ? 10 : (c == '-') ? 12 : 11;
      for (y = 0; y < 8; y++)
	{
	  dst[x + screen->pitch / 4 * y] =
	    (font_data[(int) c][y & 7] & (1 << (x & 7))) ? 0xFFFFFF00 : 0;
	}
    }
}

int
main ()
{
  fd_set fds;
  fd_set efds;
  struct timespec timeout = {.tv_sec = 1 };
  int x, y;
  uint32_t *src, *dst;
  char txtbuf[16];

  signal (SIGINT, inthandler);

  if (!init ())
    return 1;

  while (quit <= 1)
    {
      FD_ZERO (&fds);
      FD_ZERO (&efds);
      FD_SET (0, &fds);
      while (quit <= 1 && SDL_PollEvent (&event))
	{
	  quit |= handle_events ();
	}

      if (quit == 1)
	usleep (100000);
      else
	switch (pselect (1, &fds, NULL, &efds, &timeout, NULL))
	  {
	  case -1:
	    if (errno == EINTR)
	      quit = 1;
	    else
	      {
		perror ("select");
		abort ();
	      }
	    break;
	  case 1:
	    if (FD_ISSET (0, &efds))
	      quit = 1;
	    else
	      quit |= process_input ();

	    SDL_LockSurface (screen);
	    src = (uint32_t *) target->pixels;
	    dst = (uint32_t *) screen->pixels;
	    for (x = 0; x < screen->w; x++)
	      for (y = 0; y < screen->h; y++)
		dst[screen->pitch / 4 * (y + 1) - x - 1] =
		  src[target->pitch / 4 * (x + 1) - y - 1];
	    snprintf (txtbuf, sizeof (txtbuf) - 1, "%f", max);
	    font (0, 0, txtbuf);
	    snprintf (txtbuf, sizeof (txtbuf) - 1, "%f", min);
	    font (0, screen->h - 8, txtbuf);
	    SDL_UnlockSurface (screen);
	    if (SDL_Flip (screen) == -1)
	      {
		perror ("SDL_Flip");
		abort ();
	      }
	    break;
	  }
    }

  SDL_Quit ();

  return 0;
}
