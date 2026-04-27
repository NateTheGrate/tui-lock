#include <fontconfig/fontconfig.h>
#include <gtk/gtk.h>
#include <pango/pango.h>
#include <vte/vte.h>

#include "colors.h"
#include "glib.h"
#include "gtklock.h"
#include "prompt.h"
#include "window.h"

extern void config_load(const char* path, const char* group,
                        GOptionEntry entries[]);

const char module_name[] = "tui-lock";
const guint module_major_version = 4;
const guint module_minor_version = 0;

static VteTerminal* term = NULL;
static char password[256] = {0};
static int pw_len = 0;
static char* log_path = "/tmp/tui-lock.log";
static const char* fallback_font_family = "Monospace";
static ColorTheme theme = {0};

// config variable defaults
static gboolean debug_mode = FALSE;
static gint font_size = 12;  // points
static char* font_family = "Monospace";
static gint border_x = 320;  // in pixels
static gint border_y = 180;  // in pixels

static gint border_style = MIN_BORDER_STYLE;  // 1 = single
                                              // 2 = double
                                              // 3 = single curved

static gint prompt_style = MIN_PROMPT_STYLE;  // 1 = User and Password boxes
                                              // 2 = Just Password box
static gchar* login_text = "Login";
// default theme is terminal green
static char* border_color_hex = "#33FF00";
static char* bg_color_hex = "#000000";
static char* fg_color_hex = "#FFFFFF";
static char* login_text_color_hex = "#FFFFFF";
static char* prompt_text_color_hex = "#33FF00";
static gchar* logout_cmd = NULL;
static gchar* shutdown_cmd = NULL;
static gchar* reboot_cmd = NULL;

GOptionEntry module_entries[] = {
    {"debug", 0, 0, G_OPTION_ARG_NONE, &debug_mode, NULL, NULL},
    {"font-size", 0, 0, G_OPTION_ARG_INT, &font_size, NULL, NULL},
    {"font-family", 0, 0, G_OPTION_ARG_STRING, &font_family, NULL, NULL},
    {"border-x", 0, 0, G_OPTION_ARG_INT, &border_x, NULL, NULL},
    {"border-y", 0, 0, G_OPTION_ARG_INT, &border_y, NULL, NULL},
    {"border-style", 0, 0, G_OPTION_ARG_INT, &border_style, NULL, NULL},
    {"border-color", 0, 0, G_OPTION_ARG_STRING, &border_color_hex, NULL, NULL},
    {"bg-color", 0, 0, G_OPTION_ARG_STRING, &bg_color_hex, NULL, NULL},
    {"fg-color", 0, 0, G_OPTION_ARG_STRING, &fg_color_hex, NULL, NULL},
    {"login-color", 0, 0, G_OPTION_ARG_STRING, &login_text_color_hex, NULL,
     NULL},
    {"login-text", 0, 0, G_OPTION_ARG_STRING, &login_text, NULL, NULL},
    {"prompt-style", 0, 0, G_OPTION_ARG_INT, &prompt_style, NULL, NULL},
    {"prompt-color", 0, 0, G_OPTION_ARG_STRING, &prompt_text_color_hex, NULL,
     NULL},
    {"logout-cmd", 0, 0, G_OPTION_ARG_STRING, &logout_cmd, NULL, NULL},
    {"shutdown-cmd", 0, 0, G_OPTION_ARG_STRING, &shutdown_cmd, NULL, NULL},
    {"reboot-cmd", 0, 0, G_OPTION_ARG_STRING, &reboot_cmd, NULL, NULL},
    {NULL},
};

static void write_line_to_log(const char* string) {
  if (debug_mode) {
    const char* prefix = "TUI-Module: ";
    FILE* f = fopen(log_path, "a");
    fprintf(f, "%s%s\n", prefix, string);
    fclose(f);
  }
}

void setup_logging() {
  const char* cache_dir = g_get_user_cache_dir();
  char* log_dir = g_build_filename(cache_dir, "tui-lock", NULL);
  g_mkdir_with_parents(log_dir, 0755);
  log_path = g_build_filename(log_dir, "tui-lock.log", NULL);

  g_free(log_dir);
}

// find and replace $USER with actual user name
// otherwise return original string
// WARNING memory created on heap, needs g_free at some point
gchar* resolve_user_text(const gchar* str) {
  if (!str) return NULL;
  const gchar* username = g_get_user_name();
  gchar** parts = g_strsplit(str, "$USER", -1);
  gchar* resolved = g_strjoinv(username, parts);
  g_strfreev(parts);
  return resolved;
}

