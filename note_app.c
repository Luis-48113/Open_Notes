#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h> // Added for isalnum

#define NOTES_DIR "notes"

// Global widgets
GtkWidget *notes_list;
GtkWidget *editor_title;
GtkWidget *editor_text;
GtkWidget *preview_title;
GtkWidget *preview_body;

// ---------------------- FUNCTION PROTOTYPES ----------------------
void refresh_notes_list();
void view_note(GtkWidget *btn, gpointer data);
void delete_note(GtkWidget *btn, gpointer data);

// Helper to get filename without extension for UI display
char *get_display_name(const char *filename) {
    char *display_name = g_strdup(filename);
    char *dot = strrchr(display_name, '.');
    if (dot != NULL) {
        *dot = '\0';
    }
    return display_name;
}

// ---------- NEW UTILITY: Sanitize Title ----------
// Creates a clean, safe version of the title to use as a filename base.
// Returns a g_strdup, which must be g_free'd.
char *sanitize_title(const char *title) {
    if (!title || *title == '\0') {
        return g_strdup("untitled");
    }
    
    // Allocate enough memory: max length of title + 1 for null terminator
    // We use a buffer to build the sanitized string
    char *sanitized = g_malloc(strlen(title) + 1);
    char *p = sanitized;
    
    for (const char *q = title; *q != '\0'; q++) {
        // Only allow alphanumeric, space, underscore, and hyphen
        if (isalnum((unsigned char)*q) || *q == ' ' || *q == '_' || *q == '-') {
            *p++ = *q;
        }
    }
    *p = '\0'; // Null-terminate the string
    
    // If the sanitized string is empty (e.g., if title was just "!!!"), use "untitled"
    if (*sanitized == '\0') {
        g_free(sanitized);
        return g_strdup("untitled");
    }
    
    // Reallocate to save memory if needed (optional but good practice)
    sanitized = g_realloc(sanitized, strlen(sanitized) + 1);
    
    return sanitized;
}


// ---------- UTILITIES ----------
void ensure_notes_dir() {
    if (mkdir(NOTES_DIR, 0755) != 0 && errno != EEXIST) {
        g_printerr("Error creating notes folder\n");
    }
}

// save_note now takes the SANITIZED title
void save_note(const char *title, const char *content) {
    char path[256];
    // Uses the sanitized title to construct the path
    snprintf(path, sizeof(path), "%s/%s.txt", NOTES_DIR, title);

    FILE *f = fopen(path, "w");
    if (!f) {
        g_printerr("Error saving note file: %s (%s)\n", path, strerror(errno));
        return;
    }

    fputs(content, f);
    fclose(f);
}

// ---------- VIEW NOTE (No change needed here, it relies on the button name) ----------
void view_note(GtkWidget *btn, gpointer data) {
    const char *name_with_ext = gtk_widget_get_name(btn); // e.g., "My Note.txt"

    char path[256];
    snprintf(path, sizeof(path), "%s/%s", NOTES_DIR, name_with_ext); 

    FILE *f = fopen(path, "r");
    if (!f) {
        g_printerr("Error opening note file: %s (%s)\n", path, strerror(errno));
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buff = (char*)malloc(size + 1);
    if (buff == NULL) {
        fclose(f);
        g_printerr("Memory allocation failed for note content.\n");
        return;
    }
    
    if (fread(buff, 1, size, f) != size) {
        g_printerr("Error reading note content from file: %s\n", path);
    }
    buff[size] = '\0';
    fclose(f);

    char *display_title = get_display_name(name_with_ext);

    gtk_label_set_text(GTK_LABEL(preview_title), display_title);
    gtk_label_set_text(GTK_LABEL(preview_body), buff);
    
    // Also, populate the editor fields so the user can edit and save the opened note
    gtk_entry_set_text(GTK_ENTRY(editor_title), display_title);
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(editor_text));
    gtk_text_buffer_set_text(buffer, buff, -1);


    g_free(display_title); 
    free(buff);          
}

// ---------- DELETE NOTE ----------
void delete_note(GtkWidget *btn, gpointer data) {
    const char *name = gtk_widget_get_name(btn);

    char path[256];
    snprintf(path, sizeof(path), "%s/%s", NOTES_DIR, name);

    if (remove(path) != 0) {
        g_printerr("Error deleting file: %s (%s)\n", path, strerror(errno));
    }

    refresh_notes_list();
    
    // Clear preview area after deletion
    gtk_label_set_text(GTK_LABEL(preview_title), "Note Deleted");
    gtk_label_set_text(GTK_LABEL(preview_body), "");
}

