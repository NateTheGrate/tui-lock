#include <gtk/gtk.h>
#include <vte/vte.h>

#include "colors.h"

typedef struct {
    const char *tl, *tr, *bl, *br;
    const char *h, *v;
    const char *lj, *rj;
} BoxChars;

static const BoxChars SINGLE = { "┌", "┐", "└", "┘", "─", "│", "├", "┤" };
static const BoxChars DOUBLE = { "╔", "╗", "╚", "╝", "═", "║", "╠", "╣" };
static const BoxChars ROUND  = { "╭", "╮", "╰", "╯", "─", "│", "├", "┤" };

static const BoxChars *box = &SINGLE;

static const char* color_border = COLOR_BORDER;
static const char* color_login = COLOR_LOGIN_TEXT;
static const char* color_username = COLOR_PROMPT_TEXT;
static const char* color_password = COLOR_PROMPT_TEXT;

static void draw_prompt(VteTerminal* term, int pw_len, int target_px_w, int target_px_h, int font_size) {
    if (!term) return;

    // Get the current font
    const PangoFontDescription *current = vte_terminal_get_font(VTE_TERMINAL(term));

    // Copy it so we can modify it
    PangoFontDescription *fd = pango_font_description_copy(current);
    pango_font_description_set_size(fd, font_size * PANGO_SCALE);  // size is in Pango units
    vte_terminal_set_font(VTE_TERMINAL(term), fd);
    pango_font_description_free(fd);

    long cell_w = vte_terminal_get_char_width(term);
    long cell_h = vte_terminal_get_char_height(term);
    if (cell_w <= 0 || cell_h <= 0) {
        // Fallback to sane defaults
        cell_w = 8;
        cell_h = 16;
    }

    // target dimensions in pixels
    if( target_px_w <= 0 || target_px_h <= 0)
    {
      target_px_w = 320;
      target_px_h = 180;
    }
    // calculate dimensions in cells (normalized for font size)
    int box_w = MAX(40, (int)(target_px_w / cell_w));
    int box_h = MAX(9,  (int)(target_px_h / cell_h));

    const char *username = g_get_user_name();

    long cols = vte_terminal_get_column_count(term);
    long rows = vte_terminal_get_row_count(term);

    int box_x = (cols - box_w) / 2;
    int box_y = (rows - box_h) / 2;

    if (box_x < 1) box_x = 1;
    if (box_y < 1) box_y = 1;

    char buf[4096];
    int off = 0;

    // Clear screen, hide cursor
    off += snprintf(buf + off, sizeof(buf) - off,
                    "\033[2J\033[3J\033[?25l");

    // --- Top border ---
    off += snprintf(buf + off, sizeof(buf) - off,
                    "\033[%d;%dH%s%s", box_y, box_x, color_border, box->tl);
    for (int i = 0; i < box_w - 2; i++)
        off += snprintf(buf + off, sizeof(buf) - off, "%s", box->h);
    off += snprintf(buf + off, sizeof(buf) - off, "%s", box->tr);

    // --- Side walls + empty interior ---
    for (int r = 1; r < box_h - 1; r++) {
        off += snprintf(buf + off, sizeof(buf) - off,
                        "\033[%d;%dH%s", box_y + r, box_x, box->v);
        for (int i = 0; i < box_w - 2; i++)
            off += snprintf(buf + off, sizeof(buf) - off, " ");
        off += snprintf(buf + off, sizeof(buf) - off, "%s", box->v);
    }

    // --- Bottom border ---
    off += snprintf(buf + off, sizeof(buf) - off,
                    "\033[%d;%dH%s", box_y + box_h - 1, box_x, box->bl);
    for (int i = 0; i < box_w - 2; i++)
        off += snprintf(buf + off, sizeof(buf) - off, "%s", box->h);
    off += snprintf(buf + off, sizeof(buf) - off, "%s", box->br);

    // --- Title centered on top border ---
    const char *title = " Login ";
    int title_len = strlen(title);
    int title_x = box_x + (box_w - title_len) / 2;
    off += snprintf(buf + off, sizeof(buf) - off,
                    "\033[%d;%dH%s%s", box_y, title_x, color_login, title);

    // The content block spans 5 rows: user, blank, separator, blank, password
    int content_h = 5;
    int content_start = box_y + (box_h - content_h) / 2;
    int user_row = content_start;
    int sep_row = content_start + 2;
    int prompt_row = content_start + 4;

    // --- Username (static, not editable) ---
    int label_x  = box_x + 2;
    off += snprintf(buf + off, sizeof(buf) - off,
                    "\033[%d;%dH%sUser:     \033[0m%s",
                    user_row, label_x, color_username, username);

    // --- Separator line inside the box ---
    off += snprintf(buf + off, sizeof(buf) - off,
                   "\033[%d;%dH%s", sep_row, box_x, color_border);
    // Left junction
    off += snprintf(buf + off, sizeof(buf) - off, "%s", box->lj);
    for (int i = 0; i < box_w - 2; i++)
        off += snprintf(buf + off, sizeof(buf) - off, "%s", box->h);
    // Right junction
    off += snprintf(buf + off, sizeof(buf) - off, "%s", box->rj);

    // --- Password label + asterisks ---
    off += snprintf(buf + off, sizeof(buf) - off,
                    "\033[%d;%dH%sPassword: \033[0m",
                    prompt_row, label_x, color_password);
    for (int i = 0; i < pw_len; i++)
        off += snprintf(buf + off, sizeof(buf) - off, "*");

    // Show cursor, reset color
    off += snprintf(buf + off, sizeof(buf) - off, "\033[?25h\033[0m");

    vte_terminal_feed(term, buf, off);
}
