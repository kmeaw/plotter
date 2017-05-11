#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <SDL/SDL.h>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int SCREEN_BPP = 32;

SDL_Surface *screen = NULL;
SDL_Event event;

static int
init ()
{
  if (SDL_Init (SDL_INIT_EVERYTHING) == -1)
    {
      return 0;
    }

  screen =
    SDL_SetVideoMode (SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP,
		      SDL_SWSURFACE | SDL_RESIZABLE);

  if (screen == NULL)
    {
      return 0;
    }

  SDL_WM_SetCaption ("Window Event Test", NULL);

  return 1;
}

static void
redraw ()
{
  int x, y;
  SDL_LockSurface (screen);
  uint8_t *pixels = (uint8_t *) screen->pixels;
  for (y = 0; y < screen->h; y++)
    for (x = 0; x < screen->w; x++)
      {
	pixels[y * screen->pitch + x * 4 + 0] = x & 0xFF;
	pixels[y * screen->pitch + x * 4 + 1] = y & 0xFF;
	pixels[y * screen->pitch + x * 4 + 2] = (x ^ y) & 0xFF;
	pixels[y * screen->pitch + x * 4 + 3] = 0xFF;
      }

  SDL_UnlockSurface (screen);
}

static void
handle_events ()
{
  if (event.type == SDL_VIDEORESIZE)
    {
      screen =
	SDL_SetVideoMode (event.resize.w, event.resize.h, SCREEN_BPP,
			  SDL_HWSURFACE | SDL_RESIZABLE);

      if (screen == NULL)
	{
	  return;
	}

      redraw ();
    }
  else if (event.type == SDL_VIDEOEXPOSE)
    {
      puts ("Exposure!");
      redraw ();
      if (SDL_Flip (screen) == -1)
	{
	  return;
	}
    }
}

static void
feed (double value)
{
  uint8_t *pixels = (uint8_t *) screen->pixels;
  memmove (pixels + screen->pitch, pixels, screen->pitch * (screen->h - 1));
  memset (pixels, 0x00, screen->pitch);
  if (value >= 0.0 && value < 1.0)
    {
      pixels[(int) (screen->pitch * value) + 0] = 0xFF;
      pixels[(int) (screen->pitch * value) + 1] = 0xFF;
      pixels[(int) (screen->pitch * value) + 2] = 0xFF;
      pixels[(int) (screen->pitch * value) + 3] = 0xFF;
    }
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

int
main ()
{
  fd_set fds;
  fd_set efds;
  struct timespec timeout = {.tv_sec = 1 };

  signal (SIGINT, inthandler);

  if (init () == 0)
    {
      return 1;
    }

  while (quit <= 1)
    {
      FD_ZERO (&fds);
      FD_ZERO (&efds);
      FD_SET (0, &fds);
      while (SDL_PollEvent (&event))
	{
	  handle_events ();

	  if ((event.type == SDL_KEYDOWN)
	      && (event.key.keysym.sym == SDLK_ESCAPE))
	    {
	      quit = 2;
	    }

	  if (event.type == SDL_QUIT)
	    {
	      quit = 2;
	    }
	}

      if (quit == 1)
	{
	  usleep (100000);
	}
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
	      quit = process_input ();

	    if (SDL_Flip (screen) == -1)
	      {
		return 1;
	      }
	    break;
	  }
    }

  SDL_Quit ();

  return 0;
}
