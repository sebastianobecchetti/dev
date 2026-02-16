#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define WIDTH 1280
#define HEIGHT 720
#define NUM_PARTICLES 1000
#define DEFAULT_SPAWN_MASS 100.0 
#define GRAVITY_CONSTANT 20.0
#define BASE_DT 0.016
#define EVENT_HORIZON 15.0 // Visual size base
#define FRICTION 0.995
#define MAX_BODIES 12
#define EXPLOSION_THRESHOLD 0.1 
#define MAX_STABLE_MASS 800.0 // Supernova Trigger

// UI Constants
#define UI_PADDING 20
#define SLIDER_W 200
#define SLIDER_H 20
#define CHECKBOX_SIZE 20
#define UI_BG_W 350
#define UI_BG_H 250 

typedef enum { BODY_BH, BODY_SUN } BodyType;

typedef struct {
    double x, y;
    double vx, vy;
    int alive;
    Uint8 r, g, b;
} Particle;

typedef struct {
    double x, y;
    double vx, vy;
    double mass;
    int alive;
    BodyType type;
} Body;

// Global State
Body bodies[MAX_BODIES];
int dragging_body_idx = -1;

// Settings
double spawn_mass = DEFAULT_SPAWN_MASS;
double time_scale = 1.0;
int show_vectors = 0; 
int show_fps = 0;
int show_grid = 0;    
int paused = 0;
BodyType next_spawn_type = BODY_BH;

// Interaction
int dragging_m_slider = 0; // Mass slider
int dragging_s_slider = 0; // Speed slider

// --- Helpers ---
void spawn_body(double x, double y) {
    for(int i=0; i<MAX_BODIES; i++) {
        if (!bodies[i].alive) {
            bodies[i].alive = 1;
            bodies[i].x = x;
            bodies[i].y = y;
            bodies[i].vx = 0;
            bodies[i].vy = 0;
            bodies[i].mass = spawn_mass;
            bodies[i].type = next_spawn_type;
            return;
        }
    }
}

void init_particle_at(Particle *p, double x, double y, double speed_mult) {
    double angle = (double)rand() / RAND_MAX * 2 * M_PI;
    double dist = 20.0 + (double)rand() / RAND_MAX * 50.0;
    
    p->x = x + cos(angle) * dist;
    p->y = y + sin(angle) * dist;
    
    double velocity = 50.0 + ((double)rand() / RAND_MAX * 100.0) * speed_mult;
    p->vx = cos(angle) * velocity;
    p->vy = sin(angle) * velocity;

    p->alive = 1;
    p->r = 200 + rand() % 55;
    p->g = 100 + rand() % 100;
    p->b = 50 + rand() % 100; // Fiery colors
}

void init_particle_default(Particle *p) {
    // Find a random active Body to orbit
    int target_idx = -1;
    int active_count = 0;
    for(int i=0; i<MAX_BODIES; i++) {
        if(bodies[i].alive) active_count++;
    }

    if (active_count > 0) {
        int pick = rand() % active_count;
        int current = 0;
        for(int i=0; i<MAX_BODIES; i++) {
            if(bodies[i].alive) {
                if(current == pick) {
                    target_idx = i;
                    break;
                }
                current++;
            }
        }
    }

    double cx = WIDTH/2.0, cy = HEIGHT/2.0;
    
    if (target_idx != -1) {
        cx = bodies[target_idx].x;
        cy = bodies[target_idx].y;
    }

    double angle = (double)rand() / RAND_MAX * 2 * M_PI;
    double dist = 250.0 + (double)rand() / RAND_MAX * 350.0;
    
    p->x = cx + cos(angle) * dist;
    p->y = cy + sin(angle) * dist;
    
    double mass = (target_idx != -1) ? bodies[target_idx].mass : 100.0;
    double v_orbit = sqrt(GRAVITY_CONSTANT * mass / dist) * 10.0; 
    if (v_orbit > 300) v_orbit = 300;

    p->vx = -sin(angle) * v_orbit;
    p->vy = cos(angle) * v_orbit;
    
    p->vx += ((double)rand() / RAND_MAX - 0.5) * 50.0;
    p->vy += ((double)rand() / RAND_MAX - 0.5) * 50.0;

    p->alive = 1;
    // Cold starfield colors initially
    p->r = 100 + rand() % 55;
    p->g = 200 + rand() % 55;
    p->b = 255;
}