void on_activation(struct GtkLock* lock, int id) {
  setup_logging();
  write_line_to_log("Module loaded successfully");

  if (border_style > MAX_BORDER_STYLE) {
    border_style = MAX_BORDER_STYLE;
  } else if (border_style < MIN_BORDER_STYLE) {
    border_style = MIN_BORDER_STYLE;
  }

  if (prompt_style > MAX_PROMPT_STYLE) {
    prompt_style = MAX_PROMPT_STYLE;
  } else if (prompt_style < MIN_PROMPT_STYLE) {
    prompt_style = MIN_PROMPT_STYLE;
  }

  theme = (ColorTheme){hex_to_rgba(bg_color_hex), hex_to_rgba(fg_color_hex),
                       hex_to_rgba(border_color_hex),
                       hex_to_rgba(login_text_color_hex),
                       hex_to_rgba(prompt_text_color_hex)};

  login_text = resolve_user_text(login_text);
  if (logout_cmd == NULL) {
    logout_cmd = resolve_user_text("loginctl terminate-user $USER");
  } else {
    logout_cmd = resolve_user_text(logout_cmd);
  }

  if (shutdown_cmd == NULL) {
    shutdown_cmd = resolve_user_text("systemctl -i poweroff");
  } else {
    shutdown_cmd = resolve_user_text(shutdown_cmd);
  }

  if (reboot_cmd == NULL) {
    reboot_cmd = resolve_user_text("systemctl reboot");
  } else {
    reboot_cmd = resolve_user_text(shutdown_cmd);
  }
}

static void on_resize(GtkWidget* widget, GdkRectangle* alloc, gpointer data) {
  draw_prompt(term, pw_len, border_x, border_y, border_style, &theme,
              login_text, prompt_style);
}

static GtkWidget* input_field = NULL;
static gboolean on_key_press(GtkWidget* widget, GdkEventKey* event,
                             gpointer data) {
  guint key = event->keyval;

  switch (key) {
    case GDK_KEY_F1:
      g_spawn_command_line_async(logout_cmd, NULL);
      return TRUE;
    case GDK_KEY_F2:
      g_spawn_command_line_async(shutdown_cmd, NULL);
      return TRUE;
    case GDK_KEY_F3:
      g_spawn_command_line_async(reboot_cmd, NULL);
      return TRUE;
    case GDK_KEY_BackSpace:
      if (pw_len > 0) {
        pw_len--;
        password[pw_len] = '\0';
        draw_prompt(term, pw_len, border_x, border_y, border_style, &theme,
                    login_text, prompt_style);
      }
      break;
    case GDK_KEY_Return:
      write_line_to_log("Enter pressed: gtklock should handle password now");
      break;

    default:
      guint32 ch = gdk_keyval_to_unicode(key);
      if (ch && g_unichar_isprint(ch) && pw_len < (int)(sizeof(password) - 1)) {
        password[pw_len++] = (char)ch;
        password[pw_len] = '\0';
        draw_prompt(term, pw_len, border_x, border_y, border_style, &theme,
                    login_text, prompt_style);
      }
      break;
  }

  // forward all keystrokes to input_field
  // gtklock will handle everything else like unlocking and handling the
  // password
  if (input_field) {
    GdkEvent* copy = gdk_event_copy((GdkEvent*)event);
    gtk_widget_event(input_field, copy);
    gdk_event_free(copy);
  }

  return TRUE;  // swallow the event — don't pass to VTE
}

static gboolean grab_focus_idle(gpointer data) {
  gtk_widget_grab_focus(GTK_WIDGET(data));
  return G_SOURCE_REMOVE;  // only runs once
}

static gboolean on_map(GtkWidget* widget, gpointer data) {
  // queue to run in the next main loop iteration,
  // so gtklock's focus logic doesn't override ours
  g_idle_add(grab_focus_idle, widget);
  return FALSE;
}

static void set_vte_font_size(VteTerminal* term, int font_size) {
  // Get the current font
  const PangoFontDescription* current =
      vte_terminal_get_font(VTE_TERMINAL(term));

  // Copy it so we can modify it
  PangoFontDescription* fd = pango_font_description_copy(current);
  pango_font_description_set_size(
      fd, font_size * PANGO_SCALE);  // size is in Pango units
  vte_terminal_set_font(VTE_TERMINAL(term), fd);
  pango_font_description_free(fd);
}

