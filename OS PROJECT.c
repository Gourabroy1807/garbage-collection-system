#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <gtk/gtk.h>

#define YOUNG_GEN_SIZE 20  // Smaller for visualization
#define OLD_GEN_SIZE 40
#define MAX_ROOTS 10
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

typedef struct Object {
    bool marked;
    bool in_old_gen;
    size_t size;
    struct Object* references[4];
    int ref_count;
    int id;  // Unique identifier for visualization
    char data[];
} Object;

typedef struct {
    Object* young_gen[YOUNG_GEN_SIZE];
    Object* old_gen[OLD_GEN_SIZE];
    Object* roots[MAX_ROOTS];
    int young_top;
    int old_top;
    int root_count;
    int obj_counter;  // For generating unique IDs
    GtkWidget* drawing_area;
    GtkWidget* window;
} Heap;

// Forward declarations
Heap* heap_create(GtkWidget* window, GtkWidget* drawing_area);
void garbage_collect(Heap* heap);
void visualize_heap(Heap* heap);

// Colors for visualization
GdkColor COLOR_YOUNG = {0, 0, 20000, 30000};    // Blue
GdkColor COLOR_OLD = {0, 30000, 20000, 0};      // Orange
GdkColor COLOR_MARKED = {0, 0, 50000, 0};       // Green
GdkColor COLOR_ROOT = {0, 50000, 0, 0};         // Red
GdkColor COLOR_TEXT = {0, 0, 0, 0};             // Black

gboolean draw_callback(GtkWidget* widget, cairo_t* cr, gpointer data) {
    Heap* heap = (Heap*)data;
    visualize_heap(heap);
    return FALSE;
}

void visualize_heap(Heap* heap) {
    GtkWidget* da = heap->drawing_area;
    GdkWindow* window = gtk_widget_get_window(da);
    if (!window) return;

    cairo_t* cr = gdk_cairo_create(window);
    
    // Set background
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    
    // Draw title
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 20);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, 50, 30);
    cairo_show_text(cr, "Garbage Collector Visualization");
    
    // Draw young generation label
    cairo_set_font_size(cr, 16);
    cairo_move_to(cr, 50, 80);
    cairo_show_text(cr, "Young Generation");
    
    // Draw old generation label
    cairo_move_to(cr, 450, 80);
    cairo_show_text(cr, "Old Generation");
    
    // Draw young generation objects
    for (int i = 0; i < heap->young_top; i++) {
        if (heap->young_gen[i]) {
            int x = 50 + (i % 10) * 35;
            int y = 100 + (i / 10) * 60;
            
            // Set color based on object state
            if (heap->young_gen[i]->marked) {
                cairo_set_source_rgb(cr, 0, 0.8, 0);  // Green for marked
            } else {
                cairo_set_source_rgb(cr, 0, 0.5, 0.8);  // Blue for young
            }
            
            cairo_rectangle(cr, x, y, 30, 30);
            cairo_fill(cr);
            
            // Draw object ID
            cairo_set_source_rgb(cr, 0, 0, 0);
            cairo_set_font_size(cr, 10);
            char id_str[10];
            sprintf(id_str, "%d", heap->young_gen[i]->id);
            cairo_move_to(cr, x + 10, y + 20);
            cairo_show_text(cr, id_str);
        }
    }
    
    // Draw old generation objects
    for (int i = 0; i < heap->old_top; i++) {
        if (heap->old_gen[i]) {
            int x = 450 + (i % 10) * 35;
            int y = 100 + (i / 10) * 60;
            
            // Set color based on object state
            if (heap->old_gen[i]->marked) {
                cairo_set_source_rgb(cr, 0, 0.8, 0);  // Green for marked
            } else {
                cairo_set_source_rgb(cr, 0.8, 0.5, 0);  // Orange for old
            }
            
            cairo_rectangle(cr, x, y, 30, 30);
            cairo_fill(cr);
            
            // Draw object ID
            cairo_set_source_rgb(cr, 0, 0, 0);
            cairo_set_font_size(cr, 10);
            char id_str[10];
            sprintf(id_str, "%d", heap->old_gen[i]->id);
            cairo_move_to(cr, x + 10, y + 20);
            cairo_show_text(cr, id_str);
        }
    }
    
    // Draw references (simplified visualization)
    cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    cairo_set_line_width(cr, 1);
    for (int i = 0; i < heap->root_count; i++) {
        if (heap->roots[i]) {
            int root_x = 50 + (i % 10) * 35 + 15;
            int root_y = 100 + (i / 10) * 60 + 15;
            
            for (int j = 0; j < heap->roots[i]->ref_count; j++) {
                if (heap->roots[i]->references[j]) {
                    int ref_x, ref_y;
                    if (heap->roots[i]->references[j]->in_old_gen) {
                        for (int k = 0; k < heap->old_top; k++) {
                            if (heap->old_gen[k] == heap->roots[i]->references[j]) {
                                ref_x = 450 + (k % 10) * 35 + 15;
                                ref_y = 100 + (k / 10) * 60 + 15;
                                break;
                            }
                        }
                    } else {
                        for (int k = 0; k < heap->young_top; k++) {
                            if (heap->young_gen[k] == heap->roots[i]->references[j]) {
                                ref_x = 50 + (k % 10) * 35 + 15;
                                ref_y = 100 + (k / 10) * 60 + 15;
                                break;
                            }
                        }
                    }
                    cairo_move_to(cr, root_x, root_y);
                    cairo_line_to(cr, ref_x, ref_y);
                    cairo_stroke(cr);
                }
            }
        }
    }
    
    cairo_destroy(cr);
}

