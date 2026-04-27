#pragma once

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

struct _ColorTheme {
  GdkRGBA bg;
  GdkRGBA fg;
  GdkRGBA border;
  GdkRGBA login;
  GdkRGBA prompt;
} typedef ColorTheme;

// must free output string
static char* ansi_str(GdkRGBA* color) {
  if (!color) return NULL;

  int r = color->red * 255;
  int g = color->green * 255;
  int b = color->blue * 255;

  char* buffer = malloc(25);
  sprintf(buffer, "\033[38;2;%d;%d;%dm", r, g, b);
  return buffer;
}

// input in format of #XXXXXX
static GdkRGBA hex_to_rgba(const char* hex_color) {
  if (strlen(hex_color) != 7) {
    GdkRGBA empty = {0.0, 0.0, 0.0, 0.0};
    return empty;
  }

  char pairs[3][3] = {
      {hex_color[1], hex_color[2], '\0'},  // red hex digits
      {hex_color[3], hex_color[4], '\0'},  // green
      {hex_color[5], hex_color[6], '\0'},  // blue
  };

  int r = (int)strtol(pairs[0], NULL, 16);
  int g = (int)strtol(pairs[1], NULL, 16);
  int b = (int)strtol(pairs[2], NULL, 16);

  GdkRGBA result = {(gdouble)r / 255, (gdouble)g / 255, (gdouble)b / 255,
                    (gdouble)1.0};
  return result;
}