char* resolve_font_family(const char* family) {
  FcPattern* pat = FcNameParse((const FcChar8*)family);
  FcConfigSubstitute(NULL, pat, FcMatchPattern);
  FcDefaultSubstitute(pat);

  FcResult result;
  FcPattern* match = FcFontMatch(NULL, pat, &result);

  FcChar8* resolved = NULL;
  FcPatternGetString(match, FC_FAMILY, 0, &resolved);

  // Duplicate before freeing the pattern
  char* out = g_strdup((char*)resolved);

  FcPatternDestroy(pat);
  FcPatternDestroy(match);
  return out;  // caller must g_free()
}

static gboolean set_vte_font_family(VteTerminal* term, const char* font_str) {
  const PangoFontDescription* current = vte_terminal_get_font(term);
  char* resolved = resolve_font_family(font_str);
  PangoFontDescription* fd = pango_font_description_copy(current);
  pango_font_description_set_family(fd, resolved);
  g_free(resolved);

  PangoFontMap* map = pango_cairo_font_map_get_default();
  PangoContext* ctx = pango_font_map_create_context(map);
  PangoFont* loaded = pango_context_load_font(ctx, fd);

  if (!loaded) {
    g_object_unref(ctx);
    pango_font_description_free(fd);
    write_line_to_log("Font failed to load");
    return FALSE;
  }

  write_line_to_log("Font was loaded successfully");
  vte_terminal_set_font(term, fd);

  g_object_unref(ctx);
  pango_font_description_free(fd);
  return TRUE;
}

static gboolean term_created = FALSE;
void on_window_create(struct GtkLock* lock, struct Window* win) {
  if (!term_created) {
    term_created = TRUE;
    input_field = win->input_field;

    GtkWidget* terminal = vte_terminal_new();
    term = VTE_TERMINAL(terminal);
    if (!terminal || !term) {
      write_line_to_log("Failed to create primary VTE");
    }
    vte_terminal_set_color_background(term, &(theme.bg));
    vte_terminal_set_color_foreground(term, &(theme.fg));

    g_signal_connect(terminal, "key-press-event", G_CALLBACK(on_key_press),
                     NULL);
    g_signal_connect(terminal, "map", G_CALLBACK(on_map), NULL);
    g_signal_connect(terminal, "size-allocate", G_CALLBACK(on_resize), NULL);
    gtk_widget_set_can_focus(terminal, TRUE);

    // fill entire screen with VTE
    gtk_widget_set_hexpand(terminal, TRUE);
    gtk_widget_set_vexpand(terminal, TRUE);
    gtk_overlay_add_overlay(GTK_OVERLAY(win->overlay), terminal);
    gtk_widget_show(terminal);

    set_vte_font_size(term, font_size);
    if (!set_vte_font_family(term, font_family)) {
      write_line_to_log("Font failed to load, trying fallback font");
      set_vte_font_family(term, fallback_font_family);
    }

    draw_prompt(term, pw_len, border_x, border_y, border_style, &theme,
                login_text, prompt_style);

    write_line_to_log("primarty VTE with prompt added to lock screen");
  } else {
    // other monitors: blank VTE, no input
    GtkWidget* blank = vte_terminal_new();
    VteTerminal* blank_term = VTE_TERMINAL(blank);
    if (!blank || !blank_term) {
      write_line_to_log("Failed to create secondary VTE");
    }
    vte_terminal_set_color_background(blank_term, &(theme.bg));
    vte_terminal_set_color_foreground(blank_term, &(theme.fg));
    vte_terminal_feed(blank_term, "\033[?25l", -1);  // hide cursor

    gtk_widget_set_can_focus(blank, FALSE);
    gtk_widget_set_hexpand(blank, TRUE);
    gtk_widget_set_vexpand(blank, TRUE);

    gtk_overlay_add_overlay(GTK_OVERLAY(win->overlay), blank);
    gtk_widget_show(blank);

    set_vte_font_size(term, font_size);
    if (!set_vte_font_family(term, font_family)) {
      write_line_to_log("Font failed to load, trying fallback font");
      set_vte_font_family(term, fallback_font_family);
    }

    write_line_to_log("Created blank VTE on secondary monitors");
  }
}
