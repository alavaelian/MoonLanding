#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_font.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

using namespace std;

// --- Estados del Juego ---
enum EstadoJuego { ESTADO_SPLASH, ESTADO_JUEGO, ESTADO_VICTORIA };
EstadoJuego estado_actual = ESTADO_SPLASH;

// --- Recursos Globales Allegro 5 ---
ALLEGRO_DISPLAY      *display       = NULL;
ALLEGRO_EVENT_QUEUE  *event_queue   = NULL;
ALLEGRO_TIMER        *timer         = NULL;
ALLEGRO_FONT         *fuente        = NULL;

ALLEGRO_BITMAP *buffer            = NULL;
ALLEGRO_BITMAP *splash_imagen     = NULL;
ALLEGRO_BITMAP *splash_victoria   = NULL;

ALLEGRO_SAMPLE *musica_fondo      = NULL;
ALLEGRO_SAMPLE *sfx_explosion     = NULL;
ALLEGRO_SAMPLE *sfx_motor         = NULL;
ALLEGRO_SAMPLE *sfx_start         = NULL;
ALLEGRO_SAMPLE *sfx_victoria      = NULL;
ALLEGRO_SAMPLE *sonido_splash     = NULL;

ALLEGRO_SAMPLE_INSTANCE *instancia_motor = NULL;

// --- Variables de Estado del Gameplay ---
float cx = 710, cy = 100;
float vx = 0, vy = 0;
float combustible = 100;
int num_nivel = 1;
bool motor_encendido = false;

int nivel_y = -30;
int nivel_timer = 2000;
int parpadeo_global = 0;
float victoria_nave_x = 370, victoria_nave_y = 500;
int alpha_splash = 0;
int fase_splash = 0; // 0: FadeIn, 1: Espera, 2: FadeOut

// --- Teclado (Simulación de Estado) ---
bool teclas[ALLEGRO_KEY_MAX] = { false };

struct Estrella {
    int x, y;
    int color_r, color_g, color_b;
};
Estrella fondo[100];

#define MAX_ROCAS 15
float roca_x[MAX_ROCAS];
float roca_y[MAX_ROCAS];
float roca_vy[MAX_ROCAS];
bool roca_activa[MAX_ROCAS];
int roca_timer[MAX_ROCAS];
float gravedad_rocas = 0.5f;

// --- Prototipos de Funciones de Física y Lógica original ---
void rotar(float &x, float &y, float cx, float cy, float da);
void pintar_nave (float cx, float cy);
void mover_nave (float &cx, float &cy, float &vx, float &vy);
void aceleracion (float da, float &vx, float &vy );
void pintar_motor (float da, float cx, float cy);
void medidor_combustible(float combustible);
void pintar_nivel(int num_nivel);
void explocion(float cx, float cy, int num_nivel);
b#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_font.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

using namespace std;

// --- Estados del Juego ---
enum EstadoJuego { ESTADO_SPLASH, ESTADO_JUEGO, ESTADO_VICTORIA };
EstadoJuego estado_actual = ESTADO_SPLASH;

// --- Recursos Globales Allegro 5 ---
ALLEGRO_DISPLAY      *display       = NULL;
ALLEGRO_EVENT_QUEUE  *event_queue   = NULL;
ALLEGRO_TIMER        *timer         = NULL;
ALLEGRO_FONT         *fuente        = NULL;

ALLEGRO_BITMAP *buffer            = NULL;
ALLEGRO_BITMAP *splash_imagen     = NULL;
ALLEGRO_BITMAP *splash_victoria   = NULL;

ALLEGRO_SAMPLE *musica_fondo      = NULL;
ALLEGRO_SAMPLE *sfx_explosion     = NULL;
ALLEGRO_SAMPLE *sfx_motor         = NULL;
ALLEGRO_SAMPLE *sfx_start         = NULL;
ALLEGRO_SAMPLE *sfx_victoria      = NULL;
ALLEGRO_SAMPLE *sonido_splash     = NULL;

ALLEGRO_SAMPLE_INSTANCE *instancia_motor = NULL;

// --- Variables de Estado del Gameplay ---
float cx = 710, cy = 100;
float vx = 0, vy = 0;
float combustible = 100;
int num_nivel = 1;
bool motor_encendido = false;

int nivel_y = -30;
int nivel_timer = 2000;
int parpadeo_global = 0;
float victoria_nave_x = 370, victoria_nave_y = 500;
int alpha_splash = 0;
int fase_splash = 0; // 0: FadeIn, 1: Espera, 2: FadeOut

// --- Teclado (Simulación de Estado) ---
bool teclas[ALLEGRO_KEY_MAX] = { false };

struct Estrella {
    int x, y;
    int color_r, color_g, color_b;
};
Estrella fondo[100];

#define MAX_ROCAS 15
float roca_x[MAX_ROCAS];
float roca_y[MAX_ROCAS];
float roca_vy[MAX_ROCAS];
bool roca_activa[MAX_ROCAS];
int roca_timer[MAX_ROCAS];
float gravedad_rocas = 0.5f;

// --- Prototipos de Funciones de Física y Lógica original ---
void rotar(float &x, float &y, float cx, float cy, float da);
void pintar_nave (float cx, float cy);
void mover_nave (float &cx, float &cy, float &vx, float &vy);
void aceleracion (float da, float &vx, float &vy );
void pintar_motor (float da, float cx, float cy);
void medidor_combustible(float combustible);
void pintar_nivel(int num_nivel);
void explocion(float cx, float cy, int num_nivel);
bool game_over(float &cx, float &cy, float &vx, float &vy, int &num_nivel, float &combustible);
bool aterrizar(float cx, float cy, float vx, float vy, int num_nivel);
bool choque_triangulo(float x1, float y1, float x2, float y2, float p1x, float p1y, float p2x, float p2y, string tipo_triangulo);
bool choque_nave(int num_nivel, float cx, float cy );
void avanzar_nivel(float &cx, float &cy, float &vx, float &vy, int &num_nivel , float &combustible);
void reiniciar_nivel(float &cx, float &cy, float &vx, float &vy, float &combustible, int num_nivel);
void dibujar_triangulos();

// --- Funciones de Utilidad de Texto (Estilo Retro-Futurista) ---
void texto_glow(const char *texto, int x, int y, ALLEGRO_COLOR color) {
    ALLEGRO_COLOR g1 = al_map_rgb(0, 60, 60);
    ALLEGRO_COLOR g2 = al_map_rgb(0, 140, 140);
    al_draw_text(fuente, g1, x-2, y, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, g1, x+2, y, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, g1, x, y-2, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, g1, x, y+2, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, g2, x-1, y, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, g2, x+1, y, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, g2, x, y-1, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, g2, x, y+1, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, color, x, y, ALLEGRO_ALIGN_CENTRE, texto);
}

void texto_borde(const char *texto, int x, int y, ALLEGRO_COLOR color) {
    ALLEGRO_COLOR negro = al_map_rgb(0, 0, 0);
    al_draw_text(fuente, negro, x-1, y, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, negro, x+1, y, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, negro, x, y-1, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, negro, x, y+1, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, color, x, y, ALLEGRO_ALIGN_CENTRE, texto);
}