void init_particles(Particle *p, int count) {
    for (int i = 0; i < count; i++) {
        init_particle_default(&p[i]);
    }
}

void explode_particles(Particle *p, int count, double x, double y, int intensity) {
    int spawned = 0;
    int limit = 150 * intensity; 
    for(int i=0; i<count && spawned < limit; i++) { 
        if(!p[i].alive || (rand()%5 == 0)) { // Recycle dead or cannibalize some live
            init_particle_at(&p[i], x, y, 6.0 + intensity); 
            spawned++;
        }
    }
}

void update_physics(Particle *p, int count) {
    double dt = BASE_DT * time_scale;

    // --- Body Updates ---
    for(int i=0; i<MAX_BODIES; i++) {
        if(!bodies[i].alive) continue;
        
        // Supernova Check
        if(bodies[i].mass > MAX_STABLE_MASS) {
            bodies[i].alive = 0;
            explode_particles(p, count, bodies[i].x, bodies[i].y, 5); // Huge explosion
             if(dragging_body_idx == i) dragging_body_idx = -1;
            continue;
        }

        if(i == dragging_body_idx) { 
             bodies[i].vx = 0; bodies[i].vy = 0;
             continue;
        }

        for(int j=0; j<MAX_BODIES; j++) {
            if(i==j || !bodies[j].alive) continue;

            double dx = bodies[j].x - bodies[i].x;
            double dy = bodies[j].y - bodies[i].y;
            double dist_sq = dx*dx + dy*dy;
            double dist = sqrt(dist_sq);

            if(dist < 5.0) dist = 5.0; 

            // Visual Radius Logic (Mass based)
            double r1 = EVENT_HORIZON * (bodies[i].type == BODY_SUN ? 2.0 : 1.0); // Suns bigger visually
            double r2 = EVENT_HORIZON * (bodies[j].type == BODY_SUN ? 2.0 : 1.0);
            
            // Collision
            if (dist < (r1 + r2) * 0.8) {
                // Determine Interaction
                double m1 = bodies[i].mass;
                double m2 = bodies[j].mass;
                
                // If BH hits SUN: BH eats SUN
                if (bodies[i].type == BODY_BH && bodies[j].type == BODY_SUN) {
                    bodies[i].mass += bodies[j].mass;
                    bodies[j].alive = 0;
                     if(dragging_body_idx == j) dragging_body_idx = -1;
                }
                else if (bodies[i].type == BODY_SUN && bodies[j].type == BODY_BH) {
                    bodies[j].mass += bodies[i].mass;
                    bodies[i].alive = 0; 
                    if(dragging_body_idx == i) dragging_body_idx = -1;
                }
                // If Same Types
                else {
                    double diff = fabs(m1 - m2) / fmax(m1, m2);
                    if (diff < EXPLOSION_THRESHOLD) {
                        // Explode
                        bodies[i].alive = 0;
                        bodies[j].alive = 0;
                        explode_particles(p, count, (bodies[i].x+bodies[j].x)/2, (bodies[i].y+bodies[j].y)/2, 2);
                        if(dragging_body_idx == i || dragging_body_idx == j) dragging_body_idx = -1;
                    } else {
                        // Merge
                        if (m1 >= m2) {
                            bodies[i].mass += m2;
                            bodies[j].alive = 0;
                            if(dragging_body_idx == j) dragging_body_idx = -1;
                        }
                    }
                }
                continue; 
            }

            // Gravity
            double force = (GRAVITY_CONSTANT/10.0) * (bodies[i].mass * bodies[j].mass) / dist_sq;
            double ax = force * (dx / dist) / bodies[i].mass;
            double ay = force * (dy / dist) / bodies[i].mass;

            bodies[i].vx += ax;
            bodies[i].vy += ay;
        }
    }

    // Apply Velocity
    for(int i=0; i<MAX_BODIES; i++) {
        if(bodies[i].alive && i != dragging_body_idx) {
             bodies[i].x += bodies[i].vx * dt;
             bodies[i].y += bodies[i].vy * dt;
             
             if(bodies[i].x < 0 || bodies[i].x > WIDTH) bodies[i].vx *= -0.5;
             if(bodies[i].y < 0 || bodies[i].y > HEIGHT) bodies[i].vy *= -0.5;
             
             if(bodies[i].x < 0) bodies[i].x = 0;
             if(bodies[i].x > WIDTH) bodies[i].x = WIDTH;
             if(bodies[i].y < 0) bodies[i].y = 0;
             if(bodies[i].y > HEIGHT) bodies[i].y = HEIGHT;
        }
    }

    // --- Particle Updates ---
    for (int i = 0; i < count; i++) {
        if (!p[i].alive) {
            if (rand() % 100 == 0) init_particle_default(&p[i]);
            continue;
        }

        double total_ax = 0;
        double total_ay = 0;
        int active_bodies = 0;

        for(int j=0; j<MAX_BODIES; j++) {
            if(!bodies[j].alive) continue;
            active_bodies++;

            double dx = bodies[j].x - p[i].x;
            double dy = bodies[j].y - p[i].y;
            double dist_sq = dx*dx + dy*dy;
            double dist = sqrt(dist_sq);

            double limit = (bodies[j].type == BODY_SUN) ? 10.0 : EVENT_HORIZON;

            // Collision with Body
            if (dist < limit) {
                p[i].alive = 0;
                bodies[j].mass += 0.5; // Eat particle
                continue;
            }

            double force = GRAVITY_CONSTANT * bodies[j].mass / (dist_sq + 10.0);
            total_ax += force * (dx / dist);
            total_ay += force * (dy / dist);
        }

        if (!p[i].alive) continue; 

        if (active_bodies > 0) {
            p[i].vx += total_ax;
            p[i].vy += total_ay;
        }
        
        p[i].vx *= FRICTION;
        p[i].vy *= FRICTION;

        p[i].x += p[i].vx * dt;
        p[i].y += p[i].vy * dt;

        double speed = sqrt(p[i].vx*p[i].vx + p[i].vy*p[i].vy);
        p[i].r = (Uint8)fmin(255, speed * 2);
        p[i].g = (Uint8)fmax(0, 255 - speed * 1);
        p[i].b = 255;
        
        if (p[i].x < 0 || p[i].x > WIDTH || p[i].y < 0 || p[i].y > HEIGHT) {
            p[i].alive = 0;
        }
    }
}

