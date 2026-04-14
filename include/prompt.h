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

static void draw_prompt(VteTerminal* term, int pw_len) {
    if (!term) return;

    long cols = vte_terminal_get_column_count(term);
    long rows = vte_terminal_get_row_count(term);

    const char *username = g_get_user_name();

    int box_w = 40;
    int box_h = 9;  // taller to fit both fields
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

    // --- Username (static, not editable) ---
    int user_row = box_y + 2;
    int label_x  = box_x + 2;
    off += snprintf(buf + off, sizeof(buf) - off,
                    "\033[%d;%dH%sUser:     \033[0m%s",
                    user_row, label_x, color_username, username);

    // --- Separator line inside the box ---
    int sep_row = box_y + 4;
    off += snprintf(buf + off, sizeof(buf) - off,
                   "\033[%d;%dH%s", sep_row, box_x, color_border);
    // Left junction
    off += snprintf(buf + off, sizeof(buf) - off, "%s", box->lj);
    for (int i = 0; i < box_w - 2; i++)
        off += snprintf(buf + off, sizeof(buf) - off, "%s", box->h);
    // Right junction
    off += snprintf(buf + off, sizeof(buf) - off, "%s", box->rj);

    // --- Password label + asterisks ---
    int prompt_row = box_y + 6;
    off += snprintf(buf + off, sizeof(buf) - off,
                    "\033[%d;%dH%sPassword: \033[0m",
                    prompt_row, label_x, color_password);
    for (int i = 0; i < pw_len; i++)
        off += snprintf(buf + off, sizeof(buf) - off, "*");

    // Show cursor, reset color
    off += snprintf(buf + off, sizeof(buf) - off, "\033[?25h\033[0m");

    vte_terminal_feed(term, buf, off);
}