// --- Bucle No Bloqueante Principal (Compatible con WebAssembly) ---
void loop_principal() {
    ALLEGRO_EVENT evento;
    bool redibujar = false;

    while (al_get_next_event(event_queue, &evento)) {
        if (evento.type == ALLEGRO_EVENT_TIMER) {
            redibujar = true;
        } else if (evento.type == ALLEGRO_EVENT_KEY_DOWN) {
            teclas[evento.keyboard.keycode] = true;
        } else if (evento.type == ALLEGRO_EVENT_KEY_UP) {
            teclas[evento.keyboard.keycode] = false;
        }
    }

    if (redibujar && al_is_event_queue_empty(event_queue)) {
        parpadeo_global++;
        
        // --- MANEJO DE LA MÁQUINA DE ESTADOS ---
        if (estado_actual == ESTADO_SPLASH) {
            if (fase_splash == 0) { // Fade in
                alpha_splash += 8;
                if (alpha_splash >= 255) { alpha_splash = 255; fase_splash = 1; }
            } else if (fase_splash == 1) { // Espera Tecla
                // Verifica si alguna tecla estructural fue presionada para pasar
                if (teclas[ALLEGRO_KEY_A] || teclas[ALLEGRO_KEY_ENTER] || teclas[ALLEGRO_KEY_SPACE]) {
                    fase_splash = 2;
                }
            } else if (fase_splash == 2) { // Fade out
                alpha_splash -= 8;
                if (alpha_splash <= 0) {
                    alpha_splash = 0;
                    if (sonido_splash) al_stop_samples();
                    if (sfx_start) al_play_sample(sfx_start, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                    if (musica_fondo) al_play_sample(musica_fondo, 0.6, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, NULL);
                    estado_actual = ESTADO_JUEGO;
                }
            }
        }
        else if (estado_actual == ESTADO_JUEGO) {
            if (teclas[ALLEGRO_KEY_ESCAPE]) return;

            int motores_activos = 0;
            mover_nave(cx, cy, vx, vy);

            if (teclas[ALLEGRO_KEY_UP] && combustible >= 0) {
                aceleracion(0, vx, vy);
                motores_activos++;
            }
            if (teclas[ALLEGRO_KEY_RIGHT] && combustible >= 0) {
                aceleracion(-90, vx, vy);
                motores_activos++;
            }
            if (teclas[ALLEGRO_KEY_LEFT] && combustible >= 0) {
                aceleracion(90, vx, vy);
                motores_activos++;
            }

            if (motores_activos > 0) {
                combustible -= (0.2f * motores_activos);
                if (!motor_encendido && instancia_motor) {
                    al_set_sample_instance_gain(instancia_motor, 0.8);
                    motor_encendido = true;
                }
            } else {
                if (motor_encendido && instancia_motor) {
                    al_set_sample_instance_gain(instancia_motor, 0.0);
                    motor_encendido = false;
                }
            }

            avanzar_nivel(cx, cy, vx, vy, num_nivel, combustible);

            if (game_over(cx, cy, vx, vy, num_nivel, combustible)) {
                reiniciar_nivel(cx, cy, vx, vy, combustible, num_nivel);
            }
        }
        else if (estado_actual == ESTADO_VICTORIA) {
            if (teclas[ALLEGRO_KEY_ESCAPE]) return;
            if (teclas[ALLEGRO_KEY_A]) {
                if (musica_fondo) al_play_sample(musica_fondo, 0.6, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, NULL);
                num_nivel = 1;
                reiniciar_nivel(cx, cy, vx, vy, combustible, num_nivel);
                estado_actual = ESTADO_JUEGO;
            }
            if (victoria_nave_y > 200) victoria_nave_y -= 1.5f;
        }

        // --- SECCIÓN DE RENDERIZADO (DIBUJO) ---
        al_set_target_bitmap(buffer);
        al_clear_to_color(al_map_rgb(0, 0, 0));

        if (estado_actual == ESTADO_SPLASH) {
            if (splash_imagen) {
                al_draw_scaled_bitmap(splash_imagen, 0, 0, al_get_bitmap_width(splash_imagen), al_get_bitmap_height(splash_imagen), 0, 0, 740, 500, 0);
            }
            // Capa de opacidad para simular el Blend tinting de Allegro 4
            al_draw_filled_rectangle(0, 0, 740, 500, al_map_rgba(0, 0, 0, 255 - alpha_splash));
            if (fase_splash == 1 && (parpadeo_global % 80) < 40) {
                al_draw_text(fuente, al_map_rgb(255, 255, 255), 740/2, 500 - 50, ALLEGRO_ALIGN_CENTRE, "PRESS A KEY TO START");
            }
        }
        else if (estado_actual == ESTADO_JUEGO) {
            // Estrellas centelleantes
            for (int i = 0; i < 100; i++) {
                if (rand() % 100 < 5) {
                    int gris = 100 + (rand() % 155);
                    fondo[i].color_r = fondo[i].color_g = fondo[i].color_b = gris;
                }
                al_put_pixel(fondo[i].x, fondo[i].y, al_map_rgb(fondo[i].color_r, fondo[i].color_g, fondo[i].color_b));
            }

            pintar_nivel(num_nivel);

            if (teclas[ALLEGRO_KEY_UP] && combustible >= 0)    pintar_motor(0, cx, cy);
            if (teclas[ALLEGRO_KEY_RIGHT] && combustible >= 0) pintar_motor(-90, cx, cy);
            if (teclas[ALLEGRO_KEY_LEFT] && combustible >= 0)  pintar_motor(90, cx, cy);

            pintar_nave(cx, cy);
            medidor_combustible(combustible);

            // Transición del HUD del Nivel
            if (nivel_timer > 0) {
                if (nivel_y < 25 && nivel_timer > 100) nivel_y += 1;
                else if (nivel_timer < 80) nivel_y -= 1;

                al_draw_filled_rectangle(740/2 - 110, nivel_y - 8, 740/2 + 110, nivel_y + 18, al_map_rgb(0, 0, 0));
                al_draw_rectangle(740/2 - 110, nivel_y - 8, 740/2 + 110, nivel_y + 18, al_map_rgb(255, 255, 255), 1.0);
                al_draw_textf(fuente, al_map_rgb(255, 255, 255), 740/2, nivel_y, ALLEGRO_ALIGN_CENTRE, "MISION - NIVEL %d", num_nivel);

                if (teclas[ALLEGRO_KEY_UP] || teclas[ALLEGRO_KEY_LEFT] || teclas[ALLEGRO_KEY_RIGHT]) nivel_timer--;
            }
        }
        else if (estado_actual == ESTADO_VICTORIA) {
            if (splash_victoria) {
                al_draw_scaled_bitmap(splash_victoria, 0, 0, al_get_bitmap_width(splash_victoria), al_get_bitmap_height(splash_victoria), 0, 0, 740, 500, 0);
            } else {
                al_clear_to_color(al_map_rgb(0, 0, 0));
            }

            pintar_nave(victoria_nave_x, victoria_nave_y);
            if (victoria_nave_y > 200) pintar_motor(0, victoria_nave_x, victoria_nave_y);

            // Marcos estéticos en Cian (Dark Deco style)
            al_draw_rectangle(150, 118, 590, 362, al_map_rgb(0, 100, 100), 1.0);
            al_draw_rectangle(150, 120, 590, 360, al_map_rgb(0, 255, 255), 2.0);
            al_draw_line(160, 168, 580, 168, al_map_rgb(0, 255, 255), 1.0);
            al_draw_line(160, 292, 580, 292, al_map_rgb(0, 255, 255), 1.0);

            texto_glow("** MISION CUMPLIDA **", 370, 145, al_map_rgb(0, 255, 255));
            texto_borde("HAS COMPLETADO TODOS LOS NIVELES", 370, 200, al_map_rgb(255, 255, 255));
            texto_borde("ALUNIZAJE PERFECTO, COMANDANTE", 370, 220, al_map_rgb(0, 255, 128));

            if ((parpadeo_global % 80) < 50) {
                al_draw_text(fuente, al_map_rgb(0, 255, 128), 370, 315, ALLEGRO_ALIGN_CENTRE, "PRESS (A) TO PLAY AGAIN");
                al_draw_text(fuente, al_map_rgb(255, 255, 255), 370, 335, ALLEGRO_ALIGN_CENTRE, "PRESS (ESC) TO EXIT");
            }
        }

        // Volcar el buffer a la pantalla física
        al_set_target_backbuffer(display);
        al_draw_bitmap(buffer, 0, 0, 0);
        al_flip_display();
    }
}

int main() {
    srand(time(NULL));

    if (!al_init()) return -1;
    al_install_keyboard();
    al_init_primitives_addon();
    al_init_image_addon();
    al_install_audio();
    al_init_acodec_addon();
    al_reserve_samples(4);

    display = al_create_display(740, 500);
    event_queue = al_create_event_queue();
    timer = al_create_timer(1.0 / 60.0);
    fuente = al_create_builtin_font();

    buffer = al_create_bitmap(740, 500);

    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_timer_event_source(timer));
    al_register_event_source(event_queue, al_get_keyboard_event_source());

    // Carga de recursos multimedia
    splash_imagen   = al_load_bitmap("./splash_nix.bmp");
    splash_victoria = al_load_bitmap("./victoria_art.bmp");
    
    sonido_splash   = al_load_sample("pdpsong.wav");
    sfx_start       = al_load_sample("./sonidostart.wav");
    sfx_motor       = al_load_sample("./thrust.wav");
    sfx_explosion   = al_load_sample("./boom.wav");
    musica_fondo    = al_load_sample("./gamesong.wav");
    sfx_victoria    = al_load_sample("./victory_fixed.wav");

    if (sfx_motor) {
        instancia_motor = al_create_sample_instance(sfx_motor);
        al_attach_sample_instance_to_mixer(instancia_motor, al_get_default_mixer());
        al_set_sample_instance_playmode(instancia_motor, ALLEGRO_PLAYMODE_LOOP);
        al_set_sample_instance_gain(instancia_motor, 0.0); // Silencioso hasta pulsar tecla
        al_play_sample_instance(instancia_motor);
    }

    if (sonido_splash) {
        al_play_sample(sonido_splash, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, NULL);
    }

    // Inicialización de Estrellas
    for(int i = 0; i < 100; i++) {
        fondo[i].x = rand() % 740;
        fondo[i].y = rand() % 500;
        int gris = 100 + (rand() % 155);
        fondo[i].color_r = fondo[i].color_g = fondo[i].color_b = gris;
    }

    al_start_timer(timer);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(loop_principal, 0, 1);
#else
    while (true) {
        loop_principal();
    }
#endif

    // Limpieza de memoria explícita de Allegro 5
    if (buffer) al_destroy_bitmap(buffer);
    if (splash_imagen) al_destroy_bitmap(splash_imagen);
    if (splash_victoria) al_destroy_bitmap(splash_victoria);
    if (fuente) al_destroy_font(fuente);
    al_destroy_timer(timer);
    al_destroy_event_queue(event_queue);
    al_destroy_display(display);

    return 0;
}