// ---------------- UI & RENDERING ----------------

void draw_rect(SDL_Renderer *r, int x, int y, int w, int h, int R, int G, int B, int A) {
    SDL_SetRenderDrawColor(r, R, G, B, A);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(r, &rect);
}

void draw_rect_outline(SDL_Renderer *r, int x, int y, int w, int h, int R, int G, int B) {
    SDL_SetRenderDrawColor(r, R, G, B, 255);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderDrawRect(r, &rect);
}

void draw_char(SDL_Renderer *r, int x, int y, char c, Uint8 R, Uint8 G, Uint8 B) {
    SDL_SetRenderDrawColor(r, R, G, B, 255);
    static const int font[][5] = {
        {0b111, 0b100, 0b111, 0b101, 0b111}, // G
        {0b111, 0b100, 0b111, 0b001, 0b111}, // S
        {0b111, 0b010, 0b010, 0b010, 0b010}, // V
        {0b111, 0b100, 0b110, 0b100, 0b100}, // F
        {0b101, 0b111, 0b101, 0b101, 0b101}, // M
        {0b111, 0b101, 0b101, 0b101, 0b111}, // 0
        {0b010, 0b010, 0b010, 0b010, 0b010}, // 1
        {0b111, 0b001, 0b111, 0b100, 0b111}, // 2
        {0b111, 0b001, 0b111, 0b001, 0b111}, // 3
        {0b101, 0b101, 0b111, 0b001, 0b001}, // 4
        {0b111, 0b100, 0b111, 0b001, 0b111}, // 5
        {0b111, 0b100, 0b111, 0b101, 0b111}, // 6
        {0b111, 0b001, 0b001, 0b001, 0b001}, // 7
        {0b111, 0b101, 0b111, 0b101, 0b111}, // 8
        {0b111, 0b101, 0b111, 0b001, 0b111}, // 9
        {0b000, 0b000, 0b000, 0b000, 0b010}, // .
        {0b111, 0b111, 0b111, 0b101, 0b101}, // A
        {0b111, 0b010, 0b010, 0b010, 0b010}, // T
        {0b111, 0b110, 0b110, 0b110, 0b111}, // B
        {0b111, 0b101, 0b101, 0b101, 0b111}, // O... for BODY
        {0b101, 0b101, 0b111, 0b101, 0b101}, // H
    };
    
    int idx = -1;
    // VERY limited font map - hacking specific letters
    if(c == 'M') idx = 4; // Mass
    else if(c == 'S') idx = 1; // Speed
    else if(c == 'V') idx = 2; 
    else if(c == 'G') idx = 0; // Grid
    else if(c == 'F') idx = 3;
    else if(c >= '0' && c <= '9') idx = 5 + (c - '0');
    else if(c == '.') idx = 15;
    else if(c == 'A') idx = 16;
    else if(c == 'T') idx = 17;
    else if(c == 'B') idx = 18;
    else if(c == 'H') idx = 20;

    if(idx >= 0) {
        for(int row=0; row<5; row++) {
            for(int col=0; col<3; col++) {
                if( (font[idx][row] >> (2-col)) & 1 ) {
                    SDL_RenderDrawPoint(r, x+col*2, y+row*2); 
                    SDL_RenderDrawPoint(r, x+col*2+1, y+row*2);
                    SDL_RenderDrawPoint(r, x+col*2, y+row*2+1);
                    SDL_RenderDrawPoint(r, x+col*2+1, y+row*2+1);
                }
            }
        }
    }
}