// ---------- REFRESH NOTES LIST ----------
void refresh_notes_list() {
    // Clear the current list
    GList *children = gtk_container_get_children(GTK_CONTAINER(notes_list));
    for (GList *l = children; l; l = l->next)
        gtk_widget_destroy(GTK_WIDGET(l->data));
    g_list_free(children);

    DIR *d = opendir(NOTES_DIR);
    if (!d) return;

    struct dirent *ent;
    while ((ent = readdir(d))) {
        if (ent->d_name[0] == '.') continue;
        
        const char *name_with_ext = ent->d_name;
        char *display_name = get_display_name(name_with_ext);

        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

        // Display name without extension
        GtkWidget *lab = gtk_label_new(display_name);
        gtk_box_pack_start(GTK_BOX(row), lab, TRUE, TRUE, 0);

        // Buttons store the full filename ("Note 1.txt")
        GtkWidget *open_btn = gtk_button_new_with_label("Open");
        gtk_widget_set_name(open_btn, name_with_ext); 
        gtk_box_pack_start(GTK_BOX(row), open_btn, FALSE, FALSE, 0);
        g_signal_connect(open_btn, "clicked", G_CALLBACK(view_note), NULL);

        GtkWidget *del_btn = gtk_button_new_with_label("Delete");
        gtk_widget_set_name(del_btn, name_with_ext); 
        gtk_box_pack_start(GTK_BOX(row), del_btn, FALSE, FALSE, 0);
        g_signal_connect(del_btn, "clicked", G_CALLBACK(delete_note), NULL);

        gtk_list_box_insert(GTK_LIST_BOX(notes_list), row, -1);
        g_free(display_name);
    }

    closedir(d);
    gtk_widget_show_all(notes_list);
}

// ---------- SAVE (FIXED: Sanitizes title before saving) ----------
void save_note_action(GtkWidget *btn, gpointer data) {
    const char *user_title = gtk_entry_get_text(GTK_ENTRY(editor_title));
    if (strlen(user_title) == 0) {
        g_printerr("Note title cannot be empty.\n");
        return;
    }
    
    // NEW: Sanitize the user-entered title
    char *sanitized_title = sanitize_title(user_title);
    
    // If the sanitized title is "untitled", we should still ensure the user intended to save.
    if (strcmp(sanitized_title, "untitled") == 0 && strlen(user_title) > 0) {
        // If the user entered something, but it was all invalid chars, 
        // we'll still use "untitled" to save.
    }


    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(editor_text));
    GtkTextIter start, end;

    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);

    char *content = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    // Use the sanitized title for saving
    save_note(sanitized_title, content);
    refresh_notes_list();
    
    // Update the editor title to show the user what was saved
    gtk_entry_set_text(GTK_ENTRY(editor_title), sanitized_title);

    g_free(sanitized_title);
    g_free(content);
}

// ---------- CSS ----------
const char *CSS =
"window { background:#f4f4f7; }"
"#sidebar { background:#ffffff; border-right:1px solid #e0e0e0; }"
"#rightpane { background:#f8f8fa; }"
"label { font-size:15px; }"
"entry, textview { font-size:15px; padding:6px; border-radius:8px; border:1px solid #ccc; }"
"button { background:#eaeaea; border-radius:8px; padding:6px; }"
"button:hover { background:#dcdcdc; }"
;

// ---------- MAIN ----------
int main(int argc, char **argv) {
    gtk_init(&argc, &argv);

    ensure_notes_dir();

    // CSS loading
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css, CSS, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Open Notes");
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *layout = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(window), layout);

    // LEFT SIDE
    GtkWidget *left = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_name(left, "sidebar");
    gtk_widget_set_size_request(left, 250, -1);
    gtk_box_pack_start(GTK_BOX(layout), left, FALSE, FALSE, 0);

    notes_list = gtk_list_box_new();
    gtk_box_pack_start(GTK_BOX(left), notes_list, TRUE, TRUE, 0);

    // RIGHT SIDE
    GtkWidget *right = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_name(right, "rightpane");
    gtk_box_pack_start(GTK_BOX(layout), right, TRUE, TRUE, 10);

    preview_title = gtk_label_new("Select a note");
    gtk_widget_set_halign(preview_title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(right), preview_title, FALSE, FALSE, 0);

    preview_body = gtk_label_new("");
    gtk_widget_set_halign(preview_body, GTK_ALIGN_START);
    gtk_label_set_line_wrap(GTK_LABEL(preview_body), TRUE);
    gtk_box_pack_start(GTK_BOX(right), preview_body, TRUE, TRUE, 0);

    editor_title = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(editor_title), "Note title...");
    gtk_box_pack_start(GTK_BOX(right), editor_title, FALSE, FALSE, 0);

    editor_text = gtk_text_view_new();
    gtk_box_pack_start(GTK_BOX(right), editor_text, TRUE, TRUE, 0);

    GtkWidget *save_btn = gtk_button_new_with_label("Save Note");
    gtk_box_pack_start(GTK_BOX(right), save_btn, FALSE, FALSE, 0);
    g_signal_connect(save_btn, "clicked", G_CALLBACK(save_note_action), NULL);

    refresh_notes_list();
    gtk_widget_show_all(window);

    gtk_main();
    return 0;
}