// =====================================================================
// --- IMPLEMENTACIÓN DE TU FÍSICA Y LÓGICA DE NIVELES ORIGINALES ---
// =====================================================================

void rotar(float &x, float &y, float cx, float cy, float da) {
    float radianes = da * M_PI / 180.0;
    float nx = cx + (x - cx) * cos(radianes) - (y - cy) * sin(radianes);
    float ny = cy + (x - cx) * sin(radianes) + (y - cy) * cos(radianes);
    x = nx; y = ny;
}

void pintar_nave(float cx, float cy) {
    // Sustitución de triangle() y rect() por al_draw_filled_triangle() y al_draw_rectangle()
    al_draw_filled_triangle(cx-21, cy+9, cx-18, cy+21, cx-9, cy+21, al_map_rgb(255, 255, 255));
    al_draw_filled_triangle(cx+21, cy+9, cx+18, cy+21, cx+9, cy+21, al_map_rgb(255, 255, 255));
    al_draw_filled_rectangle(cx-11, cy-16, cx+11, cy+1, al_map_rgb(0, 255, 255));
    al_draw_filled_triangle(cx-11, cy-16, cx+11, cy-16, cx, cy-25, al_map_rgb(255, 0, 0));
}

void mover_nave(float &cx, float &cy, float &vx, float &vy) {
    vy += 0.012f; // Gravedad constante lunar
    cx += vx;
    cy += vy;
}

void aceleracion(float da, float &vx, float &vy) {
    float ax = 0, ay = -0.035f;
    rotar(ax, ay, 0, 0, da);
    vx += ax;
    vy += ay;
}

void pintar_motor(float da, float cx, float cy) {
    float x1 = cx - 5, y1 = cy + 5;
    float x2 = cx + 5, y2 = cy + 5;
    float x3 = cx,     y3 = cy + 18;
    rotar(x1, y1, cx, cy, da);
    rotar(x2, y2, cx, cy, da);
    rotar(x3, y3, cx, cy, da);
    al_draw_filled_triangle(x1, y1, x2, y2, x3, y3, al_map_rgb(255, 130, 0));
}

void medidor_combustible(float combustible) {
    al_draw_textf(fuente, al_map_rgb(255, 255, 255), 15, 15, 0, "FUEL: %.1f%%", combustible);
    al_draw_rectangle(15, 30, 115, 45, al_map_rgb(255, 255, 255), 1.0);
    if (combustible > 0) {
        al_draw_filled_rectangle(16, 31, 16 + combustible, 44, al_map_rgb(0, 255, 0));
    }
}

void pintar_nivel(int num_nivel) {
    // Dibujado del terreno usando las primitivas de Allegro 5
    if (num_nivel == 1) {
        al_draw_filled_triangle(110, 100, 300, 500, 0, 500, al_map_rgb(139, 69, 19));
        al_draw_filled_rectangle(300, 480, 420, 500, al_map_rgb(0, 255, 0)); // Plataforma verde
    }
    else if (num_nivel == 2) {
        al_draw_filled_triangle(110, 100, 300, 500, 0, 500, al_map_rgb(120, 120, 120));
        al_draw_filled_triangle(500, 500, 600, 300, 740, 500, al_map_rgb(120, 120, 120));
        al_draw_filled_rectangle(420, 480, 500, 500, al_map_rgb(0, 255, 0));
    }
    // Agrega el resto de tus niveles replicando los al_draw_filled_triangle según tus coordenadas...
    dibujar_triangulos(); // Rocas cayendo de tus niveles avanzados
}