void draw_text(SDL_Renderer *r, int x, int y, const char* str) {
    for(int i=0; str[i] != '\0'; i++) {
        draw_char(r, x + i*10, y, str[i], 255, 255, 255);
    }
}

void draw_grid(SDL_Renderer *r) {
    SDL_SetRenderDrawColor(r, 40, 50, 80, 255); 
    int step = 40;
    
    for (int x = 0; x <= WIDTH; x += step) {
        for (int y = 0; y <= HEIGHT; y += step) {
            // Horizontal Line Segment
            if (x + step <= WIDTH) {
                double x1 = x, y1 = y;
                double x2 = x + step, y2 = y;
                
                // Distort P1
                for(int i=0; i<MAX_BODIES; i++){
                    if(!bodies[i].alive) continue;
                    double dx = bodies[i].x - x1; double dy = bodies[i].y - y1;
                    double dist = sqrt(dx*dx+dy*dy);
                    double f = (GRAVITY_CONSTANT * (bodies[i].mass/100.0) * 4.0) / (dist + 50.0);
                    if(f > dist*0.9) f=dist*0.9;
                    x1 += dx/dist*f;
                    y1 += dy/dist*f;
                }
                // Distort P2
                for(int i=0; i<MAX_BODIES; i++){
                    if(!bodies[i].alive) continue;
                    double dx = bodies[i].x - x2; double dy = bodies[i].y - y2;
                    double dist = sqrt(dx*dx+dy*dy);
                    double f = (GRAVITY_CONSTANT * (bodies[i].mass/100.0) * 4.0) / (dist + 50.0);
                    if(f > dist*0.9) f=dist*0.9;
                    x2 += dx/dist*f;
                    y2 += dy/dist*f;
                }
                SDL_RenderDrawLine(r, (int)x1, (int)y1, (int)x2, (int)y2);
            }
            
            // Vertical Line Segment
            if (y + step <= HEIGHT) {
                double x1 = x, y1 = y;
                double x2 = x, y2 = y + step;
                
                // Distort P1
                for(int i=0; i<MAX_BODIES; i++){
                    if(!bodies[i].alive) continue;
                    double dx = bodies[i].x - x1; double dy = bodies[i].y - y1;
                    double dist = sqrt(dx*dx+dy*dy);
                    double f = (GRAVITY_CONSTANT * (bodies[i].mass/100.0) * 4.0) / (dist + 50.0);
                    if(f > dist*0.9) f=dist*0.9;
                    x1 += dx/dist*f;
                    y1 += dy/dist*f;
                }
                // Distort P2
                for(int i=0; i<MAX_BODIES; i++){
                    if(!bodies[i].alive) continue;
                    double dx = bodies[i].x - x2; double dy = bodies[i].y - y2;
                    double dist = sqrt(dx*dx+dy*dy);
                    double f = (GRAVITY_CONSTANT * (bodies[i].mass/100.0) * 4.0) / (dist + 50.0);
                    if(f > dist*0.9) f=dist*0.9;
                    x2 += dx/dist*f;
                    y2 += dy/dist*f;
                }
                SDL_RenderDrawLine(r, (int)x1, (int)y1, (int)x2, (int)y2);
            }
        }
    }
}

