#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <3ds.h>
#include "screenshot_png.h"

#include "colors_bin.h"

#define NUM_LEVELS (Z_BEST_COMPRESSION - Z_NO_COMPRESSION + 1)

static const struct
{
  GSP_FramebufferFormats format;
  u32                    bytes_per_pixel;
  const char             *str;
} formats[] =
{
  { GSP_RGBA8_OES,   4, "rgba8",  },
  { GSP_BGR8_OES,    3, "bgr8",   },
  { GSP_RGB565_OES,  2, "rgb565", },
  { GSP_RGB5_A1_OES, 2, "rgb5a1", },
  { GSP_RGBA4_OES,   2, "rgba4",  },
};
static const size_t num_formats = sizeof(formats)/sizeof(formats[0]);

static inline void
update_gfx(void)
{
  gfxFlushBuffers();
  gspWaitForVBlank();
  gfxSwapBuffers();
}

static void
rgb_to_pixel(u8                     *dst,
             const u8               *src,
             GSP_FramebufferFormats format)
{
  u16 half;

  switch(format)
  {
    case GSP_RGBA8_OES:
      dst[0] = 0xFF;
      dst[1] = src[2];
      dst[2] = src[1];
      dst[3] = src[0];
      break;

    case GSP_BGR8_OES:
      dst[0] = src[2];
      dst[1] = src[1];
      dst[2] = src[0];
      break;

    case GSP_RGB565_OES:
      half  = (src[0] & 0xF8) << 8;
      half |= (src[1] & 0xFC) << 3;
      half |= (src[2] & 0xF8) >> 3;
      memcpy(dst, &half, sizeof(half));
      break;

    case GSP_RGB5_A1_OES:
      half  = (src[0] & 0xF8) << 8;
      half |= (src[1] & 0xF8) << 3;
      half |= (src[2] & 0xF8) >> 2;
      half |= 1;
      memcpy(dst, &half, sizeof(half));
      break;

    case GSP_RGBA4_OES:
      half  = (src[0] & 0xF0) << 8;
      half |= (src[1] & 0xF0) << 4;
      half |= (src[2] & 0xF0) << 0;
      half |= 0x0F;
      memcpy(dst, &half, sizeof(half));
      break;
  }
}

static void
fill_screen(gfxScreen_t            screen,
            GSP_FramebufferFormats format)
{
  u16 x, y;
  u16 fbWidth, fbHeight;
  u8  *fb = gfxGetFramebuffer(screen, GFX_LEFT, &fbWidth, &fbHeight);
  const u8 *colors = colors_bin;
  u16 bpp = formats[format].bytes_per_pixel;

  printf("\x1b[1;0Hw=%u h=%u bpp=%u\n", fbWidth, fbHeight, bpp*8);

  for(y = 0; y < fbHeight; ++y)
  {
    for(x = 0; x < fbWidth; ++x)
    {
      rgb_to_pixel(fb, colors, format);
      fb     += bpp;
      colors += 3;
    }
  }
}

int main(int argc, char *argv[])
{
  unsigned int format_choice = 0;
  unsigned int down;
  int          level = Z_NO_COMPRESSION;
  GSP_FramebufferFormats format = GSP_RGBA8_OES;
  static char  path[32];

  srvInit();
  aptInit();
  fsInit();
  sdmcInit();
  hidInit(NULL);
  gfxInit(GSP_BGR8_OES, GSP_RGBA8_OES, 0);
  gfxSet3D(false);

  consoleInit(GFX_TOP, NULL);

  while(aptMainLoop())
  {
    hidScanInput();
    down = hidKeysDown();

    if(down & KEY_LEFT)
      format_choice = (format_choice + num_formats-1) % num_formats;
    else if(down & KEY_RIGHT)
      format_choice = (format_choice + 1) % num_formats;

    if(down & KEY_UP)
      level = (level + 1) % NUM_LEVELS;
    else if(down & KEY_DOWN)
      level = (level + NUM_LEVELS-1) % NUM_LEVELS;

    format = formats[format_choice].format;
    sprintf(path, "/%s.%d.png", formats[format_choice].str, level);

    printf("\x1b[0;0H%-30s", path+1);
    gfxSetScreenFormat(GFX_BOTTOM, format);
    fill_screen(GFX_BOTTOM, format);
    update_gfx();

    if(down & KEY_A)
    {
      update_gfx();
      screenshot_png(path, level);
    }
    else if(down & KEY_B)
      break;
  }

  gfxExit();
  hidExit();
  sdmcExit();
  fsExit();
  aptExit();
  srvExit();

  return EXIT_SUCCESS;
}