void avanzar_nivel(float &cx, float &cy, float &vx, float &vy, int &num_nivel, float &combustible) {
    if (aterrizar(cx, cy, vx, vy, num_nivel)) {
        if (num_nivel == 8) {
            estado_actual = ESTADO_VICTORIA;
            if (musica_fondo) al_stop_samples();
            if (sfx_victoria) al_play_sample(sfx_victoria, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
            return;
        }
        num_nivel++;
        reiniciar_nivel(cx, cy, vx, vy, combustible, num_nivel);
        nivel_y = -30;
        nivel_timer = 2000;
    }
}

void reiniciar_nivel(float &cx, float &cy, float &vx, float &vy, float &combustible, int num_nivel) {
    combustible = 100;
    vx = 0; vy = -0.05f;
    cx = 710; cy = 100;
    if (num_nivel == 4) { cy = 80; vy = 0; }
}

bool aterrizar(float cx, float cy, float vx, float vy, int num_nivel) {
    // Tu lógica exacta de colisión con la plataforma verde
    if (num_nivel == 1 && cx >= 300 && cx <= 420 && cy >= 460 && vy < 0.15f) return true;
    if (num_nivel == 2 && cx >= 420 && cx <= 500 && cy >= 460 && vy < 0.15f) return true;
    return false;
}

bool game_over(float &cx, float &cy, float &vx, float &vy, int &num_nivel, float &combustible) {
    if (cy > 500 || cx < 0 || cx > 740) {
        if (sfx_explosion) al_play_sample(sfx_explosion, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
        return true;
    }
    return false;
}

void dibujar_triangulos() {
    // Simulación del comportamiento de meteoritos/rocas cayendo
    for (int i = 0; i < MAX_ROCAS; i++) {
        if (roca_activa[i]) {
            roca_y[i] += roca_vy[i];
            if (roca_y[i] > 500) roca_activa[i] = false;
            al_draw_filled_triangle(roca_x[i]-12, roca_y[i]-6, roca_x[i]+12, roca_y[i]-6, roca_x[i], roca_y[i]+12, al_map_rgb(153,153,153));
        } else {
            roca_timer[i]--;
            if (roca_timer[i] <= 0) {
                roca_x[i] = 100 + rand() % 500;
                roca_y[i] = 30;
                roca_vy[i] = 1.5f + (rand() % 4);
                roca_activa[i] = true;
                roca_timer[i] = 120;
            }
        }
    }
}ool game_over(float &cx, float &cy, float &vx, float &vy, int &num_nivel, float &combustible);
bool aterrizar(float cx, float cy, float vx, float vy, int num_nivel);
bool choque_triangulo(float x1, float y1, float x2, float y2, float p1x, float p1y, float p2x, float p2y, string tipo_triangulo);
bool choque_nave(int num_nivel, float cx, float cy );
void avanzar_nivel(float &cx, float &cy, float &vx, float &vy, int &num_nivel , float &combustible);
void reiniciar_nivel(float &cx, float &cy, float &vx, float &vy, float &combustible, int num_nivel);
void dibujar_triangulos();

// --- Funciones de Utilidad de Texto (Estilo Retro-Futurista) ---
void texto_glow(const char *texto, int x, int y, ALLEGRO_COLOR color) {
    ALLEGRO_COLOR g1 = al_map_rgb(0, 60, 60);
    ALLEGRO_COLOR g2 = al_map_rgb(0, 140, 140);
    al_draw_text(fuente, g1, x-2, y, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, g1, x+2, y, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, g1, x, y-2, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, g1, x, y+2, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, g2, x-1, y, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, g2, x+1, y, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, g2, x, y-1, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, g2, x, y+1, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, color, x, y, ALLEGRO_ALIGN_CENTRE, texto);
}

void texto_borde(const char *texto, int x, int y, ALLEGRO_COLOR color) {
    ALLEGRO_COLOR negro = al_map_rgb(0, 0, 0);
    al_draw_text(fuente, negro, x-1, y, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, negro, x+1, y, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, negro, x, y-1, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, negro, x, y+1, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, color, x, y, ALLEGRO_ALIGN_CENTRE, texto);
}

// --- Bucle No Bloqueante Principal (Compatible con WebAssembly) ---
void loop_principal() {
    ALLEGRO_EVENT evento;
    bool redibujar = false;

    while (al_get_next_event(event_queue, &evento)) {
        if (evento.type == ALLEGRO_EVENT_TIMER) {
            redibujar = true;
        } else if (evento.type == ALLEGRO_EVENT_KEY_DOWN) {
            teclas[evento.keyboard.keycode] = true;
        } else if (evento.type == ALLEGRO_EVENT_KEY_UP) {
            teclas[evento.keyboard.keycode] = false;
        }
    }

    if (redibujar && al_is_event_queue_empty(event_queue)) {
        parpadeo_global++;
        
        // --- MANEJO DE LA MÁQUINA DE ESTADOS ---
        if (estado_actual == ESTADO_SPLASH) {
            if (fase_splash == 0) { // Fade in
                alpha_splash += 8;
                if (alpha_splash >= 255) { alpha_splash = 255; fase_splash = 1; }
            } else if (fase_splash == 1) { // Espera Tecla
                // Verifica si alguna tecla estructural fue presionada para pasar
                if (teclas[ALLEGRO_KEY_A] || teclas[ALLEGRO_KEY_ENTER] || teclas[ALLEGRO_KEY_SPACE]) {
                    fase_splash = 2;
                }
            } else if (fase_splash == 2) { // Fade out
                alpha_splash -= 8;
                if (alpha_splash <= 0) {
                    alpha_splash = 0;
                    if (sonido_splash) al_stop_samples();
                    if (sfx_start) al_play_sample(sfx_start, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                    if (musica_fondo) al_play_sample(musica_fondo, 0.6, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, NULL);
                    estado_actual = ESTADO_JUEGO;
                }
            }
        }
        else if (estado_actual == ESTADO_JUEGO) {
            if (teclas[ALLEGRO_KEY_ESCAPE]) return;

            int motores_activos = 0;
            mover_nave(cx, cy, vx, vy);

            if (teclas[ALLEGRO_KEY_UP] && combustible >= 0) {
                aceleracion(0, vx, vy);
                motores_activos++;
            }
            if (teclas[ALLEGRO_KEY_RIGHT] && combustible >= 0) {
                aceleracion(-90, vx, vy);
                motores_activos++;
            }
            if (teclas[ALLEGRO_KEY_LEFT] && combustible >= 0) {
                aceleracion(90, vx, vy);
                motores_activos++;
            }

            if (motores_activos > 0) {
                combustible -= (0.2f * motores_activos);
                if (!motor_encendido && instancia_motor) {
                    al_set_sample_instance_gain(instancia_motor, 0.8);
                    motor_encendido = true;
                }
            } else {
                if (motor_encendido && instancia_motor) {
                    al_set_sample_instance_gain(instancia_motor, 0.0);
                    motor_encendido = false;
                }
            }

            avanzar_nivel(cx, cy, vx, vy, num_nivel, combustible);

            if (game_over(cx, cy, vx, vy, num_nivel, combustible)) {
                reiniciar_nivel(cx, cy, vx, vy, combustible, num_nivel);
            }
        }
        else if (estado_actual == ESTADO_VICTORIA) {
            if (teclas[ALLEGRO_KEY_ESCAPE]) return;
            if (teclas[ALLEGRO_KEY_A]) {
                if (musica_fondo) al_play_sample(musica_fondo, 0.6, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, NULL);
                num_nivel = 1;
                reiniciar_nivel(cx, cy, vx, vy, combustible, num_nivel);
                estado_actual = ESTADO_JUEGO;
            }
            if (victoria_nave_y > 200) victoria_nave_y -= 1.5f;
        }

        // --- SECCIÓN DE RENDERIZADO (DIBUJO) ---
        al_set_target_bitmap(buffer);
        al_clear_to_color(al_map_rgb(0, 0, 0));

        if (estado_actual == ESTADO_SPLASH) {
            if (splash_imagen) {
                al_draw_scaled_bitmap(splash_imagen, 0, 0, al_get_bitmap_width(splash_imagen), al_get_bitmap_height(splash_imagen), 0, 0, 740, 500, 0);
            }
            // Capa de opacidad para simular el Blend tinting de Allegro 4
            al_draw_filled_rectangle(0, 0, 740, 500, al_map_rgba(0, 0, 0, 255 - alpha_splash));
            if (fase_splash == 1 && (parpadeo_global % 80) < 40) {
                al_draw_text(fuente, al_map_rgb(255, 255, 255), 740/2, 500 - 50, ALLEGRO_ALIGN_CENTRE, "PRESS A KEY TO START");
            }
 #include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_font.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

using namespace std;

// --- Estados del Juego ---
enum EstadoJuego { ESTADO_SPLASH, ESTADO_JUEGO, ESTADO_VICTORIA };
EstadoJuego estado_actual = ESTADO_SPLASH;

// --- Recursos Globales Allegro 5 ---
ALLEGRO_DISPLAY      *display       = NULL;
ALLEGRO_EVENT_QUEUE  *event_queue   = NULL;
ALLEGRO_TIMER        *timer         = NULL;
ALLEGRO_FONT         *fuente        = NULL;

ALLEGRO_BITMAP *buffer            = NULL;
ALLEGRO_BITMAP *splash_imagen     = NULL;
ALLEGRO_BITMAP *splash_victoria   = NULL;

ALLEGRO_SAMPLE *musica_fondo      = NULL;
ALLEGRO_SAMPLE *sfx_explosion     = NULL;
ALLEGRO_SAMPLE *sfx_motor         = NULL;
ALLEGRO_SAMPLE *sfx_start         = NULL;
ALLEGRO_SAMPLE *sfx_victoria      = NULL;
ALLEGRO_SAMPLE *sonido_splash     = NULL;

ALLEGRO_SAMPLE_INSTANCE *instancia_motor = NULL;

// --- Variables de Estado del Gameplay ---
float cx = 710, cy = 100;
float vx = 0, vy = 0;
float combustible = 100;
int num_nivel = 1;
bool motor_encendido = false;

int nivel_y = -30;
int nivel_timer = 2000;
int parpadeo_global = 0;
float victoria_nave_x = 370, victoria_nave_y = 500;
int alpha_splash = 0;
int fase_splash = 0; // 0: FadeIn, 1: Espera, 2: FadeOut

// --- Teclado (Simulación de Estado) ---
bool teclas[ALLEGRO_KEY_MAX] = { false };

struct Estrella {
    int x, y;
    int color_r, color_g, color_b;
};
Estrella fondo[100];

#define MAX_ROCAS 15
float roca_x[MAX_ROCAS];
float roca_y[MAX_ROCAS];
float roca_vy[MAX_ROCAS];
bool roca_activa[MAX_ROCAS];
int roca_timer[MAX_ROCAS];
float gravedad_rocas = 0.5f;

// --- Prototipos de Funciones de Física y Lógica original ---
void rotar(float &x, float &y, float cx, float cy, float da);
void pintar_nave (float cx, float cy);
void mover_nave (float &cx, float &cy, float &vx, float &vy);
void aceleracion (float da, float &vx, float &vy );
void pintar_motor (float da, float cx, float cy);
void medidor_combustible(float combustible);
void pintar_nivel(int num_nivel);
void explocion(float cx, float cy, int num_nivel);
bool game_over(float &cx, float &cy, float &vx, float &vy, int &num_nivel, float &combustible);
bool aterrizar(float cx, float cy, float vx, float vy, int num_nivel);
bool choque_triangulo(float x1, float y1, float x2, float y2, float p1x, float p1y, float p2x, float p2y, string tipo_triangulo);
bool choque_nave(int num_nivel, float cx, float cy );
void avanzar_nivel(float &cx, float &cy, float &vx, float &vy, int &num_nivel , float &combustible);
void reiniciar_nivel(float &cx, float &cy, float &vx, float &vy, float &combustible, int num_nivel);
void dibujar_triangulos();

// --- Funciones de Utilidad de Texto (Estilo Retro-Futurista) ---
void texto_glow(const char *texto, int x, int y, ALLEGRO_COLOR color) {
    ALLEGRO_COLOR g1 = al_map_rgb(0, 60, 60);
    ALLEGRO_COLOR g2 = al_map_rgb(0, 140, 140);
    al_draw_text(fuente, g1, x-2, y, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, g1, x+2, y, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, g1, x, y-2, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, g1, x, y+2, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, g2, x-1, y, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, g2, x+1, y, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, g2, x, y-1, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, g2, x, y+1, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, color, x, y, ALLEGRO_ALIGN_CENTRE, texto);
}

void texto_borde(const char *texto, int x, int y, ALLEGRO_COLOR color) {
    ALLEGRO_COLOR negro = al_map_rgb(0, 0, 0);
    al_draw_text(fuente, negro, x-1, y, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, negro, x+1, y, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, negro, x, y-1, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, negro, x, y+1, ALLEGRO_ALIGN_CENTRE, texto);
    al_draw_text(fuente, color, x, y, ALLEGRO_ALIGN_CENTRE, texto);
}

// --- Bucle No Bloqueante Principal (Compatible con WebAssembly) ---
void loop_principal() {
    ALLEGRO_EVENT evento;
    bool redibujar = false;

    while (al_get_next_event(event_queue, &evento)) {
        if (evento.type == ALLEGRO_EVENT_TIMER) {
            redibujar = true;
        } else if (evento.type == ALLEGRO_EVENT_KEY_DOWN) {
            teclas[evento.keyboard.keycode] = true;
        } else if (evento.type == ALLEGRO_EVENT_KEY_UP) {
            teclas[evento.keyboard.keycode] = false;
        }
    }

    if (redibujar && al_is_event_queue_empty(event_queue)) {
        parpadeo_global++;
        
        // --- MANEJO DE LA MÁQUINA DE ESTADOS ---
        if (estado_actual == ESTADO_SPLASH) {
            if (fase_splash == 0) { // Fade in
                alpha_splash += 8;
                if (alpha_splash >= 255) { alpha_splash = 255; fase_splash = 1; }
            } else if (fase_splash == 1) { // Espera Tecla
                // Verifica si alguna tecla estructural fue presionada para pasar
                if (teclas[ALLEGRO_KEY_A] || teclas[ALLEGRO_KEY_ENTER] || teclas[ALLEGRO_KEY_SPACE]) {
                    fase_splash = 2;
                }
            } else if (fase_splash == 2) { // Fade out
                alpha_splash -= 8;
                if (alpha_splash <= 0) {
                    alpha_splash = 0;
                    if (sonido_splash) al_stop_samples();
                    if (sfx_start) al_play_sample(sfx_start, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                    if (musica_fondo) al_play_sample(musica_fondo, 0.6, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, NULL);
                    estado_actual = ESTADO_JUEGO;
                }
            }
        }
        else if (estado_actual == ESTADO_JUEGO) {
            if (teclas[ALLEGRO_KEY_ESCAPE]) return;

            int motores_activos = 0;
            mover_nave(cx, cy, vx, vy);

            if (teclas[ALLEGRO_KEY_UP] && combustible >= 0) {
                aceleracion(0, vx, vy);
                motores_activos++;
            }
            if (teclas[ALLEGRO_KEY_RIGHT] && combustible >= 0) {
                aceleracion(-90, vx, vy);
                motores_activos++;
            }
            if (teclas[ALLEGRO_KEY_LEFT] && combustible >= 0) {
                aceleracion(90, vx, vy);
                motores_activos++;
            }

            if (motores_activos > 0) {
                combustible -= (0.2f * motores_activos);
                if (!motor_encendido && instancia_motor) {
                    al_set_sample_instance_gain(instancia_motor, 0.8);
                    motor_encendido = true;
                }
            } else {
                if (motor_encendido && instancia_motor) {
                    al_set_sample_instance_gain(instancia_motor, 0.0);
                    motor_encendido = false;
                }
            }

            avanzar_nivel(cx, cy, vx, vy, num_nivel, combustible);

            if (game_over(cx, cy, vx, vy, num_nivel, combustible)) {
                reiniciar_nivel(cx, cy, vx, vy, combustible, num_nivel);
            }
        }
        else if (estado_actual == ESTADO_VICTORIA) {
            if (teclas[ALLEGRO_KEY_ESCAPE]) return;
            if (teclas[ALLEGRO_KEY_A]) {
                if (musica_fondo) al_play_sample(musica_fondo, 0.6, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, NULL);
                num_nivel = 1;
                reiniciar_nivel(cx, cy, vx, vy, combustible, num_nivel);
                estado_actual = ESTADO_JUEGO;
            }
            if (victoria_nave_y > 200) victoria_nave_y -= 1.5f;
        }

        // --- SECCIÓN DE RENDERIZADO (DIBUJO) ---
        al_set_target_bitmap(buffer);
        al_clear_to_color(al_map_rgb(0, 0, 0));

        if (estado_actual == ESTADO_SPLASH) {
            if (splash_imagen) {
                al_draw_scaled_bitmap(splash_imagen, 0, 0, al_get_bitmap_width(splash_imagen), al_get_bitmap_height(splash_imagen), 0, 0, 740, 500, 0);
            }
            // Capa de opacidad para simular el Blend tinting de Allegro 4
            al_draw_filled_rectangle(0, 0, 740, 500, al_map_rgba(0, 0, 0, 255 - alpha_splash));
            if (fase_splash == 1 && (parpadeo_global % 80) < 40) {
                al_draw_text(fuente, al_map_rgb(255, 255, 255), 740/2, 500 - 50, ALLEGRO_ALIGN_CENTRE, "PRESS A KEY TO START");
            }
        }
        else if (estado_actual == ESTADO_JUEGO) {
            // Estrellas centelleantes
            for (int i = 0; i < 100; i++) {
                if (rand() % 100 < 5) {
                    int gris = 100 + (rand() % 155);
                    fondo[i].color_r = fondo[i].color_g = fondo[i].color_b = gris;
                }
                al_put_pixel(fondo[i].x, fondo[i].y, al_map_rgb(fondo[i].color_r, fondo[i].color_g, fondo[i].color_b));
            }

            pintar_nivel(num_nivel);

            if (teclas[ALLEGRO_KEY_UP] && combustible >= 0)    pintar_motor(0, cx, cy);
            if (teclas[ALLEGRO_KEY_RIGHT] && combustible >= 0) pintar_motor(-90, cx, cy);
            if (teclas[ALLEGRO_KEY_LEFT] && combustible >= 0)  pintar_motor(90, cx, cy);

            pintar_nave(cx, cy);
            medidor_combustible(combustible);

            // Transición del HUD del Nivel
            if (nivel_timer > 0) {
                if (nivel_y < 25 && nivel_timer > 100) nivel_y += 1;
                else if (nivel_timer < 80) nivel_y -= 1;

                al_draw_filled_rectangle(740/2 - 110, nivel_y - 8, 740/2 + 110, nivel_y + 18, al_map_rgb(0, 0, 0));
                al_draw_rectangle(740/2 - 110, nivel_y - 8, 740/2 + 110, nivel_y + 18, al_map_rgb(255, 255, 255), 1.0);
                al_draw_textf(fuente, al_map_rgb(255, 255, 255), 740/2, nivel_y, ALLEGRO_ALIGN_CENTRE, "MISION - NIVEL %d", num_nivel);

                if (teclas[ALLEGRO_KEY_UP] || teclas[ALLEGRO_KEY_LEFT] || teclas[ALLEGRO_KEY_RIGHT]) nivel_timer--;
            }
        }
        else if (estado_actual == ESTADO_VICTORIA) {
            if (splash_victoria) {
                al_draw_scaled_bitmap(splash_victoria, 0, 0, al_get_bitmap_width(splash_victoria), al_get_bitmap_height(splash_victoria), 0, 0, 740, 500, 0);
            } else {
                al_clear_to_color(al_map_rgb(0, 0, 0));
            }

            pintar_nave(victoria_nave_x, victoria_nave_y);
            if (victoria_nave_y > 200) pintar_motor(0, victoria_nave_x, victoria_nave_y);

            // Marcos estéticos en Cian (Dark Deco style)
            al_draw_rectangle(150, 118, 590, 362, al_map_rgb(0, 100, 100), 1.0);
            al_draw_rectangle(150, 120, 590, 360, al_map_rgb(0, 255, 255), 2.0);
            al_draw_line(160, 168, 580, 168, al_map_rgb(0, 255, 255), 1.0);
            al_draw_line(160, 292, 580, 292, al_map_rgb(0, 255, 255), 1.0);

            texto_glow("** MISION CUMPLIDA **", 370, 145, al_map_rgb(0, 255, 255));
            texto_borde("HAS COMPLETADO TODOS LOS NIVELES", 370, 200, al_map_rgb(255, 255, 255));
            texto_borde("ALUNIZAJE PERFECTO, COMANDANTE", 370, 220, al_map_rgb(0, 255, 128));

            if ((parpadeo_global % 80) < 50) {
                al_draw_text(fuente, al_map_rgb(0, 255, 128), 370, 315, ALLEGRO_ALIGN_CENTRE, "PRESS (A) TO PLAY AGAIN");
                al_draw_text(fuente, al_map_rgb(255, 255, 255), 370, 335, ALLEGRO_ALIGN_CENTRE, "PRESS (ESC) TO EXIT");
            }
        }

        // Volcar el buffer a la pantalla física
        al_set_target_backbuffer(display);
        al_draw_bitmap(buffer, 0, 0, 0);
        al_flip_display();
    }
}

int main() {
    srand(time(NULL));

    if (!al_init()) return -1;
    al_install_keyboard();
    al_init_primitives_addon();
    al_init_image_addon();
    al_install_audio();
    al_init_acodec_addon();
    al_reserve_samples(4);

    display = al_create_display(740, 500);
    event_queue = al_create_event_queue();
    timer = al_create_timer(1.0 / 60.0);
    fuente = al_create_builtin_font();

    buffer = al_create_bitmap(740, 500);

    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_timer_event_source(timer));
    al_register_event_source(event_queue, al_get_keyboard_event_source());

    // Carga de recursos multimedia
    splash_imagen   = al_load_bitmap("./splash_nix.bmp");
    splash_victoria = al_load_bitmap("./victoria_art.bmp");
    
    sonido_splash   = al_load_sample("pdpsong.wav");
    sfx_start       = al_load_sample("./sonidostart.wav");
    sfx_motor       = al_load_sample("./thrust.wav");
    sfx_explosion   = al_load_sample("./boom.wav");
    musica_fondo    = al_load_sample("./gamesong.wav");
    sfx_victoria    = al_load_sample("./victory_fixed.wav");

    if (sfx_motor) {
        instancia_motor = al_create_sample_instance(sfx_motor);
        al_attach_sample_instance_to_mixer(instancia_motor, al_get_default_mixer());
        al_set_sample_instance_playmode(instancia_motor, ALLEGRO_PLAYMODE_LOOP);
        al_set_sample_instance_gain(instancia_motor, 0.0); // Silencioso hasta pulsar tecla
        al_play_sample_instance(instancia_motor);
    }

    if (sonido_splash) {
        al_play_sample(sonido_splash, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, NULL);
    }

    // Inicialización de Estrellas
    for(int i = 0; i < 100; i++) {
        fondo[i].x = rand() % 740;
        fondo[i].y = rand() % 500;
        int gris = 100 + (rand() % 155);
        fondo[i].color_r = fondo[i].color_g = fondo[i].color_b = gris;
    }

    al_start_timer(timer);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(loop_principal, 0, 1);
#else
    while (true) {
        loop_principal();
    }
#endif

    // Limpieza de memoria explícita de Allegro 5
    if (buffer) al_destroy_bitmap(buffer);
    if (splash_imagen) al_destroy_bitmap(splash_imagen);
    if (splash_victoria) al_destroy_bitmap(splash_victoria);
    if (fuente) al_destroy_font(fuente);
    al_destroy_timer(timer);
    al_destroy_event_queue(event_queue);
    al_destroy_display(display);

    return 0;
}

// =====================================================================
// --- IMPLEMENTACIÓN DE TU FÍSICA Y LÓGICA DE NIVELES ORIGINALES ---
// =====================================================================

void rotar(float &x, float &y, float cx, float cy, float da) {
    float radianes = da * M_PI / 180.0;
    float nx = cx + (x - cx) * cos(radianes) - (y - cy) * sin(radianes);
    float ny = cy + (x - cx) * sin(radianes) + (y - cy) * cos(radianes);
    x = nx; y = ny;
}

void pintar_nave(float cx, float cy) {
    // Sustitución de triangle() y rect() por al_draw_filled_triangle() y al_draw_rectangle()
    al_draw_filled_triangle(cx-21, cy+9, cx-18, cy+21, cx-9, cy+21, al_map_rgb(255, 255, 255));
    al_draw_filled_triangle(cx+21, cy+9, cx+18, cy+21, cx+9, cy+21, al_map_rgb(255, 255, 255));
    al_draw_filled_rectangle(cx-11, cy-16, cx+11, cy+1, al_map_rgb(0, 255, 255));
    al_draw_filled_triangle(cx-11, cy-16, cx+11, cy-16, cx, cy-25, al_map_rgb(255, 0, 0));
}

void mover_nave(float &cx, float &cy, float &vx, float &vy) {
    vy += 0.012f; // Gravedad constante lunar
    cx += vx;
    cy += vy;
}

void aceleracion(float da, float &vx, float &vy) {
    float ax = 0, ay = -0.035f;
    rotar(ax, ay, 0, 0, da);
    vx += ax;
    vy += ay;
}

void pintar_motor(float da, float cx, float cy) {
    float x1 = cx - 5, y1 = cy + 5;
    float x2 = cx + 5, y2 = cy + 5;
    float x3 = cx,     y3 = cy + 18;
    rotar(x1, y1, cx, cy, da);
    rotar(x2, y2, cx, cy, da);
    rotar(x3, y3, cx, cy, da);
    al_draw_filled_triangle(x1, y1, x2, y2, x3, y3, al_map_rgb(255, 130, 0));
}

void medidor_combustible(float combustible) {
    al_draw_textf(fuente, al_map_rgb(255, 255, 255), 15, 15, 0, "FUEL: %.1f%%", combustible);
    al_draw_rectangle(15, 30, 115, 45, al_map_rgb(255, 255, 255), 1.0);
    if (combustible > 0) {
        al_draw_filled_rectangle(16, 31, 16 + combustible, 44, al_map_rgb(0, 255, 0));
    }
}

void pintar_nivel(int num_nivel) {
    // Dibujado del terreno usando las primitivas de Allegro 5
    if (num_nivel == 1) {
        al_draw_filled_triangle(110, 100, 300, 500, 0, 500, al_map_rgb(139, 69, 19));
        al_draw_filled_rectangle(300, 480, 420, 500, al_map_rgb(0, 255, 0)); // Plataforma verde
    }
    else if (num_nivel == 2) {
        al_draw_filled_triangle(110, 100, 300, 500, 0, 500, al_map_rgb(120, 120, 120));
        al_draw_filled_triangle(500, 500, 600, 300, 740, 500, al_map_rgb(120, 120, 120));
        al_draw_filled_rectangle(420, 480, 500, 500, al_map_rgb(0, 255, 0));
    }
    // Agrega el resto de tus niveles replicando los al_draw_filled_triangle según tus coordenadas...
    dibujar_triangulos(); // Rocas cayendo de tus niveles avanzados
}

void avanzar_nivel(float &cx, float &cy, float &vx, float &vy, int &num_nivel, float &combustible) {
    if (aterrizar(cx, cy, vx, vy, num_nivel)) {
        if (num_nivel == 8) {
            estado_actual = ESTADO_VICTORIA;
            if (musica_fondo) al_stop_samples();
            if (sfx_victoria) al_play_sample(sfx_victoria, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
            return;
        }
        num_nivel++;
        reiniciar_nivel(cx, cy, vx, vy, combustible, num_nivel);
        nivel_y = -30;
        nivel_timer = 2000;
    }
}

void reiniciar_nivel(float &cx, float &cy, float &vx, float &vy, float &combustible, int num_nivel) {
    combustible = 100;
    vx = 0; vy = -0.05f;
    cx = 710; cy = 100;
    if (num_nivel == 4) { cy = 80; vy = 0; }
}

bool aterrizar(float cx, float cy, float vx, float vy, int num_nivel) {
    // Tu lógica exacta de colisión con la plataforma verde
    if (num_nivel == 1 && cx >= 300 && cx <= 420 && cy >= 460 && vy < 0.15f) return true;
    if (num_nivel == 2 && cx >= 420 && cx <= 500 && cy >= 460 && vy < 0.15f) return true;
    return false;
}

bool game_over(float &cx, float &cy, float &vx, float &vy, int &num_nivel, float &combustible) {
    if (cy > 500 || cx < 0 || cx > 740) {
        if (sfx_explosion) al_play_sample(sfx_explosion, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
        return true;
    }
    return false;
}

void dibujar_triangulos() {
    // Simulación del comportamiento de meteoritos/rocas cayendo
    for (int i = 0; i < MAX_ROCAS; i++) {
        if (roca_activa[i]) {
            roca_y[i] += roca_vy[i];
            if (roca_y[i] > 500) roca_activa[i] = false;
            al_draw_filled_triangle(roca_x[i]-12, roca_y[i]-6, roca_x[i]+12, roca_y[i]-6, roca_x[i], roca_y[i]+12, al_map_rgb(153,153,153));
        } else {
            roca_timer[i]--;
            if (roca_timer[i] <= 0) {
                roca_x[i] = 100 + rand() % 500;
                roca_y[i] = 30;
                roca_vy[i] = 1.5f + (rand() % 4);
                roca_activa[i] = true;
                roca_timer[i] = 120;
            }
        }
    }
}       }
        else if (estado_actual == ESTADO_JUEGO) {
            // Estrellas centelleantes
            for (int i = 0; i < 100; i++) {
                if (rand() % 100 < 5) {
                    int gris = 100 + (rand() % 155);
                    fondo[i].color_r = fondo[i].color_g = fondo[i].color_b = gris;
                }
                al_put_pixel(fondo[i].x, fondo[i].y, al_map_rgb(fondo[i].color_r, fondo[i].color_g, fondo[i].color_b));
            }

            pintar_nivel(num_nivel);

            if (teclas[ALLEGRO_KEY_UP] && combustible >= 0)    pintar_motor(0, cx, cy);
            if (teclas[ALLEGRO_KEY_RIGHT] && combustible >= 0) pintar_motor(-90, cx, cy);
            if (teclas[ALLEGRO_KEY_LEFT] && combustible >= 0)  pintar_motor(90, cx, cy);

            pintar_nave(cx, cy);
            medidor_combustible(combustible);

            // Transición del HUD del Nivel
            if (nivel_timer > 0) {
                if (nivel_y < 25 && nivel_timer > 100) nivel_y += 1;
                else if (nivel_timer < 80) nivel_y -= 1;

                al_draw_filled_rectangle(740/2 - 110, nivel_y - 8, 740/2 + 110, nivel_y + 18, al_map_rgb(0, 0, 0));
                al_draw_rectangle(740/2 - 110, nivel_y - 8, 740/2 + 110, nivel_y + 18, al_map_rgb(255, 255, 255), 1.0);
                al_draw_textf(fuente, al_map_rgb(255, 255, 255), 740/2, nivel_y, ALLEGRO_ALIGN_CENTRE, "MISION - NIVEL %d", num_nivel);

                if (teclas[ALLEGRO_KEY_UP] || teclas[ALLEGRO_KEY_LEFT] || teclas[ALLEGRO_KEY_RIGHT]) nivel_timer--;
            }
        }
        else if (estado_actual == ESTADO_VICTORIA) {
            if (splash_victoria) {
                al_draw_scaled_bitmap(splash_victoria, 0, 0, al_get_bitmap_width(splash_victoria), al_get_bitmap_height(splash_victoria), 0, 0, 740, 500, 0);
            } else {
                al_clear_to_color(al_map_rgb(0, 0, 0));
            }

            pintar_nave(victoria_nave_x, victoria_nave_y);
            if (victoria_nave_y > 200) pintar_motor(0, victoria_nave_x, victoria_nave_y);

            // Marcos estéticos en Cian (Dark Deco style)
            al_draw_rectangle(150, 118, 590, 362, al_map_rgb(0, 100, 100), 1.0);
            al_draw_rectangle(150, 120, 590, 360, al_map_rgb(0, 255, 255), 2.0);
            al_draw_line(160, 168, 580, 168, al_map_rgb(0, 255, 255), 1.0);
            al_draw_line(160, 292, 580, 292, al_map_rgb(0, 255, 255), 1.0);

            texto_glow("** MISION CUMPLIDA **", 370, 145, al_map_rgb(0, 255, 255));
            texto_borde("HAS COMPLETADO TODOS LOS NIVELES", 370, 200, al_map_rgb(255, 255, 255));
            texto_borde("ALUNIZAJE PERFECTO, COMANDANTE", 370, 220, al_map_rgb(0, 255, 128));

            if ((parpadeo_global % 80) < 50) {
                al_draw_text(fuente, al_map_rgb(0, 255, 128), 370, 315, ALLEGRO_ALIGN_CENTRE, "PRESS (A) TO PLAY AGAIN");
                al_draw_text(fuente, al_map_rgb(255, 255, 255), 370, 335, ALLEGRO_ALIGN_CENTRE, "PRESS (ESC) TO EXIT");
            }
        }

        // Volcar el buffer a la pantalla física
        al_set_target_backbuffer(display);
        al_draw_bitmap(buffer, 0, 0, 0);
        al_flip_display();
    }
}

int main() {
    srand(time(NULL));

    if (!al_init()) return -1;
    al_install_keyboard();
    al_init_primitives_addon();
    al_init_image_addon();
    al_install_audio();
    al_init_acodec_addon();
    al_reserve_samples(4);

    display = al_create_display(740, 500);
    event_queue = al_create_event_queue();
    timer = al_create_timer(1.0 / 60.0);
    fuente = al_create_builtin_font();

    buffer = al_create_bitmap(740, 500);

    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_timer_event_source(timer));
    al_register_event_source(event_queue, al_get_keyboard_event_source());

    // Carga de recursos multimedia
    splash_imagen   = al_load_bitmap("./splash_nix.bmp");
    splash_victoria = al_load_bitmap("./victoria_art.bmp");
    
    sonido_splash   = al_load_sample("pdpsong.wav");
    sfx_start       = al_load_sample("./sonidostart.wav");
    sfx_motor       = al_load_sample("./thrust.wav");
    sfx_explosion   = al_load_sample("./boom.wav");
    musica_fondo    = al_load_sample("./gamesong.wav");
    sfx_victoria    = al_load_sample("./victory_fixed.wav");

    if (sfx_motor) {
        instancia_motor = al_create_sample_instance(sfx_motor);
        al_attach_sample_instance_to_mixer(instancia_motor, al_get_default_mixer());
        al_set_sample_instance_playmode(instancia_motor, ALLEGRO_PLAYMODE_LOOP);
        al_set_sample_instance_gain(instancia_motor, 0.0); // Silencioso hasta pulsar tecla
        al_play_sample_instance(instancia_motor);
    }

    if (sonido_splash) {
        al_play_sample(sonido_splash, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, NULL);
    }

    // Inicialización de Estrellas
    for(int i = 0; i < 100; i++) {
        fondo[i].x = rand() % 740;
        fondo[i].y = rand() % 500;
        int gris = 100 + (rand() % 155);
        fondo[i].color_r = fondo[i].color_g = fondo[i].color_b = gris;
    }

    al_start_timer(timer);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(loop_principal, 0, 1);
#else
    while (true) {
        loop_principal();
    }
#endif

    // Limpieza de memoria explícita de Allegro 5
    if (buffer) al_destroy_bitmap(buffer);
    if (splash_imagen) al_destroy_bitmap(splash_imagen);
    if (splash_victoria) al_destroy_bitmap(splash_victoria);
    if (fuente) al_destroy_font(fuente);
    al_destroy_timer(timer);
    al_destroy_event_queue(event_queue);
    al_destroy_display(display);

    return 0;
}

// =====================================================================
// --- IMPLEMENTACIÓN DE TU FÍSICA Y LÓGICA DE NIVELES ORIGINALES ---
// =====================================================================

void rotar(float &x, float &y, float cx, float cy, float da) {
    float radianes = da * M_PI / 180.0;
    float nx = cx + (x - cx) * cos(radianes) - (y - cy) * sin(radianes);
    float ny = cy + (x - cx) * sin(radianes) + (y - cy) * cos(radianes);
    x = nx; y = ny;
}

void pintar_nave(float cx, float cy) {
    // Sustitución de triangle() y rect() por al_draw_filled_triangle() y al_draw_rectangle()
    al_draw_filled_triangle(cx-21, cy+9, cx-18, cy+21, cx-9, cy+21, al_map_rgb(255, 255, 255));
    al_draw_filled_triangle(cx+21, cy+9, cx+18, cy+21, cx+9, cy+21, al_map_rgb(255, 255, 255));
    al_draw_filled_rectangle(cx-11, cy-16, cx+11, cy+1, al_map_rgb(0, 255, 255));
    al_draw_filled_triangle(cx-11, cy-16, cx+11, cy-16, cx, cy-25, al_map_rgb(255, 0, 0));
}

void mover_nave(float &cx, float &cy, float &vx, float &vy) {
    vy += 0.012f; // Gravedad constante lunar
    cx += vx;
    cy += vy;
}

void aceleracion(float da, float &vx, float &vy) {
    float ax = 0, ay = -0.035f;
    rotar(ax, ay, 0, 0, da);
    vx += ax;
    vy += ay;
}

void pintar_motor(float da, float cx, float cy) {
    float x1 = cx - 5, y1 = cy + 5;
    float x2 = cx + 5, y2 = cy + 5;
    float x3 = cx,     y3 = cy + 18;
    rotar(x1, y1, cx, cy, da);
    rotar(x2, y2, cx, cy, da);
    rotar(x3, y3, cx, cy, da);
    al_draw_filled_triangle(x1, y1, x2, y2, x3, y3, al_map_rgb(255, 130, 0));
}

void medidor_combustible(float combustible) {
    al_draw_textf(fuente, al_map_rgb(255, 255, 255), 15, 15, 0, "FUEL: %.1f%%", combustible);
    al_draw_rectangle(15, 30, 115, 45, al_map_rgb(255, 255, 255), 1.0);
    if (combustible > 0) {
        al_draw_filled_rectangle(16, 31, 16 + combustible, 44, al_map_rgb(0, 255, 0));
    }
}

void pintar_nivel(int num_nivel) {
    // Dibujado del terreno usando las primitivas de Allegro 5
    if (num_nivel == 1) {
        al_draw_filled_triangle(110, 100, 300, 500, 0, 500, al_map_rgb(139, 69, 19));
        al_draw_filled_rectangle(300, 480, 420, 500, al_map_rgb(0, 255, 0)); // Plataforma verde
    }
    else if (num_nivel == 2) {
        al_draw_filled_triangle(110, 100, 300, 500, 0, 500, al_map_rgb(120, 120, 120));
        al_draw_filled_triangle(500, 500, 600, 300, 740, 500, al_map_rgb(120, 120, 120));
        al_draw_filled_rectangle(420, 480, 500, 500, al_map_rgb(0, 255, 0));
    }
    // Agrega el resto de tus niveles replicando los al_draw_filled_triangle según tus coordenadas...
    dibujar_triangulos(); // Rocas cayendo de tus niveles avanzados
}

void avanzar_nivel(float &cx, float &cy, float &vx, float &vy, int &num_nivel, float &combustible) {
    if (aterrizar(cx, cy, vx, vy, num_nivel)) {
        if (num_nivel == 8) {
            estado_actual = ESTADO_VICTORIA;
            if (musica_fondo) al_stop_samples();
            if (sfx_victoria) al_play_sample(sfx_victoria, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
            return;
        }
        num_nivel++;
        reiniciar_nivel(cx, cy, vx, vy, combustible, num_nivel);
        nivel_y = -30;
        nivel_timer = 2000;
    }
}

void reiniciar_nivel(float &cx, float &cy, float &vx, float &vy, float &combustible, int num_nivel) {
    combustible = 100;
    vx = 0; vy = -0.05f;
    cx = 710; cy = 100;
    if (num_nivel == 4) { cy = 80; vy = 0; }
}

bool aterrizar(float cx, float cy, float vx, float vy, int num_nivel) {
    // Tu lógica exacta de colisión con la plataforma verde
    if (num_nivel == 1 && cx >= 300 && cx <= 420 && cy >= 460 && vy < 0.15f) return true;
    if (num_nivel == 2 && cx >= 420 && cx <= 500 && cy >= 460 && vy < 0.15f) return true;
    return false;
}

bool game_over(float &cx, float &cy, float &vx, float &vy, int &num_nivel, float &combustible) {
    if (cy > 500 || cx < 0 || cx > 740) {
        if (sfx_explosion) al_play_sample(sfx_explosion, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
        return true;
    }
    return false;
}

void dibujar_triangulos() {
    // Simulación del comportamiento de meteoritos/rocas cayendo
    for (int i = 0; i < MAX_ROCAS; i++) {
        if (roca_activa[i]) {
            roca_y[i] += roca_vy[i];
            if (roca_y[i] > 500) roca_activa[i] = false;
            al_draw_filled_triangle(roca_x[i]-12, roca_y[i]-6, roca_x[i]+12, roca_y[i]-6, roca_x[i], roca_y[i]+12, al_map_rgb(153,153,153));
        } else {
            roca_timer[i]--;
            if (roca_timer[i] <= 0) {
                roca_x[i] = 100 + rand() % 500;
                roca_y[i] = 30;
                roca_vy[i] = 1.5f + (rand() % 4);
                roca_activa[i] = true;
                roca_timer[i] = 120;
            }
        }
    }
}