void draw_ui(SDL_Renderer *renderer, int current_fps) {
    draw_rect(renderer, UI_PADDING - 10, UI_PADDING - 10, UI_BG_W, UI_BG_H, 20, 20, 20, 200);
    draw_rect_outline(renderer, UI_PADDING - 10, UI_PADDING - 10, UI_BG_W, UI_BG_H, 60, 60, 60);

    int y = UI_PADDING;
    char buf[32];

    // --- Mass ---
    draw_char(renderer, UI_PADDING, y+5, 'M', 200, 200, 200);
    draw_rect(renderer, UI_PADDING + 30, y, SLIDER_W, SLIDER_H, 50, 50, 50, 255);
    double m_ratio = spawn_mass / 500.0;
    if (m_ratio > 1) m_ratio = 1;
    draw_rect(renderer, UI_PADDING + 30 + (int)(m_ratio * SLIDER_W) - 6, y-2, 12, SLIDER_H+4, 180, 180, 180, 255);
    sprintf(buf, "%d", (int)spawn_mass);
    draw_text(renderer, UI_PADDING + SLIDER_W + 50, y+5, buf);
    y += SLIDER_H + 20;

    // --- Speed ---
    draw_char(renderer, UI_PADDING, y+5, 'S', 200, 200, 200);
    draw_rect(renderer, UI_PADDING + 30, y, SLIDER_W, SLIDER_H, 50, 50, 50, 255);
    double s_ratio = time_scale / 3.0;
    if (s_ratio > 1) s_ratio = 1;
    draw_rect(renderer, UI_PADDING + 30 + (int)(s_ratio * SLIDER_W) - 6, y-2, 12, SLIDER_H+4, 180, 180, 180, 255);
    sprintf(buf, "%.1f", time_scale);
    draw_text(renderer, UI_PADDING + SLIDER_W + 50, y+5, buf);
    y += SLIDER_H + 30;

    // --- Toggles ---
    int toggle_y = y;
    int x_base = UI_PADDING;
    
    // Vectors (V)
    draw_char(renderer, x_base + CHECKBOX_SIZE + 8, toggle_y+3, 'V', 255, 255, 0);
    if (show_vectors) draw_rect(renderer, x_base + 3, toggle_y + 3, CHECKBOX_SIZE - 6, CHECKBOX_SIZE - 6, 255, 255, 0, 255);
    draw_rect_outline(renderer, x_base, toggle_y, CHECKBOX_SIZE, CHECKBOX_SIZE, 150, 150, 150);

    // Grid (G)
    x_base += 80;
    draw_char(renderer, x_base + CHECKBOX_SIZE + 8, toggle_y+3, 'G', 100, 100, 255);
    if (show_grid) draw_rect(renderer, x_base + 3, toggle_y + 3, CHECKBOX_SIZE - 6, CHECKBOX_SIZE - 6, 100, 100, 255, 255);
    draw_rect_outline(renderer, x_base, toggle_y, CHECKBOX_SIZE, CHECKBOX_SIZE, 150, 150, 150);

    // FPS (F)
    x_base += 80;
    draw_char(renderer, x_base + CHECKBOX_SIZE + 8, toggle_y+3, 'F', 200, 200, 200);
    if (show_fps) {
        draw_rect(renderer, x_base + 3, toggle_y + 3, CHECKBOX_SIZE - 6, CHECKBOX_SIZE - 6, 200, 200, 200, 255);
        sprintf(buf, "%d", current_fps);
        draw_text(renderer, x_base + CHECKBOX_SIZE + 25, toggle_y+3, buf);
    }
    draw_rect_outline(renderer, x_base, toggle_y, CHECKBOX_SIZE, CHECKBOX_SIZE, 150, 150, 150);
    
    y += 40;
    // --- Type Info ---
    if (next_spawn_type == BODY_BH) draw_text(renderer, UI_PADDING, y, "TYPE BH TAB");
    else draw_text(renderer, UI_PADDING, y, "TYPE SUN TAB");
}

