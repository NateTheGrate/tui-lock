#include <gtk/gtk.h>
#include <vte/vte.h>
#include "gtklock.h"
#include "window.h"

#include "prompt.h"

GdkRGBA color_bg = { 0.157, 0.157, 0.157, 1.0 };  // #282828 (gruvbox dark background)
GdkRGBA color_fg = { 0.922, 0.859, 0.698, 1.0 };  // #ebdd2 (gruvbox dark foreground)

const char module_name[] = "tui-module";
const guint module_major_version = 4;
const guint module_minor_version = 0;

static VteTerminal *term = NULL;
static char password[256] = {0};
static int pw_len = 0;

static const char* log_file = "/tmp/tui-module.log";

static void write_line_to_log(const char *string) {
  const char *prefix = "TUI-Module: ";
  FILE *f = fopen(log_file, "a");
  fprintf(f, "%s%s\n", prefix, string);
  fclose(f);
}

void on_activation(struct GtkLock *lock, int id) {
  remove(log_file);  
  write_line_to_log("Module loaded successfully");
}

static void on_resize(GtkWidget *widget, GdkRectangle *alloc, gpointer data) {
    draw_prompt(term, pw_len);
}

static GtkWidget *input_field = NULL;
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    guint key = event->keyval;

    if (key == GDK_KEY_BackSpace) {
        if (pw_len > 0) {
            pw_len--;
            password[pw_len] = '\0';
            draw_prompt(term, pw_len);
        }
    } else if (key == GDK_KEY_Return) {
        write_line_to_log("Enter pressed");

    } else {
        guint32 ch = gdk_keyval_to_unicode(key);
        if (ch && g_unichar_isprint(ch) && pw_len < (int)(sizeof(password) - 1)) {
            password[pw_len++] = (char)ch;
            password[pw_len] = '\0';
            draw_prompt(term, pw_len);
        }
    }
    
    // forward all keystrokes to input_field
    // gtklock will handle everything else like unlocking and handling the password
    if(input_field)
    {
      GdkEvent *copy = gdk_event_copy((GdkEvent *)event);
      gtk_widget_event(input_field, copy);
      gdk_event_free(copy);
    }

    return TRUE; // swallow the event — don't pass to VTE
}

static gboolean grab_focus_idle(gpointer data) {
  gtk_widget_grab_focus(GTK_WIDGET(data));
  return G_SOURCE_REMOVE; // only runs once
}
static gboolean on_map(GtkWidget *widget, gpointer data) {
  // queue to run in the next main loop iteration, 
  // so gtklock's focus logic doesn't override ours
  g_idle_add(grab_focus_idle, widget);
  return FALSE;
}

static gboolean term_created = FALSE;
void on_window_create(struct GtkLock *lock, struct Window *win) {
  if(!term_created) {
    term_created = TRUE;
    input_field = win->input_field; 

    GtkWidget* terminal = vte_terminal_new();
    term = VTE_TERMINAL(terminal);
    if(!terminal || !term)
    {
      write_line_to_log("Failed to create VTE");
    }
    vte_terminal_set_color_background(term, &color_bg);
    vte_terminal_set_color_foreground(term, &color_fg);

    g_signal_connect(terminal, "key-press-event", G_CALLBACK(on_key_press), NULL);
    g_signal_connect(terminal, "map", G_CALLBACK(on_map), NULL);
    g_signal_connect(terminal, "size-allocate", G_CALLBACK(on_resize), NULL);
    gtk_widget_set_can_focus(terminal, TRUE);

    // fill entire screen with VTE
    gtk_widget_set_hexpand(terminal, TRUE);
    gtk_widget_set_vexpand(terminal, TRUE);
    gtk_overlay_add_overlay(GTK_OVERLAY(win->overlay), terminal);
    gtk_widget_show(terminal);
    
    draw_prompt(term, pw_len);

    write_line_to_log("VTE with prompt added to lock screen");
  } else {
    // other monitors: blank VTE, no input
    GtkWidget *blank = vte_terminal_new();
    VteTerminal* blank_term = VTE_TERMINAL(blank);
    if(!blank || !blank_term)
    {
      write_line_to_log("Failed to create secondary VTE");
    }
    vte_terminal_set_color_background(blank_term, &color_bg);
    vte_terminal_feed(blank_term, "\033[?25l", -1); // hide cursor

    gtk_widget_set_can_focus(blank, FALSE);
    gtk_widget_set_hexpand(blank, TRUE);
    gtk_widget_set_vexpand(blank, TRUE);

    gtk_overlay_add_overlay(GTK_OVERLAY(win->overlay), blank);
    gtk_widget_show(blank);

    write_line_to_log("Created blank VTE on secondary monitors");
  }
}
