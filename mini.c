#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
void enable_ansi_support() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}
#else
void enable_ansi_support() {}
#endif
#define CANVAS_WIDTH 60
#define CANVAS_HEIGHT 20
#define MAX_SHAPES 100
// Colors and styles
#define COLOR_RESET   "\033[0m"
#define COLOR_TITLE   "\033[1;36m" // Cyan Bold
#define COLOR_MENU_OPTS    "\033[1;35m" // Magenta Bold
#define COLOR_SUCCESS "\033[1;32m" // Green Bold
#define COLOR_ERROR   "\033[1;31m" // Red Bold
#define COLOR_INFO    "\033[1;33m" // Yellow Bold
#define COLOR_BORDER  "\033[90m"     // Dark Grey
#define COLOR_CANVAS  "\033[1;37m"  // White Bold
typedef enum {
    SHAPE_LINE,
    SHAPE_RECTANGLE,
    SHAPE_CIRCLE,
    SHAPE_TRIANGLE
} ShapeType;
typedef struct {
    int x1, y1;
    int x2, y2;
} LineData;
typedef struct {
    int x, y; // Top-left
    int width, height;
} RectData;
typedef struct {
    int cx, cy; // Center
    int r;      // Radius
} CircleData;
typedef struct {
    int x1, y1;
    int x2, y2;
    int x3, y3;
} TriangleData;
typedef struct {
    int id;
    ShapeType type;
    union {
        LineData line;
        RectData rect;
        CircleData circle;
        TriangleData tri;
    } data;
    int is_active;
} Shape;
// Global State
char canvas[CANVAS_HEIGHT][CANVAS_WIDTH];
Shape shapes[MAX_SHAPES];
int next_shape_id = 1;
// Function Declarations
void init_canvas();
void draw_pixel(int x, int y);
void draw_line(int x1, int y1, int x2, int y2);
void draw_circle(int cx, int cy, int r);
void draw_rectangle(int x, int y, int w, int h);
void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3);
void render_all_shapes();
void display_canvas();
int get_int(const char *prompt, int min_val, int max_val);
void add_shape_menu();
void modify_shape_menu();
void delete_shape_menu();
void clear_all_shapes();
void list_active_shapes();
// Initialize canvas with underscores
void init_canvas() {
    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            canvas[y][x] = '_';
        }
    }
}
// Draw a pixel on the canvas with bounds checking
void draw_pixel(int x, int y) {
    if (x >= 0 && x < CANVAS_WIDTH && y >= 0 && y < CANVAS_HEIGHT) {
        canvas[y][x] = '*';
    }
}
// Bresenham's Line Algorithm
void draw_line(int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1);
    int sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1);
    int sy = y1 < y2 ? 1 : -1;
    int err = dx + dy;
    int e2;
    while (1) {
        draw_pixel(x1, y1);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x1 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y1 += sy;
        }
    }
}
// Midpoint Circle Algorithm (Bresenham's Circle)
void draw_circle(int cx, int cy, int r) {
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;
    // Helper macro/closure to draw all symmetric points
    #define DRAW_8_POINTS(cx, cy, x, y) \
        draw_pixel((cx) + (x), (cy) + (y)); \
        draw_pixel((cx) - (x), (cy) + (y)); \
        draw_pixel((cx) + (x), (cy) - (y)); \
        draw_pixel((cx) - (x), (cy) - (y)); \
        draw_pixel((cx) + (y), (cy) + (x)); \
        draw_pixel((cx) - (y), (cy) + (x)); \
        draw_pixel((cx) + (y), (cy) - (x)); \
        draw_pixel((cx) - (y), (cy) - (x));
    DRAW_8_POINTS(cx, cy, x, y);
    while (y >= x) {
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
        DRAW_8_POINTS(cx, cy, x, y);
    }
    #undef DRAW_8_POINTS
}
// Draw rectangle boundaries
void draw_rectangle(int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) return;
    draw_line(x, y, x + w - 1, y);                 // Top
    draw_line(x, y + h - 1, x + w - 1, y + h - 1); // Bottom
    draw_line(x, y, x, y + h - 1);                 // Left
    draw_line(x + w - 1, y, x + w - 1, y + h - 1); // Right
}
// Draw triangle boundaries
void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3) {
    draw_line(x1, y1, x2, y2);
    draw_line(x2, y2, x3, y3);
    draw_line(x3, y3, x1, y1);
}
// Clear canvas and draw all active shapes
void render_all_shapes() {
    init_canvas();
    for (int i = 0; i < MAX_SHAPES; i++) {
        if (shapes[i].is_active) {
            switch (shapes[i].type) {
                case SHAPE_LINE:
                    draw_line(shapes[i].data.line.x1, shapes[i].data.line.y1,
                              shapes[i].data.line.x2, shapes[i].data.line.y2);
                    break;
                case SHAPE_RECTANGLE:
                    draw_rectangle(shapes[i].data.rect.x, shapes[i].data.rect.y,
                                   shapes[i].data.rect.width, shapes[i].data.rect.height);
                    break;
                case SHAPE_CIRCLE:
                    draw_circle(shapes[i].data.circle.cx, shapes[i].data.circle.cy,
                                shapes[i].data.circle.r);
                    break;
                case SHAPE_TRIANGLE:
                    draw_triangle(shapes[i].data.tri.x1, shapes[i].data.tri.y1,
                                  shapes[i].data.tri.x2, shapes[i].data.tri.y2,
                                  shapes[i].data.tri.x3, shapes[i].data.tri.y3);
                    break;
            }
        }
    }
}
// Display canvas with coordinate helper rulers
void display_canvas() {
    // Top coordinate rulers (tens digit)
    printf("     ");
    for (int x = 0; x < CANVAS_WIDTH; x++) {
        if (x % 10 == 0) printf("%d", x / 10);
        else printf(" ");
    }
    printf("\n");
    // Top coordinate rulers (units digit)
    printf("     ");
    for (int x = 0; x < CANVAS_WIDTH; x++) {
        printf("%d", x % 10);
    }
    printf("\n");
    // Top Border
    printf("    " COLOR_BORDER "┌");
    for (int x = 0; x < CANVAS_WIDTH; x++) printf("─");
    printf("┐" COLOR_RESET "\n");
    // Rows
    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        printf(COLOR_BORDER "%2d │ " COLOR_RESET, y);
        int current_color = -1; // -1: none, 0: COLOR_BORDER, 1: COLOR_CANVAS
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            char c = canvas[y][x];
            if (c == '*') {
                if (current_color != 1) {
                    printf(COLOR_CANVAS);
                    current_color = 1;
                }
                printf("%c", c);
            } else {
                if (current_color != 0) {
                    printf(COLOR_BORDER);
                    current_color = 0;
                }
                printf("%c", c);
            }
        }
        printf(COLOR_BORDER " │" COLOR_RESET "\n");
    }
    // Bottom Border
    printf("    " COLOR_BORDER "└");
    for (int x = 0; x < CANVAS_WIDTH; x++) printf("─");
    printf("┘" COLOR_RESET "\n");
}
// Safe integer reader with prompt and range check
int get_int(const char *prompt, int min_val, int max_val) {
    int val;
    char buffer[100];
    while (1) {
        printf("%s", prompt);
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            continue;
        }
        // strip newline
        char *nl = strchr(buffer, '\n');
        if (nl) *nl = '\0';
        
        char *endptr;
        long parsed = strtol(buffer, &endptr, 10);
        if (endptr == buffer || *endptr != '\0') {
            printf(COLOR_ERROR "Invalid input. Please enter an integer." COLOR_RESET "\n");
            continue;
        }
        if (parsed < min_val || parsed > max_val) {
            printf(COLOR_ERROR "Value out of range (%d to %d). Please try again." COLOR_RESET "\n", min_val, max_val);
            continue;
        }
        val = (int)parsed;
        break;
    }
    return val;
}
// Add shape menu
void add_shape_menu() {
    int slot = -1;
    for (int i = 0; i < MAX_SHAPES; i++) {
        if (!shapes[i].is_active) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        printf(COLOR_ERROR "Cannot add shape: Maximum limit reached (%d shapes)." COLOR_RESET "\n", MAX_SHAPES);
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    printf("\n" COLOR_TITLE "--- Add Shape ---" COLOR_RESET "\n");
    printf("1. Line\n");
    printf("2. Rectangle\n");
    printf("3. Circle\n");
    printf("4. Triangle\n");
    printf("5. Back to Main Menu\n");
    
    int choice = get_int("Choose shape type (1-5): ", 1, 5);
    if (choice == 5) return;
    Shape new_shape;
    new_shape.id = next_shape_id++;
    new_shape.is_active = 1;
    if (choice == 1) {
        new_shape.type = SHAPE_LINE;
        printf("\nEnter parameters for Line (Coordinates range X: 0-%d, Y: 0-%d):\n", CANVAS_WIDTH - 1, CANVAS_HEIGHT - 1);
        new_shape.data.line.x1 = get_int("Start X1: ", 0, CANVAS_WIDTH - 1);
        new_shape.data.line.y1 = get_int("Start Y1: ", 0, CANVAS_HEIGHT - 1);
        new_shape.data.line.x2 = get_int("End X2: ", 0, CANVAS_WIDTH - 1);
        new_shape.data.line.y2 = get_int("End Y2: ", 0, CANVAS_HEIGHT - 1);
    } else if (choice == 2) {
        new_shape.type = SHAPE_RECTANGLE;
        printf("\nEnter parameters for Rectangle:\n");
        new_shape.data.rect.x = get_int("Top-Left X: ", 0, CANVAS_WIDTH - 1);
        new_shape.data.rect.y = get_int("Top-Left Y: ", 0, CANVAS_HEIGHT - 1);
        new_shape.data.rect.width = get_int("Width (1-80): ", 1, CANVAS_WIDTH);
        new_shape.data.rect.height = get_int("Height (1-40): ", 1, CANVAS_HEIGHT);
    } else if (choice == 3) {
        new_shape.type = SHAPE_CIRCLE;
        printf("\nEnter parameters for Circle:\n");
        new_shape.data.circle.cx = get_int("Center X: ", 0, CANVAS_WIDTH - 1);
        new_shape.data.circle.cy = get_int("Center Y: ", 0, CANVAS_HEIGHT - 1);
        new_shape.data.circle.r = get_int("Radius (1-40): ", 1, CANVAS_WIDTH);
    } else if (choice == 4) {
        new_shape.type = SHAPE_TRIANGLE;
        printf("\nEnter coordinates for 3 Triangle Vertices:\n");
        new_shape.data.tri.x1 = get_int("Vertex 1 X: ", 0, CANVAS_WIDTH - 1);
        new_shape.data.tri.y1 = get_int("Vertex 1 Y: ", 0, CANVAS_HEIGHT - 1);
        new_shape.data.tri.x2 = get_int("Vertex 2 X: ", 0, CANVAS_WIDTH - 1);
        new_shape.data.tri.y2 = get_int("Vertex 2 Y: ", 0, CANVAS_HEIGHT - 1);
        new_shape.data.tri.x3 = get_int("Vertex 3 X: ", 0, CANVAS_WIDTH - 1);
        new_shape.data.tri.y3 = get_int("Vertex 3 Y: ", 0, CANVAS_HEIGHT - 1);
    }
    shapes[slot] = new_shape;
    printf(COLOR_SUCCESS "Shape added successfully with ID %d!" COLOR_RESET "\n", new_shape.id);
    render_all_shapes();
    printf("Press Enter to continue...");
    getchar();
}
// List all currently active shapes
void list_active_shapes() {
    int count = 0;
    for (int i = 0; i < MAX_SHAPES; i++) {
        if (shapes[i].is_active) {
            count++;
            printf("ID %2d | ", shapes[i].id);
            switch (shapes[i].type) {
                case SHAPE_LINE:
                    printf("Line: (%d, %d) to (%d, %d)\n",
                           shapes[i].data.line.x1, shapes[i].data.line.y1,
                           shapes[i].data.line.x2, shapes[i].data.line.y2);
                    break;
                case SHAPE_RECTANGLE:
                    printf("Rectangle: Top-Left (%d, %d), Width: %d, Height: %d\n",
                           shapes[i].data.rect.x, shapes[i].data.rect.y,
                           shapes[i].data.rect.width, shapes[i].data.rect.height);
                    break;
                case SHAPE_CIRCLE:
                    printf("Circle: Center (%d, %d), Radius: %d\n",
                           shapes[i].data.circle.cx, shapes[i].data.circle.cy,
                           shapes[i].data.circle.r);
                    break;
                case SHAPE_TRIANGLE:
                    printf("Triangle: V1(%d, %d), V2(%d, %d), V3(%d, %d)\n",
                           shapes[i].data.tri.x1, shapes[i].data.tri.y1,
                           shapes[i].data.tri.x2, shapes[i].data.tri.y2,
                           shapes[i].data.tri.x3, shapes[i].data.tri.y3);
                    break;
            }
        }
    }
    if (count == 0) {
        printf(COLOR_INFO "(No active shapes on canvas)" COLOR_RESET "\n");
    }
}
// Modify shape parameters
void modify_shape_menu() {
    printf("\n" COLOR_TITLE "--- Modify Shape ---" COLOR_RESET "\n");
    list_active_shapes();
    // Check if there are any active shapes to modify
    int has_active = 0;
    for (int i = 0; i < MAX_SHAPES; i++) {
        if (shapes[i].is_active) { has_active = 1; break; }
    }
    if (!has_active) {
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    int id = get_int("Enter Shape ID to modify (or 0 to cancel): ", 0, next_shape_id - 1);
    if (id == 0) return;
    int index = -1;
    for (int i = 0; i < MAX_SHAPES; i++) {
        if (shapes[i].is_active && shapes[i].id == id) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        printf(COLOR_ERROR "Active shape ID %d not found." COLOR_RESET "\n", id);
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    Shape *sh = &shapes[index];
    printf("\nModifying shape ID %d:\n", sh->id);
    if (sh->type == SHAPE_LINE) {
        printf("Current parameters - X1: %d, Y1: %d, X2: %d, Y2: %d\n",
               sh->data.line.x1, sh->data.line.y1, sh->data.line.x2, sh->data.line.y2);
        sh->data.line.x1 = get_int("Enter new X1: ", 0, CANVAS_WIDTH - 1);
        sh->data.line.y1 = get_int("Enter new Y1: ", 0, CANVAS_HEIGHT - 1);
        sh->data.line.x2 = get_int("Enter new X2: ", 0, CANVAS_WIDTH - 1);
        sh->data.line.y2 = get_int("Enter new Y2: ", 0, CANVAS_HEIGHT - 1);
    } else if (sh->type == SHAPE_RECTANGLE) {
        printf("Current parameters - X: %d, Y: %d, Width: %d, Height: %d\n",
               sh->data.rect.x, sh->data.rect.y, sh->data.rect.width, sh->data.rect.height);
        sh->data.rect.x = get_int("Enter new Top-Left X: ", 0, CANVAS_WIDTH - 1);
        sh->data.rect.y = get_int("Enter new Top-Left Y: ", 0, CANVAS_HEIGHT - 1);
        sh->data.rect.width = get_int("Enter new Width (1-80): ", 1, CANVAS_WIDTH);
        sh->data.rect.height = get_int("Enter new Height (1-40): ", 1, CANVAS_HEIGHT);
    } else if (sh->type == SHAPE_CIRCLE) {
        printf("Current parameters - Center X: %d, Center Y: %d, Radius: %d\n",
               sh->data.circle.cx, sh->data.circle.cy, sh->data.circle.r);
        sh->data.circle.cx = get_int("Enter new Center X: ", 0, CANVAS_WIDTH - 1);
        sh->data.circle.cy = get_int("Enter new Center Y: ", 0, CANVAS_HEIGHT - 1);
        sh->data.circle.r = get_int("Enter new Radius (1-40): ", 1, CANVAS_WIDTH);
    } else if (sh->type == SHAPE_TRIANGLE) {
        printf("Current parameters - V1:(%d, %d), V2:(%d, %d), V3:(%d, %d)\n",
               sh->data.tri.x1, sh->data.tri.y1,
               sh->data.tri.x2, sh->data.tri.y2,
               sh->data.tri.x3, sh->data.tri.y3);
        sh->data.tri.x1 = get_int("Enter new Vertex 1 X: ", 0, CANVAS_WIDTH - 1);
        sh->data.tri.y1 = get_int("Enter new Vertex 1 Y: ", 0, CANVAS_HEIGHT - 1);
        sh->data.tri.x2 = get_int("Enter new Vertex 2 X: ", 0, CANVAS_WIDTH - 1);
        sh->data.tri.y2 = get_int("Enter new Vertex 2 Y: ", 0, CANVAS_HEIGHT - 1);
        sh->data.tri.x3 = get_int("Enter new Vertex 3 X: ", 0, CANVAS_WIDTH - 1);
        sh->data.tri.y3 = get_int("Enter new Vertex 3 Y: ", 0, CANVAS_HEIGHT - 1);
    }
    printf(COLOR_SUCCESS "Shape ID %d modified successfully!" COLOR_RESET "\n", id);
    render_all_shapes();
    printf("Press Enter to continue...");
    getchar();
}
// Delete shape menu
void delete_shape_menu() {
    printf("\n" COLOR_TITLE "--- Delete Shape ---" COLOR_RESET "\n");
    list_active_shapes();
    // Check if active shapes exist
    int has_active = 0;
    for (int i = 0; i < MAX_SHAPES; i++) {
        if (shapes[i].is_active) { has_active = 1; break; }
    }
    if (!has_active) {
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    int id = get_int("Enter Shape ID to delete (or 0 to cancel): ", 0, next_shape_id - 1);
    if (id == 0) return;
    int index = -1;
    for (int i = 0; i < MAX_SHAPES; i++) {
        if (shapes[i].is_active && shapes[i].id == id) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        printf(COLOR_ERROR "Active shape ID %d not found." COLOR_RESET "\n", id);
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    shapes[index].is_active = 0;
    printf(COLOR_SUCCESS "Shape ID %d deleted successfully!" COLOR_RESET "\n", id);
    render_all_shapes();
    printf("Press Enter to continue...");
    getchar();
}
// Clear all active shapes
void clear_all_shapes() {
    for (int i = 0; i < MAX_SHAPES; i++) {
        shapes[i].is_active = 0;
    }
    render_all_shapes();
    printf(COLOR_SUCCESS "Canvas cleared. All shapes deleted!" COLOR_RESET "\n");
    printf("Press Enter to continue...");
    getchar();
}
int main(int argc, char **argv) {
    enable_ansi_support();
    init_canvas();
    memset(shapes, 0, sizeof(shapes));
    if (argc > 1 && strcmp(argv[1], "--demo") == 0) {
        // Line
        shapes[0].id = 1;
        shapes[0].type = SHAPE_LINE;
        shapes[0].data.line.x1 = 0;
        shapes[0].data.line.y1 = 0;
        shapes[0].data.line.x2 = 20;
        shapes[0].data.line.y2 = 10;
        shapes[0].is_active = 1;
        // Rectangle
        shapes[1].id = 2;
        shapes[1].type = SHAPE_RECTANGLE;
        shapes[1].data.rect.x = 30;
        shapes[1].data.rect.y = 2;
        shapes[1].data.rect.width = 15;
        shapes[1].data.rect.height = 8;
        shapes[1].is_active = 1;
        // Circle
        shapes[2].id = 3;
        shapes[2].type = SHAPE_CIRCLE;
        shapes[2].data.circle.cx = 15;
        shapes[2].data.circle.cy = 12;
        shapes[2].data.circle.r = 6;
        shapes[2].is_active = 1;
        // Triangle
        shapes[3].id = 4;
        shapes[3].type = SHAPE_TRIANGLE;
        shapes[3].data.tri.x1 = 45;
        shapes[3].data.tri.y1 = 12;
        shapes[3].data.tri.x2 = 55;
        shapes[3].data.tri.y2 = 18;
        shapes[3].data.tri.x3 = 35;
        shapes[3].data.tri.y3 = 18;
        shapes[3].is_active = 1;
        render_all_shapes();
        printf("=== DEMO RUN COMPLETED successfully ===\n");
        display_canvas();
        return 0;
    }
    while (1) {
        // Clear screen and reset cursor to top left
        printf("\033[H\033[J");
        
        printf(COLOR_TITLE "============================================================" COLOR_RESET "\n");
        printf(COLOR_TITLE "                 2D ASCII GRAPHICS EDITOR                  " COLOR_RESET "\n");
        printf(COLOR_TITLE "============================================================" COLOR_RESET "\n");
        display_canvas();
        printf("\n" COLOR_MENU_OPTS "--- Control Menu ---" COLOR_RESET "\n");
        printf("1. Add Shape\n");
        printf("2. Modify Shape\n");
        printf("3. Delete Shape\n");
        printf("4. List Active Shapes\n");
        printf("5. Clear All Shapes\n");
        printf("6. Exit\n");
        int choice = get_int("\nSelect an option (1-6): ", 1, 6);
        if (choice == 1) {
            add_shape_menu();
        } else if (choice == 2) {
            modify_shape_menu();
        } else if (choice == 3) {
            delete_shape_menu();
        } else if (choice == 4) {
            printf("\n" COLOR_TITLE "--- Active Shapes List ---" COLOR_RESET "\n");
            list_active_shapes();
            printf("\nPress Enter to continue...");
            getchar();
        } else if (choice == 5) {
            clear_all_shapes();
        } else if (choice == 6) {
            printf(COLOR_SUCCESS "\nThank you for using the 2D ASCII Graphics Editor! Goodbye." COLOR_RESET "\n");
            break;
        }
    }
    return 0;
}