int handle_ui_input(int mx, int my, int is_down, int is_move) {
    int y = UI_PADDING;
    int slider_x = UI_PADDING + 30;

    // Mass
    if ((is_down || (is_move && dragging_m_slider)) && 
        mx >= slider_x && mx <= slider_x + SLIDER_W && 
        my >= y - 5 && my <= y + SLIDER_H + 5) {
        if (is_down) dragging_m_slider = 1;
        double ratio = (double)(mx - slider_x) / SLIDER_W;
        spawn_mass = ratio * 500.0;
        return 1;
    }
    y += SLIDER_H + 20;

    // Speed
    if ((is_down || (is_move && dragging_s_slider)) && 
        mx >= slider_x && mx <= slider_x + SLIDER_W && 
        my >= y - 5 && my <= y + SLIDER_H + 5) {
        if (is_down) dragging_s_slider = 1;
        double ratio = (double)(mx - slider_x) / SLIDER_W;
        time_scale = ratio * 3.0; 
        if(time_scale < 0.1) time_scale = 0.1;
        return 1;
    }
    y += SLIDER_H + 30;

    int toggle_y = y;
    int x_base = UI_PADDING;
    
    // Vectors
    if (is_down && mx >= x_base && mx <= x_base + CHECKBOX_SIZE + 40 &&
        my >= toggle_y && my <= toggle_y + CHECKBOX_SIZE) {
        show_vectors = !show_vectors;
        return 1;
    }
    
    // Grid
    x_base += 80;
    if (is_down && mx >= x_base && mx <= x_base + CHECKBOX_SIZE + 40 &&
        my >= toggle_y && my <= toggle_y + CHECKBOX_SIZE) {
        show_grid = !show_grid;
        return 1;
    }

    // FPS
    x_base += 80;
    if (is_down && mx >= x_base && mx <= x_base + CHECKBOX_SIZE + 40 &&
        my >= toggle_y && my <= toggle_y + CHECKBOX_SIZE) {
        show_fps = !show_fps;
        return 1;
    }
    
    // TYPE Toggle (Bottom area)
    y += 40;
    if (is_down && mx >= UI_PADDING && mx <= UI_PADDING + 200 &&
        my >= y && my <= y + 20) {
        next_spawn_type = (next_spawn_type == BODY_BH) ? BODY_SUN : BODY_BH;
        return 1;
    }

    return 0;
}

void get_bh_color(double mass, Uint8 *r, Uint8 *g, Uint8 *b) {
    // Red (Low) -> Violet (High)
    // Range 100 -> 500
    double t = (mass - 100.0) / 400.0;
    if (t < 0) t = 0; if (t > 1) t = 1;
    
    *r = (Uint8)(255 * (1-t) + 148 * t);
    *g = 0;
    *b = (Uint8)(0 * (1-t) + 211 * t);
}