Heap* heap_create(GtkWidget* window, GtkWidget* drawing_area) {
    Heap* heap = malloc(sizeof(Heap));
    memset(heap, 0, sizeof(Heap));
    heap->window = window;
    heap->drawing_area = drawing_area;
    return heap;
}

Object* allocate_in_gen(Object** gen, int* top, int gen_size, size_t size, bool in_old_gen, Heap* heap) {
    if (*top >= gen_size) return NULL;
    
    Object* obj = malloc(sizeof(Object) + size);
    obj->marked = false;
    obj->in_old_gen = in_old_gen;
    obj->size = size;
    obj->ref_count = 0;
    obj->id = heap->obj_counter++;
    memset(obj->references, 0, sizeof(obj->references));
    
    gen[(*top)++] = obj;
    return obj;
}

Object* allocate(Heap* heap, size_t size, bool is_root) {
    // First try young generation
    Object* obj = allocate_in_gen(heap->young_gen, &heap->young_top, YOUNG_GEN_SIZE, size, false, heap);
    
    // If young gen is full, try old generation
    if (!obj) {
        obj = allocate_in_gen(heap->old_gen, &heap->old_top, OLD_GEN_SIZE, size, true, heap);
    }
    
    // If still no memory, run GC and try again
    if (!obj) {
        garbage_collect(heap);
        obj = allocate_in_gen(heap->young_gen, &heap->young_top, YOUNG_GEN_SIZE, size, false, heap);
        if (!obj) {
            obj = allocate_in_gen(heap->old_gen, &heap->old_top, OLD_GEN_SIZE, size, true, heap);
        }
    }
    
    if (is_root && obj) {
        heap->roots[heap->root_count++] = obj;
    }
    
    // Update visualization
    gtk_widget_queue_draw(heap->drawing_area);
    while (gtk_events_pending()) gtk_main_iteration();
    
    return obj;
}

void mark(Object* obj) {
    if (obj->marked) return;
    
    obj->marked = true;
    for (int i = 0; i < obj->ref_count; i++) {
        mark(obj->references[i]);
    }
}

void mark_roots(Heap* heap) {
    for (int i = 0; i < heap->root_count; i++) {
        if (heap->roots[i]) {
            mark(heap->roots[i]);
        }
    }
}

void sweep_gen(Object** gen, int* top) {
    int new_top = 0;
    for (int i = 0; i < *top; i++) {
        if (gen[i] && gen[i]->marked) {
            gen[i]->marked = false;
            gen[new_top++] = gen[i];
        } else if (gen[i]) {
            free(gen[i]);
        }
    }
    *top = new_top;
}

void promote_to_old_gen(Heap* heap) {
    for (int i = 0; i < heap->young_top; i++) {
        if (heap->young_gen[i] && heap->young_gen[i]->marked) {
            if (heap->old_top < OLD_GEN_SIZE) {
                heap->young_gen[i]->in_old_gen = true;
                heap->old_gen[heap->old_top++] = heap->young_gen[i];
                heap->young_gen[i] = NULL;
            }
        }
    }
    heap->young_top = 0;
}

void garbage_collect(Heap* heap) {
    // Mark phase
    mark_roots(heap);
    
    // Update visualization to show marked objects
    gtk_widget_queue_draw(heap->drawing_area);
    while (gtk_events_pending()) gtk_main_iteration();
    usleep(500000);  // Pause to show marked objects
    
    // Sweep phase
    sweep_gen(heap->old_gen, &heap->old_top);
    
    // Generational collection
    promote_to_old_gen(heap);
    sweep_gen(heap->young_gen, &heap->young_top);
    
    // Update visualization
    gtk_widget_queue_draw(heap->drawing_area);
    while (gtk_events_pending()) gtk_main_iteration();
}

void simulate_workload(Heap* heap) {
    // Create some root objects
    Object* root1 = allocate(heap, 100, true);
    Object* root2 = allocate(heap, 100, true);
    
    // Create object tree
    Object* child1 = allocate(heap, 50, false);
    Object* child2 = allocate(heap, 75, false);
    Object* child3 = allocate(heap, 60, false);
    
    // Set up references
    root1->references[root1->ref_count++] = child1;
    root1->references[root1->ref_count++] = child2;
    child1->references[child1->ref_count++] = child3;
    
    // Allocate more objects
    for (int i = 0; i < 15; i++) {
        allocate(heap, 10 + i*2, false);
    }
    
    // Update visualization
    gtk_widget_queue_draw(heap->drawing_area);
    while (gtk_events_pending()) gtk_main_iteration();
    usleep(1000000);
    
    // Break some references to create garbage
    root1->ref_count--;
    
    // Run GC
    garbage_collect(heap);
    
    // Allocate more after GC
    for (int i = 0; i < 5; i++) {
        allocate(heap, 20 + i*3, false);
    }
}

int main(int argc, char** argv) {
    gtk_init(&argc, &argv);
    
    // Create main window
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Garbage Collector Visualization");
    gtk_window_set_default_size(GTK_WINDOW(window), WINDOW_WIDTH, WINDOW_HEIGHT);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    // Create drawing area
    GtkWidget* drawing_area = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(window), drawing_area);
    
    // Create heap and associate with GUI
    Heap* heap = heap_create(window, drawing_area);
    
    // Connect draw callback
    g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK(draw_callback), heap);
    
    // Show the window
    gtk_widget_show_all(window);
    
    // Start a timer to run the simulation after the GUI is ready
    g_timeout_add(1000, (GSourceFunc)simulate_workload, heap);
    
    // Start main loop
    gtk_main();
    
    // Clean up
    free(heap);
    return 0;
}