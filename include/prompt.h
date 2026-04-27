#pragma once

#include <gtk/gtk.h>
#include <vte/vte.h>

#include "colors.h"

typedef struct {
  const char *tl, *tr, *bl, *br;
  const char *h, *v;
  const char *lj, *rj;
} BoxChars;

#define MIN_BORDER_STYLE 1
#define MAX_BORDER_STYLE 3
#define MIN_PROMPT_STYLE 1
#define MAX_PROMPT_STYLE 2
static const BoxChars SINGLE = {"┌", "┐", "└", "┘", "─", "│", "├", "┤"};
static const BoxChars DOUBLE = {"╔", "╗", "╚", "╝", "═", "║", "╠", "╣"};
static const BoxChars ROUND = {"╭", "╮", "╰", "╯", "─", "│", "├", "┤"};

static const BoxChars* box = &SINGLE;

static void draw_prompt(VteTerminal* term, int pw_len, int target_px_w,
                        int target_px_h, int border_style, ColorTheme* theme,
                        gchar* login_text, int prompt_style) {
  if (!term) return;

  switch (border_style) {
    case 0:
      box = &SINGLE;
      break;
    case 2:
      box = &DOUBLE;
      break;
    case 3:
      box = &ROUND;
      break;
  }

  long cell_w = vte_terminal_get_char_width(term);
  long cell_h = vte_terminal_get_char_height(term);
  if (cell_w <= 0 || cell_h <= 0) {
    // Fallback to sane defaults
    cell_w = 8;
    cell_h = 16;
  }

  // target dimensions in pixels
  if (target_px_w <= 0 || target_px_h <= 0) {
    if (prompt_style == MIN_PROMPT_STYLE) {
      target_px_w = 320;
      target_px_h = 180;
    } else if (prompt_style == 2) {
      target_px_w = 250;
      target_px_h = 100;
    }
  }
  // calculate dimensions in cells (normalized for font size)
  int box_w = MAX(30, (int)(target_px_w / cell_w));
  int box_h = MAX(3, (int)(target_px_h / cell_h));

  const char* username = g_get_user_name();

  long cols = vte_terminal_get_column_count(term);
  long rows = vte_terminal_get_row_count(term);

  int box_x = (cols - box_w) / 2;
  int box_y = (rows - box_h) / 2;

  if (box_x < 1) box_x = 1;
  if (box_y < 1) box_y = 1;

  char buf[4096];
  int off = 0;

  // Clear screen, hide cursor
  off += snprintf(buf + off, sizeof(buf) - off, "\033[2J\033[3J\033[?25l");

  char* ansi_color_str = ansi_str(&(theme->border));
  // --- Top border ---
  off += snprintf(buf + off, sizeof(buf) - off, "\033[%d;%dH%s%s", box_y, box_x,
                  ansi_color_str, box->tl);
  for (int i = 0; i < box_w - 2; i++)
    off += snprintf(buf + off, sizeof(buf) - off, "%s", box->h);
  off += snprintf(buf + off, sizeof(buf) - off, "%s", box->tr);

  // --- Side walls + empty interior ---
  for (int r = 1; r < box_h - 1; r++) {
    off += snprintf(buf + off, sizeof(buf) - off, "\033[%d;%dH%s", box_y + r,
                    box_x, box->v);
    for (int i = 0; i < box_w - 2; i++)
      off += snprintf(buf + off, sizeof(buf) - off, " ");
    off += snprintf(buf + off, sizeof(buf) - off, "%s", box->v);
  }

  // --- Bottom border ---
  off += snprintf(buf + off, sizeof(buf) - off, "\033[%d;%dH%s",
                  box_y + box_h - 1, box_x, box->bl);
  for (int i = 0; i < box_w - 2; i++)
    off += snprintf(buf + off, sizeof(buf) - off, "%s", box->h);
  off += snprintf(buf + off, sizeof(buf) - off, "%s", box->br);

  // --- Logout/Shutdown/Reboot commands ---
  int cmd_row = rows;
  int cmd_col = cols / 2;
  const char* cmd_str = "Logout <F1> Shutdown <F2> Reboot <F3>";
  int center_char = strlen(cmd_str) / 2 + 1;
  int cmd_x_start = cmd_col - center_char;

  free(ansi_color_str);
  ansi_color_str = ansi_str(&(theme->fg));
  off += snprintf(buf + off, sizeof(buf) - off, "\033[%d;%dH%s%s\033[0m",
                  cmd_row, cmd_x_start, ansi_color_str, cmd_str);

  if (login_text && strlen(login_text) > 0) {
    free(ansi_color_str);
    ansi_color_str = ansi_str(&(theme->login));
    // --- Title centered on top border ---
    int title_len = strlen(login_text) + 2;  // include spaces for padding
    int title_x = box_x + (box_w - title_len) / 2;
    off += snprintf(buf + off, sizeof(buf) - off, "\033[%d;%dH%s %s ", box_y,
                    title_x, ansi_color_str, login_text);
  }
  // The content block spans 5 rows: user, blank, separator, blank, password
  int content_h = 5;
  int content_start = box_y + (box_h - content_h) / 2;
  int user_row = content_start;
  int sep_row = content_start + 2;
  int prompt_row = content_start + 4;

  // prompt style 2 is just the password field alone
  if (prompt_style == 2) {
    content_h = 3;
    content_start = box_y + (box_h - content_h) / 2 + 1;
    prompt_row = content_start;
  }

  free(ansi_color_str);
  ansi_color_str = ansi_str(&(theme->prompt));
  // --- Username (static, not editable) ---
  int label_x = box_x + 2;

  if (prompt_style == MIN_PROMPT_STYLE) {
    off += snprintf(buf + off, sizeof(buf) - off,
                    "\033[%d;%dH%sUser:     \033[0m%s", user_row, label_x,
                    ansi_color_str, username);

    free(ansi_color_str);
    ansi_color_str = ansi_str(&(theme->border));
    // --- Separator line inside the box ---
    off += snprintf(buf + off, sizeof(buf) - off, "\033[%d;%dH%s", sep_row,
                    box_x, ansi_color_str);
    // Left junction
    off += snprintf(buf + off, sizeof(buf) - off, "%s", box->lj);
    for (int i = 0; i < box_w - 2; i++)
      off += snprintf(buf + off, sizeof(buf) - off, "%s", box->h);
    // Right junction
    off += snprintf(buf + off, sizeof(buf) - off, "%s", box->rj);
  }

  const char* pw_prompt = "Password: ";
  int pw_prompt_len = (int)strlen(pw_prompt) + 3; // + 3 for x-padding
  if (pw_len + pw_prompt_len > box_w) {
    pw_len = box_w - pw_prompt_len;
  }
  free(ansi_color_str);
  ansi_color_str = ansi_str(&(theme->prompt));
  // --- Password label + asterisks ---
  off +=
      snprintf(buf + off, sizeof(buf) - off, "\033[%d;%dH%s%s\033[0m",
               prompt_row, label_x, ansi_color_str, pw_prompt);
  for (int i = 0; i < pw_len; i++)
    off += snprintf(buf + off, sizeof(buf) - off, "*");

  // Show cursor, reset color
  off += snprintf(buf + off, sizeof(buf) - off, "\033[?25h\033[0m");

  vte_terminal_feed(term, buf, off);
  free(ansi_color_str);
}