int main() {
    srand(time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;

    SDL_Window *window = SDL_CreateWindow("Black Hole Sim", 
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                                          WIDTH, HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) return 1;

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    
    SDL_RenderSetLogicalSize(renderer, WIDTH, HEIGHT);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Init bodies
    for(int i=0; i<MAX_BODIES; i++) bodies[i].alive = 0;
    
    spawn_body(WIDTH/2.0, HEIGHT/2.0); // Initial Body

    Particle *particles = (Particle*)malloc(sizeof(Particle) * NUM_PARTICLES);
    init_particles(particles, NUM_PARTICLES);

    int quit = 0;
    SDL_Event e;
    int frame_count = 0;
    int current_fps_display = 0;
    Uint32 fps_timer = SDL_GetTicks();
    
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) quit = 1;
             else if (e.type == SDL_KEYDOWN) {
                 if (e.key.keysym.sym == SDLK_ESCAPE) paused = !paused;
                 else if (e.key.keysym.sym == SDLK_TAB) {
                     next_spawn_type = (next_spawn_type == BODY_BH) ? BODY_SUN : BODY_BH;
                 }
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mx = e.button.x;
                int my = e.button.y;
                int ui_hit = handle_ui_input(mx, my, 1, 0);
                
                if (!ui_hit) {
                     if(e.button.button == SDL_BUTTON_RIGHT) {
                         spawn_body(mx, my);
                     }
                     else if(e.button.button == SDL_BUTTON_LEFT) {
                        dragging_body_idx = -1;
                        double closest = 50.0*50.0;
                        for(int i=0; i<MAX_BODIES; i++) {
                            if(bodies[i].alive) {
                                double dx = mx - bodies[i].x;
                                double dy = my - bodies[i].y;
                                if(dx*dx + dy*dy < closest) {
                                    closest = dx*dx + dy*dy;
                                    dragging_body_idx = i;
                                }
                            }
                        }
                     }
                }
            }
            else if (e.type == SDL_MOUSEBUTTONUP) {
                dragging_body_idx = -1;
                dragging_m_slider = 0;
                dragging_s_slider = 0;
            }
            else if (e.type == SDL_MOUSEMOTION) {
                int mx = e.motion.x;
                int my = e.motion.y;
                
                handle_ui_input(mx, my, 0, 1);

                if (dragging_body_idx != -1) {
                    bodies[dragging_body_idx].x = mx;
                    bodies[dragging_body_idx].y = my;
                    bodies[dragging_body_idx].vx = 0;
                    bodies[dragging_body_idx].vy = 0;
                }
            }
        }

         if (!paused) {
            update_physics(particles, NUM_PARTICLES);
        }

        // --- Drawing ---
        SDL_SetRenderDrawColor(renderer, 5, 5, 10, 255); 
        SDL_RenderClear(renderer);

        if (show_grid) {
            draw_grid(renderer);
        }

        if (show_vectors) {
            SDL_SetRenderDrawColor(renderer, 255, 230, 50, 180); 
            for(int i=0; i<NUM_PARTICLES; i++) {
                if(!particles[i].alive) continue;
                int x1 = (int)particles[i].x;
                int y1 = (int)particles[i].y;
                int x2 = x1 + (int)(particles[i].vx * 2.0); 
                int y2 = y1 + (int)(particles[i].vy * 2.0);
                SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
            }
        }

        // Draw Bodies
         for(int i=0; i<MAX_BODIES; i++) {
             if(!bodies[i].alive) continue;
             
             int bx = (int)bodies[i].x;
             int by = (int)bodies[i].y;

             if (bodies[i].type == BODY_BH) {
                // Event Horizon
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                for(int w=-EVENT_HORIZON; w<=EVENT_HORIZON; w++) {
                    for(int h=-EVENT_HORIZON; h<=EVENT_HORIZON; h++) {
                        if (w*w + h*h <= EVENT_HORIZON*EVENT_HORIZON) {
                            SDL_RenderDrawPoint(renderer, bx + w, by + h);
                        }
                    }
                }
                
                // Accretion Disk (Red -> Violet)
                Uint8 R, G, B;
                get_bh_color(bodies[i].mass, &R, &G, &B);
                
                SDL_SetRenderDrawColor(renderer, R, G, B, 255); 
                double disk_radius = EVENT_HORIZON * 1.5; 
                 for(int w=-disk_radius; w<=disk_radius; w++) {
                    for(int h=-disk_radius; h<=disk_radius; h++) {
                        double r2 = w*w + h*h;
                         if (r2 <= disk_radius*disk_radius && r2 > EVENT_HORIZON*EVENT_HORIZON) {
                            SDL_RenderDrawPoint(renderer, bx + w, by + h);
                        }
                    }
                }
             } else {
                 // SUN
                 // Core
                 SDL_SetRenderDrawColor(renderer, 255, 255, 200, 255);
                 double sun_r = EVENT_HORIZON * 2.0;
                 for(int w=-sun_r; w<=sun_r; w++) {
                     for(int h=-sun_r; h<=sun_r; h++) {
                         if (w*w + h*h <= sun_r*sun_r) {
                             SDL_RenderDrawPoint(renderer, bx + w, by + h);
                         }
                     }
                 }
                 // Glow
                 SDL_SetRenderDrawColor(renderer, 255, 200, 50, 100);
                 draw_rect(renderer, bx - sun_r - 5, by - sun_r - 5, sun_r*2 + 10, sun_r*2 + 10, 255, 200, 50, 50);
             }
         }

        // Particles
        for (int i = 0; i < NUM_PARTICLES; i++) {
            if (particles[i].alive) {
                SDL_SetRenderDrawColor(renderer, particles[i].r, particles[i].g, particles[i].b, 255);
                SDL_RenderDrawPoint(renderer, (int)particles[i].x, (int)particles[i].y);
            }
        }
        
        draw_ui(renderer, current_fps_display);

        if (paused) {
            draw_rect(renderer, WIDTH/2 - 25, HEIGHT/2 - 30, 20, 60, 255, 255, 255, 200);
            draw_rect(renderer, WIDTH/2 + 5,  HEIGHT/2 - 30, 20, 60, 255, 255, 255, 200);
        }

        SDL_RenderPresent(renderer);
        
        frame_count++;
        Uint32 now = SDL_GetTicks();
        if (now - fps_timer > 1000) {
            current_fps_display = frame_count;
            frame_count = 0;
            fps_timer = now;
        }
    }

    free(particles);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
