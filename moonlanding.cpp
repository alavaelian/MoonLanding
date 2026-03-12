#include <allegro.h>
#include <cmath>
#include <iostream>
#include <unistd.h>
using namespace std;
SAMPLE *musica_fondo = NULL;
SAMPLE *sfx_explosion = NULL;
void rotar(float &x, float &y, float cx, float cy, float da);
void pintar_nave(float cx, float cy, BITMAP *buffer);
void mover_nave(float &cx, float &cy, float &vx, float &vy);
void aceleracion(float da, float &vx, float &vy);
void pintar_motor(float da, float cx, float cy, BITMAP *buffer);
void medidor_combustible(float combustible, BITMAP *buffer);
void pintar_nivel(int num_nivel, BITMAP *buffer, float &cx, float &cy,
                  float &vx, float &vy);
void explocion(float cx, float cy, BITMAP *buffer, int num_nivel, float vx,
               float vy);
bool game_over(float &cx, float &cy, float &vx, float &vy, int &num_nivel,
               float &combustible, BITMAP *buffer);
bool aterrizar(float cx, float cy, float vx, float vy, BITMAP *buffer,
               int num_nivel);
bool choque_triangulo(float x1, float y1, float x2, float y2, float p1x,
                      float p1y, float p2x, float p2y, string tipo_triangulo);
bool choque_nave(int num_nivel, float cx, float cy);
void avanzar_nivel(float &cx, float &cy, float &vx, float &vy, int &num_nivel,
                   float &combustible, BITMAP *BUFFER);
void reiniciar_nivel(float &cx, float &cy, float &vx, float &vy,
                     float &combustible, int num_nivel);
void actualizar_circulos_moviles();
void mostrar_splash_con_fade_con_tecla(const char *ruta_imagen,
                                       const char *ruta_wav) {
  BITMAP *splash = NULL;
  SAMPLE *sonido = NULL; // Cambiamos MIDI por SAMPLE para archivos WAV
  PALETTE pal;

  // 1. Configuraciones de compatibilidad
  set_color_conversion(COLORCONV_TOTAL);

  // 2. Carga de Imagen (con el arreglo de ruta para Linux)
  splash = load_bitmap(ruta_imagen, pal);
  if (!splash) {
    // Intento de rescate si no encuentra la ruta
    string alt = "./" + string(ruta_imagen);
    splash = load_bitmap(alt.c_str(), pal);
  }

  if (!splash) {
    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
    fprintf(stderr, "Error Allegro Imagen: %s\n", allegro_error);
    allegro_message("No se encontro la imagen: %s", ruta_imagen);
    exit(1);
  }

  // 3. Carga de Sonido WAV
  sonido = load_sample(ruta_wav);
  if (sonido) {
    // play_sample(muestra, volumen[0-255], pan[128=centro], freq[1000=normal],
    // loop[1=si])
    play_sample(sonido, 255, 128, 1000, 1);
  } else {
    // Si no carga el sonido, imprimimos error en consola pero dejamos seguir el
    // juego
    fprintf(stderr, "Error Allegro Sonido (WAV): %s\n", allegro_error);
  }

  // --- FADE IN ---
  for (int alpha = 0; alpha <= 255; alpha += 8) {
    set_trans_blender(0, 0, 0, alpha);
    // Usamos stretch_sprite para ajustar tu imagen de 1536px a los 740px de tu
    // ventana
    stretch_sprite(screen, splash, 0, 0, SCREEN_W, SCREEN_H);
    rest(15);
    if (key[KEY_ESC])
      goto fin;
  }

  // --- ESPERA ---
  clear_keybuf();
  {
    int parpadeo = 0;
    while (!keypressed()) {
      stretch_blit(splash, screen, 0, 0, splash->w, splash->h, 0, 0, SCREEN_W,
                   SCREEN_H);

      if ((parpadeo++ % 80) < 40) {
        textout_centre_ex(screen, font, "PRESS A KEY TO START", SCREEN_W / 2,
                          SCREEN_H - 50, makecol(255, 255, 255), -1);
      }
      rest(10);
      if (key[KEY_ESC])
        break;
    }
  }

  // --- FADE OUT ---
  for (int alpha = 255; alpha >= 0; alpha -= 8) {
    set_trans_blender(0, 0, 0, alpha);
    stretch_sprite(screen, splash, 0, 0, SCREEN_W, SCREEN_H);
    rest(15);
  }

fin:
  if (sonido) {
    stop_sample(sonido);
    destroy_sample(sonido);
  }
  if (splash)
    destroy_bitmap(splash);
  clear_to_color(screen, makecol(0, 0, 0));
}
// --- TEMPORIZADOR PARA CONTROLAR LOS FPS ---
volatile int fps_counter = 0;
void incrementa_fps() { fps_counter++; }
END_OF_FUNCTION(incrementa_fps);
// -------------------------------------------
struct Estrella {
  int x, y;
  int color;
};
void pantalla_victoria(BITMAP *buffer); // ← agregar aquí

bool motor_encendido = false; // Nueva variable de estado

int nivel_y = -30;      // Empieza fuera de la pantalla (arriba)
int nivel_timer = 2000; // Duración del mensaje (aprox 5 segundos a 60fps)
Estrella fondo[100];    // 100 estrellas es un buen número para 740x500
SAMPLE *sfx_motor = NULL;
int id_motor = -1;
void pantalla_victoria(BITMAP *buffer) {

  // ── AUDIO ────────────────────────────────────────────────────────
  if (musica_fondo)
    stop_sample(musica_fondo);
  if (sfx_motor)
    adjust_sample(sfx_motor, 0, 128, 1000, 1);
  motor_encendido = false;
  rest(100);

  SAMPLE *sfx_victoria = load_sample("./victory_fixed.wav");
  if (sfx_victoria)
    play_sample(sfx_victoria, 255, 128, 1000, 0);

  // ── SPLASH ───────────────────────────────────────────────────────
  PALETTE pal;
  BITMAP *splash_victoria = load_bitmap("./victoria_art.bmp", pal);

  // ── HELPERS DE TEXTO ─────────────────────────────────────────────
  auto texto_glow = [&](const char *texto, int x, int y, int color) {
    int g1 = makecol(0, 60, 60);   // cian muy oscuro
    int g2 = makecol(0, 140, 140); // cian medio
    textout_centre_ex(buffer, font, texto, x - 2, y, g1, -1);
    textout_centre_ex(buffer, font, texto, x + 2, y, g1, -1);
    textout_centre_ex(buffer, font, texto, x, y - 2, g1, -1);
    textout_centre_ex(buffer, font, texto, x, y + 2, g1, -1);
    textout_centre_ex(buffer, font, texto, x - 1, y, g2, -1);
    textout_centre_ex(buffer, font, texto, x + 1, y, g2, -1);
    textout_centre_ex(buffer, font, texto, x, y - 1, g2, -1);
    textout_centre_ex(buffer, font, texto, x, y + 1, g2, -1);
    textout_centre_ex(buffer, font, texto, x, y, color, -1);
  };

  auto texto_borde = [&](const char *texto, int x, int y, int color) {
    int negro = makecol(0, 0, 0);
    textout_centre_ex(buffer, font, texto, x - 1, y, negro, -1);
    textout_centre_ex(buffer, font, texto, x + 1, y, negro, -1);
    textout_centre_ex(buffer, font, texto, x, y - 1, negro, -1);
    textout_centre_ex(buffer, font, texto, x, y + 1, negro, -1);
    textout_centre_ex(buffer, font, texto, x, y, color, -1);
  };

  auto texto_grande = [&](const char *texto, int x, int y, int color,
                          int escala) {
    int len = strlen(texto);
    BITMAP *tmp = create_bitmap(len * 8 + 2, 10);
    clear_to_color(tmp, makecol(255, 0, 255));
    textout_ex(tmp, font, texto, 1, 1, color, -1);
    int w = tmp->w * escala;
    int h = tmp->h * escala;
    draw_sprite(buffer, tmp, x - w / 2,
                y - h / 2); // respeta magenta como transparente
    // stretch_sprite no funciona bien con colores, usamos stretch_blit
    stretch_blit(tmp, buffer, 0, 0, tmp->w, tmp->h, x - w / 2, y - h / 2, w, h);
    destroy_bitmap(tmp);
  };

  // ── ANIMACION ────────────────────────────────────────────────────
  float nave_x = 370, nave_y = 500;
  int parpadeo = 0;

  int c_cyan = makecol(0, 255, 255);
  int c_dorado = makecol(255, 215, 0);
  int c_blanco = makecol(255, 255, 255);
  int c_verde = makecol(0, 255, 128);

  while (!key[KEY_ESC] && !key[KEY_A]) {

    clear_to_color(buffer, makecol(0, 0, 0));

    // Fondo splash o estrellas si no cargó
    if (splash_victoria) {
      stretch_blit(splash_victoria, buffer, 0, 0, splash_victoria->w,
                   splash_victoria->h, 0, 0, 740, 500);
    } else {
      for (int i = 0; i < 100; i++) {
        if (rand() % 100 < 5) {
          int g = 100 + rand() % 155;
          fondo[i].color = makecol(g, g, g);
        }
        putpixel(buffer, fondo[i].x, fondo[i].y, fondo[i].color);
      }
    }

    // Nave asciende desde abajo hasta el centro
    if (nave_y > 200)
      nave_y -= 1.5f;
    pintar_nave(nave_x, nave_y, buffer);
    if (nave_y > 200)
      pintar_motor(0, nave_x, nave_y, buffer);

    // Marco doble con cian
    rect(buffer, 150, 118, 590, 362, makecol(0, 100, 100)); // sombra marco
    rect(buffer, 148, 116, 592, 364, makecol(0, 100, 100));
    rect(buffer, 150, 120, 590, 360, c_cyan);
    rect(buffer, 152, 122, 588, 358, c_cyan);
    hline(buffer, 160, 168, 580, c_cyan);
    hline(buffer, 160, 292, 580, c_cyan);

    // Título con glow grande
    texto_glow("** MISION CUMPLIDA **", 370, 145, c_cyan);

    // Subtítulos con borde
    texto_borde("HAS COMPLETADO TODOS LOS NIVELES", 370, 200, c_blanco);
    texto_borde("ALUNIZAJE PERFECTO, COMANDANTE", 370, 220, c_verde);

    // Instrucciones parpadeantes con sombra
    if ((parpadeo++ % 80) < 50) {
      // Sombra
      textout_centre_ex(buffer, font, "PRESS (A) TO PLAY AGAIN", 372, 317,
                        makecol(0, 60, 0), -1);
      textout_centre_ex(buffer, font, "PRESS (ESC) TO EXIT", 372, 337,
                        makecol(0, 60, 0), -1);
      // Texto real
      textout_centre_ex(buffer, font, "PRESS (A) TO PLAY AGAIN", 370, 315,
                        c_verde, -1);
      textout_centre_ex(buffer, font, "PRESS (ESC) TO EXIT", 370, 335, c_blanco,
                        -1);
    }

    blit(buffer, screen, 0, 0, 0, 0, 740, 500);
    rest(2);
  }

  // ── LIMPIEZA ─────────────────────────────────────────────────────
  if (splash_victoria)
    destroy_bitmap(splash_victoria);
  if (sfx_victoria) {
    stop_sample(sfx_victoria);
    destroy_sample(sfx_victoria);
  }
  if (key[KEY_A] && musica_fondo)
    play_sample(musica_fondo, 160, 128, 1000, 1);
}
int main() {
  srand(time(NULL));
  allegro_init();
  install_keyboard();
  install_timer(); // <--- INICIA EL SISTEMA DE TIMERS
  if (install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL) != 0) {
    fprintf(stderr, "Error inicializando audio: %s\n", allegro_error);
  }
  // Bloqueamos la memoria (requerido por Allegro)
  LOCK_VARIABLE(fps_counter);
  LOCK_FUNCTION(incrementa_fps);

  // Le decimos que corra a 60 FPS (Ticks por segundo)
  install_int_ex(incrementa_fps, BPS_TO_TIMER(60));
  set_color_depth(32);
  set_gfx_mode(GFX_AUTODETECT_WINDOWED, 740, 500, 0, 0);
  mostrar_splash_con_fade_con_tecla("./splash_nix.bmp", "pdpsong.wav");
  SAMPLE *sfx_start = load_sample("./sonidostart.wav");
  sfx_motor = load_sample("./thrust.wav");
  sfx_explosion = load_sample(
      "./boom.wav"); // Asegúrate de que el nombre sea igual al archivo
  if (sfx_motor) {
    play_sample(sfx_motor, 0, 128, 1000, 1); // Volumen 0, loop activado
  }
  if (sfx_start) {
    play_sample(sfx_start, 255, 128, 1000,
                0); // El '0' final significa que NO se repite
  }
  // LIMPIEZA TOTAL DE TECLADO
  clear_keybuf(); // Borra la memoria del teclado
  while (keypressed()) {
    // Mientras haya teclas guardadas, las borramos todas
    clear_keybuf();
    rest(1);
  }
  // Esperamos a que el usuario SUELTE físicamente las teclas
  while (key[KEY_A] || key[KEY_ENTER] || key[KEY_SPACE]) {
    rest(1);
  }
  rest(200);
  musica_fondo =
      load_sample("./gamesong.wav"); // Cambia por el nombre de tu archivo
  if (musica_fondo) {
    play_sample(musica_fondo, 160, 128, 1000, 1);
  }
  BITMAP *buffer = create_bitmap(740, 500);
  float cx = 710,
        cy = 100; // En medio de la pantalla (lejos de bordes y suelos)
  float vx = 0, vy = 0;
  // float cx=710,cy=100;
  // float vx=0,vy=-0.05;
  float combustible = 100;
  int num_nivel = 1;
  // reiniciar_nivel(cx, cy, vx, vy, combustible);
  fps_counter = 0; // <--- ESTO EVITA EL SALTO TEMPORAL
  rest(200);
  // --- Inicializar Estrellas ---
  for (int i = 0; i < 100; i++) {
    fondo[i].x = rand() % 740;
    fondo[i].y = rand() % 500;
    // Variaciones de gris/blanco para dar profundidad
    int gris = 100 + (rand() % 155);
    fondo[i].color = makecol(gris, gris, gris);
  }

  while (!key[KEY_ESC] &&
         !game_over(cx, cy, vx, vy, num_nivel, combustible, buffer)) {
    while (fps_counter > 0) {
      int motores_activos = 0;
      mover_nave(cx, cy, vx, vy);

      if (key[KEY_UP] && combustible >= 0) {
        aceleracion(0, vx, vy);
        motores_activos++;
      }
      if (key[KEY_RIGHT] && combustible >= 0) {
        aceleracion(-90, vx, vy);
        motores_activos++;
      }
      if (key[KEY_LEFT] && combustible >= 0) {
        aceleracion(90, vx, vy);
        motores_activos++;
      }
      if (motores_activos > 0) {
        combustible = combustible - (0.2 * motores_activos);
      }

      actualizar_circulos_moviles();

      // medidor_combustible(gastar_combustible, combustible, buffer);
      // medidor_combustible( combustible, buffer);
      avanzar_nivel(cx, cy, vx, vy, num_nivel, combustible, buffer);

      fps_counter--; // Restamos 1 al timer
    }

    // 2. DIBUJADO EN PANTALLA (Se hace todo el tiempo)
    clear_to_color(buffer, 0x000000);
    for (int i = 0; i < 100; i++) {
      // 5% de probabilidad de que una estrella cambie de brillo este frame
      if (rand() % 100 < 5) {
        int gris = 100 + (rand() % 155);
        fondo[i].color = makecol(gris, gris, gris);
      }
      putpixel(buffer, fondo[i].x, fondo[i].y, fondo[i].color);
    }
    pintar_nivel(num_nivel, buffer, cx, cy, vx, vy);

    // Pintamos los motores solo si se presionan las teclas
    if (key[KEY_UP] && combustible >= 0)
      pintar_motor(0, cx, cy, buffer);
    if (key[KEY_RIGHT] && combustible >= 0)
      pintar_motor(-90, cx, cy, buffer);
    if (key[KEY_LEFT] && combustible >= 0)
      pintar_motor(90, cx, cy, buffer);
    if ((key[KEY_UP] || key[KEY_LEFT] || key[KEY_RIGHT]) && combustible > 0) {
      if (!motor_encendido) {
        if (sfx_motor)
          adjust_sample(sfx_motor, 160, 128, 1000, 1);
        motor_encendido = true;
      }
    } else {
      // Si NO se pulsa ninguna de las tres, o se acabó el combustible, silencio
      if (motor_encendido) {
        if (sfx_motor)
          adjust_sample(sfx_motor, 0, 128, 1000, 1);
        motor_encendido = false;
      }
    }
    pintar_nave(cx, cy, buffer);

    medidor_combustible(combustible, buffer);
    // --- HUD: PRESENTACIÓN DE NIVEL LENTA ---
    if (nivel_timer > 0) {
      // 1. Entrada: Baja más lento (1 pixel por frame en vez de 2)
      if (nivel_y < 25 && nivel_timer > 100) {
        nivel_y += 1;
      }
      // 2. Salida: Solo empieza a subir cuando falten 80 frames para acabar
      else if (nivel_timer < 80) {
        nivel_y -= 1;
      }

      // 3. El cuadro y el borde (ajustados para que se vean perfectos)
      rectfill(buffer, SCREEN_W / 2 - 110, nivel_y - 8, SCREEN_W / 2 + 110,
               nivel_y + 18, makecol(0, 0, 0));
      rect(buffer, SCREEN_W / 2 - 110, nivel_y - 8, SCREEN_W / 2 + 110,
           nivel_y + 18, makecol(255, 255, 255));

      // 4. Texto dinámico
      textprintf_centre_ex(buffer, font, SCREEN_W / 2, nivel_y,
                           makecol(255, 255, 255), -1, "MISIÓN - NIVEL %d",
                           num_nivel);

      // En lugar de nivel_timer--, solo descuenta si el jugador se está
      // moviendo
      if (key[KEY_UP] || key[KEY_LEFT] || key[KEY_RIGHT]) {
        nivel_timer--;
      }
    }
    blit(buffer, screen, 0, 0, 0, 0, 740, 500);
  }
  return 0;
}
END_OF_MAIN();

bool choque_circulo(float centro_x, float centro_y, float radio, float p1x,
                    float p1y, float p2x, float p2y) {
  // Vector del segmento
  float dx = p2x - p1x;
  float dy = p2y - p1y;

  // Vector del punto inicial al centro del círculo
  float fx = p1x - centro_x;
  float fy = p1y - centro_y;

  float a = dx * dx + dy * dy; // Longitud al cuadrado del segmento
  if (a == 0) {
    // El segmento es un punto: verificar si ese punto está dentro del círculo
    return (fx * fx + fy * fy) <= radio * radio;
  }

  // Proyección del centro sobre la línea (parámetro t)
  float b = 2.0f * (fx * dx + fy * dy);
  float c = fx * fx + fy * fy - radio * radio;
  float discriminante = b * b - 4.0f * a * c;

  if (discriminante < 0)
    return false; // No hay intersección

  discriminante = sqrt(discriminante);
  float t1 = (-b - discriminante) / (2.0f * a);
  float t2 = (-b + discriminante) / (2.0f * a);

  // Verificar si algún punto de intersección está en el segmento [0,1]
  if ((t1 >= 0.0f && t1 <= 1.0f) || (t2 >= 0.0f && t2 <= 1.0f))
    return true;

  // También verificar los extremos (por si el segmento está completamente
  // dentro)
  if ((fx * fx + fy * fy) <= radio * radio)
    return true;
  float fx2 = p2x - centro_x;
  float fy2 = p2y - centro_y;
  if ((fx2 * fx2 + fy2 * fy2) <= radio * radio)
    return true;

  return false;
}

// Estructura para círculo estático con gravedad
struct CirculoEstatico {
  float cx, cy;       // centro
  float r_colision;   // radio de colisión
  float r_influencia; // radio de influencia (mayor)
  float gravedad;     // intensidad de la atracción
};

// Supongamos que tienes un array de círculos estáticos
#define MAX_CIRCULOS_ESTATICOS 5
CirculoEstatico circulos_estaticos[MAX_CIRCULOS_ESTATICOS];
int num_circulos_estaticos = 0;

// Función para inicializar algunos círculos (llamarla al empezar el nivel)
void iniciar_circulos_estaticos() {
  num_circulos_estaticos = 3;
  circulos_estaticos[0] = {200, 250, 40, 190, 0.5f};
  circulos_estaticos[1] = {500, 150, 30, 150, 0.4f};
  circulos_estaticos[2] = {600, 400, 25, 120, 0.2f};
}

// Función para aplicar gravedad de círculos estáticos a la nave
void aplicar_gravedad_circulos_estaticos(float &cx, float &cy, float &vx,
                                         float &vy) {
  for (int i = 0; i < num_circulos_estaticos; i++) {
    float dx = circulos_estaticos[i].cx - cx;
    float dy = circulos_estaticos[i].cy - cy;
    float dist = sqrt(dx * dx + dy * dy);
    // Solo actuar dentro del radio de influencia y evitar distancias
    // extremadamente pequeñas
    if (dist < circulos_estaticos[i].r_influencia && dist > 5.0f) {
      float dir_x = dx / dist;
      float dir_y = dy / dist;
      // Aceleración variable: más fuerte cerca, más débil lejos
      // Opción 1: inversa al cuadrado (realista) pero limitada por la distancia
      // mínima float acel = circulos_estaticos[i].gravedad / (dist * dist);
      // Opción 2: lineal (suave) como un resorte
      float factor =
          1.0f - dist / circulos_estaticos[i].r_influencia; // de 0 a 1
      float acel = circulos_estaticos[i].gravedad *
                   factor; // 0 en el borde, gravedad en el centro (pero como
                           // dist>5, no llega a 0)
      // Ajusta la intensidad global con un factor adicional si es necesario
      float escala = 0.05f; // muy suave, para que los motores puedan vencerla
      vx += dir_x * acel * escala;
      vy += dir_y * acel * escala;
    }
  }
}
// Función para verificar colisiones con círculos estáticos (dentro de
// choque_nave) Estructura para círculo móvil
#define MAX_CIRCULOS_MOVILES 8

struct CirculoMovil {
  float x, y;         // posición actual
  float vx, vy;       // velocidad
  float r_colision;   // radio de colisión (el círculo visible)
  float r_influencia; // radio de gravedad (más grande)
  float gravedad;     // intensidad de la atracción
  bool activo;
  int timer; // para reaparecer
};

CirculoMovil circulos_moviles[MAX_CIRCULOS_MOVILES];
bool circulos_inicializados = false; // para inicializar una vez// Inicializar
                                     // (por ejemplo, todos inactivos)
void iniciar_circulos_moviles() {
  for (int i = 0; i < MAX_CIRCULOS_MOVILES; i++) {
    circulos_moviles[i].activo = false;
    circulos_moviles[i].timer = 10 + rand() % 30; // entre 10 y 40 frames
  }
}
// Actualizar posición y estado de los círculos móviles (llamar cada frame)
// ============================================================================
// ACTUALIZAR CÍRCULOS MÓVILES (llamar en cada frame, dentro del bucle de
// física)
// ============================================================================
void actualizar_circulos_moviles() {
  for (int i = 0; i < MAX_CIRCULOS_MOVILES; i++) {
    // -------------------------------------------------
    // Si el círculo está activo: mover y verificar si sale
    // -------------------------------------------------
    if (circulos_moviles[i].activo) {
      // Actualizar posición según velocidad
      circulos_moviles[i].x += circulos_moviles[i].vx;
      circulos_moviles[i].y += circulos_moviles[i].vy;

      // Si se aleja demasiado de la pantalla (margen 100 píxeles), lo
      // desactivamos
      if (circulos_moviles[i].x < -100 || circulos_moviles[i].x > 840 ||
          circulos_moviles[i].y < -100 || circulos_moviles[i].y > 600) {
        circulos_moviles[i].activo = false;
        // Le ponemos un timer para que reaparezca después de un tiempo
        circulos_moviles[i].timer = 50 + rand() % 100; // entre 50 y 149 frames
      }
    }
    // -------------------------------------------------
    // Si NO está activo: decrementar timer y activar cuando llegue a cero
    // -------------------------------------------------
    else {
      // Decrementar el temporizador
      circulos_moviles[i].timer--;

      // Si el timer llega a cero, lo activamos con nuevos valores
      if (circulos_moviles[i].timer <= 0) {
        // Elegir un lado por donde aparecer (0: izquierda, 1: derecha, 2:
        // arriba, 3: abajo)
        int lado = rand() % 4;
        float x, y, vx, vy;

        // Asignar posición y velocidad según el lado
        switch (lado) {
        case 0: // izquierda
          x = -40;
          y = rand() % 500;
          vx = 1.5f + (rand() % 5) * 0.3f;  // hacia la derecha
          vy = (rand() % 100 - 50) / 50.0f; // entre -1 y 1
          break;
        case 1: // derecha
          x = 780;
          y = rand() % 500;
          vx = -1.5f - (rand() % 5) * 0.3f; // hacia la izquierda
          vy = (rand() % 100 - 50) / 50.0f;
          break;
        case 2: // arriba
          x = rand() % 740;
          y = -40;
          vx = (rand() % 100 - 50) / 50.0f;
          vy = 1.5f + (rand() % 5) * 0.3f; // hacia abajo
          break;
        case 3: // abajo
          x = rand() % 740;
          y = 540;
          vx = (rand() % 100 - 50) / 50.0f;
          vy = -1.5f - (rand() % 5) * 0.3f; // hacia arriba
          break;
        }

        // Asignar los valores al círculo
        circulos_moviles[i].x = x;
        circulos_moviles[i].y = y;
        circulos_moviles[i].vx = vx;
        circulos_moviles[i].vy = vy;
        circulos_moviles[i].r_colision = 15 + rand() % 20; // entre 15 y 34
        circulos_moviles[i].r_influencia = circulos_moviles[i].r_colision * 3;
        circulos_moviles[i].gravedad =
            0.02f + (rand() % 10) * 0.01f; // entre 0.02 y 0.12
        circulos_moviles[i].activo = true; // ¡Lo activamos!
      }
    }
  }
}
// Verificar colisiones con círculos móviles
void dibujar_circulos(BITMAP *buffer) {
  int color = 0x999999;
  for (int i = 0; i < num_circulos_estaticos; i++) {
    circlefill(buffer, circulos_estaticos[i].cx, circulos_estaticos[i].cy,
               circulos_estaticos[i].r_colision, color);
    set_trans_blender(50, 50, 50,
                      50); // (r,g,b,alpha) 0-255, 128 = 50% transparente

    // Activar modo translúcido
    drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);

    circle(buffer, circulos_estaticos[i].cx, circulos_estaticos[i].cy,
           circulos_estaticos[i].r_influencia, 0xCCCCCC);
    drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);
  }
  // Círculos móviles
}
void dibujar_circulos_moviles(BITMAP *buffer) {
  int color = 0x999999; // gris
  int activos = 0;
  for (int i = 0; i < MAX_CIRCULOS_MOVILES; i++) {
    if (circulos_moviles[i].activo) {
      activos++;
      // Círculo de colisión (relleno)
      circlefill(buffer, (int)circulos_moviles[i].x, (int)circulos_moviles[i].y,
                 circulos_moviles[i].r_colision, color);
      // Opcional: círculo de influencia (transparente)
      set_trans_blender(128, 128, 128, 128);
      drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
      circle(buffer, (int)circulos_moviles[i].x, (int)circulos_moviles[i].y,
             circulos_moviles[i].r_influencia, 0xCCCCCC);
      drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);
    }
  }
  // textprintf_ex(buffer, font, 10, 400, 0xFFFFFF, -1, "Circulos activos: %d",
  // activos);
}
void aplicar_gravedad_circulos_moviles(float &cx, float &cy, float &vx,
                                       float &vy) {
  for (int i = 0; i < MAX_CIRCULOS_MOVILES; i++) {
    if (!circulos_moviles[i].activo)
      continue;
    float dx = circulos_moviles[i].x - cx;
    float dy = circulos_moviles[i].y - cy;
    float dist = sqrt(dx * dx + dy * dy);
    if (dist < circulos_moviles[i].r_influencia && dist > 1.0f) {
      float dir_x = dx / dist;
      float dir_y = dy / dist;
      float acel = circulos_moviles[i].gravedad;
      vx += dir_x * acel;
      vy += dir_y * acel;
    }
  }
}
#define MAX_ROCAS 15 // Número de rocas
float roca_x[MAX_ROCAS];
float roca_y[MAX_ROCAS];
float roca_vy[MAX_ROCAS];
bool roca_activa[MAX_ROCAS];
int roca_timer[MAX_ROCAS];
float gravedad_rocas = 0.5f;

void dibujar_triangulos(BITMAP *buffer) {
  static bool inicializado = false;
  static int contador_frames = 0;

  if (!inicializado) {
    for (int i = 0; i < MAX_ROCAS; i++) {
      roca_activa[i] = false;
      roca_timer[i] = 5 + i * 5; // Tiempos de espera iniciales
    }
    inicializado = true;
  }

  // Control de velocidad: actualizar cada 3 frames
  contador_frames++;
  if (contador_frames >= 60) {
    contador_frames = 0;
    for (int i = 0; i < MAX_ROCAS; i++) {
      if (roca_activa[i]) {
        // Caída
        roca_y[i] += roca_vy[i];
        roca_vy[i] += gravedad_rocas;

        // Si toca el suelo, desactivar y esperar
        if (roca_y[i] > 500) { // Ajusta según tu límite de pantalla
          roca_activa[i] = false;
          roca_timer[i] = 10; // Tiempo de reaparición
        }
      } else {
        // Temporizador para nueva roca
        roca_timer[i]--;
        if (roca_timer[i] <= 0) {
          roca_x[i] = 100 + rand() % 500;
          roca_y[i] = 30;
          roca_vy[i] = 0.4f + (rand() % 5) * 0.4f; // Velocidad variable
          roca_activa[i] = true;
        }
      }
    }
  }

  // Dibujar rocas activas en gris
  for (int i = 0; i < MAX_ROCAS; i++) {
    if (roca_activa[i]) {
      triangle(buffer, roca_x[i] - 12, roca_y[i] - 6, roca_x[i] + 12,
               roca_y[i] - 6, roca_x[i], roca_y[i] + 12,
               0x999999); // Gris
    }
  }
}

void reiniciar_nivel(float &cx, float &cy, float &vx, float &vy,
                     float &combustible, int num_nivel) {
  combustible = 100;
  vx = 0;
  vy = 0;

  switch (num_nivel) {
  case 1:
    cx = 710;
    cy = 100;
    vy = -0.05;
    break;
  case 2:
    cx = 710;
    cy = 100;
    vy = -0.05;
    break;
  case 3:
    cx = 710;
    cy = 100;
    vy = -0.05;
    break;
  case 4:
    cx = 710;
    cy = 80;
    vy = 0;
    break;
  default:
    cx = 710;
    cy = 100;
    vy = -0.05;
    break;
  }
}
void avanzar_nivel(float &cx, float &cy, float &vx, float &vy, int &num_nivel,
                   float &combustible, BITMAP *buffer) {
  if (aterrizar(cx, cy, vx, vy, buffer, num_nivel) == true) {
    if (sfx_motor)
      adjust_sample(sfx_motor, 0, 128, 1000, 1);
    motor_encendido = false;
    if (num_nivel == 10) {
      // ← AQUÍ, antes de cualquier otra cosa
      pantalla_victoria(buffer);
      reiniciar_nivel(cx, cy, vx, vy, combustible, num_nivel);
      num_nivel = 1;
      fps_counter = 0;
      nivel_y = -30;
      nivel_timer = 2000;
      return;
    }

    // else cx = 710;cy = 100;
    vy = -0.05;
    vx = 0;
    if (num_nivel != 10)
      num_nivel++;
    reiniciar_nivel(cx, cy, vx, vy, combustible, num_nivel);
    while (!key[KEY_A]) {
      textout_centre_ex(screen, font, "PRESS (A) FOR THE NEXT LEVEL", 370, 250,
                        0xfbff00, 0x000000);
      rest(16);
    }

    combustible = 100;
    fps_counter = 0;
    // Subes el nivel
    nivel_y = -30;      // Lo mandas arriba de nuevo
    nivel_timer = 2000; // Le das tiempo para mostrarse
  }
}
bool choque_nave(int num_nivel, float cx, float cy) {
  float r1x = cx - 21, r1y = cy + 9;
  float r2x = cx - 18, r2y = cy + 21;
  float b1x = cx - 21, b1y = cy - 1;
  float b2x = cx - 9, b2y = cy + 11;
  float p1x = cx + 18, p1y = cy + 9;
  float p2x = cx + 21, p2y = cy + 21;
  float q1x = cx + 9, q1y = cy - 1;
  float q2x = cx + 21, q2y = cy + 11;
  float z1x = cx - 11, z1y = cy - 16;
  float z2x = cx + 11, z2y = cy + 1;

  if (num_nivel == 2) {
    if (choque_triangulo(110, 100, 300, 500, r1x, r1y, r2x, r2y, "abajo") ==
            true ||
        choque_triangulo(110, 100, 300, 500, b1x, b1y, b2x, b2y, "abajo") ==
            true ||
        choque_triangulo(110, 100, 300, 500, p1x, p1y, p2x, p2y, "abajo") ==
            true ||
        choque_triangulo(110, 100, 300, 500, q1x, q1y, q2x, q2y, "abajo") ==
            true ||
        choque_triangulo(110, 100, 300, 500, z1x, z1y, z2x, z2y, "abajo") ==
            true)
      return true;
    else if (choque_triangulo(500, 500, 600, 300, r1x, r1y, r2x, r2y,
                              "abajo") == true ||
             choque_triangulo(500, 500, 600, 300, b1x, b1y, b2x, b2y,
                              "abajo") == true ||
             choque_triangulo(500, 500, 600, 300, p1x, p1y, p2x, p2y,
                              "abajo") == true ||
             choque_triangulo(500, 500, 600, 300, q1x, q1y, q2x, q2y,
                              "abajo") == true ||
             choque_triangulo(500, 500, 600, 300, z1x, z1y, z2x, z2y,
                              "abajo") == true)
      return true;
    else if (choque_triangulo(600, 300, 800, 500, r1x, r1y, r2x, r2y,
                              "abajo") == true ||
             choque_triangulo(600, 300, 800, 500, b1x, b1y, b2x, b2y,
                              "abajo") == true ||
             choque_triangulo(600, 300, 800, 500, p1x, p1y, p2x, p2y,
                              "abajo") == true ||
             choque_triangulo(600, 300, 800, 500, q1x, q1y, q2x, q2y,
                              "abajo") == true ||
             choque_triangulo(600, 300, 800, 500, z1x, z1y, z2x, z2y,
                              "abajo") == true)
      return true;
    else if (choque_triangulo(200, 000, 400, 350, r1x, r1y, r2x, r2y,
                              "arriba") == true ||
             choque_triangulo(200, 000, 400, 350, b1x, b1y, b2x, b2y,
                              "arriba") == true ||
             choque_triangulo(200, 000, 400, 350, p1x, p1y, p2x, p2y,
                              "arriba") == true ||
             choque_triangulo(200, 000, 400, 350, q1x, q1y, q2x, q2y,
                              "arriba") == true ||
             choque_triangulo(200, 000, 400, 350, z1x, z1y, z2x, z2y,
                              "arriba") == true)
      return true;
  }
  if (num_nivel == 3) {
    if (choque_triangulo(110, 300, 300, 500, r1x, r1y, r2x, r2y, "abajo") ==
            true ||
        choque_triangulo(110, 300, 300, 500, b1x, b1y, b2x, b2y, "abajo") ==
            true ||
        choque_triangulo(110, 300, 300, 500, p1x, p1y, p2x, p2y, "abajo") ==
            true ||
        choque_triangulo(110, 300, 300, 500, q1x, q1y, q2x, q2y, "abajo") ==
            true ||
        choque_triangulo(110, 300, 300, 500, z1x, z1y, z2x, z2y, "abajo") ==
            true)
      return true;
    else if (choque_triangulo(305, 300, 510, 500, r1x, r1y, r2x, r2y,
                              "abajo") == true ||
             choque_triangulo(305, 300, 510, 500, b1x, b1y, b2x, b2y,
                              "abajo") == true ||
             choque_triangulo(305, 300, 510, 500, p1x, p1y, p2x, p2y,
                              "abajo") == true ||
             choque_triangulo(305, 300, 510, 500, q1x, q1y, q2x, q2y,
                              "abajo") == true ||
             choque_triangulo(305, 300, 510, 500, z1x, z1y, z2x, z2y,
                              "abajo") == true)
      return true;
    else if (choque_triangulo(400, 000, 600, 400, r1x, r1y, r2x, r2y,
                              "arriba") == true ||
             choque_triangulo(400, 000, 600, 400, b1x, b1y, b2x, b2y,
                              "arriba") == true ||
             choque_triangulo(400, 000, 600, 400, p1x, p1y, p2x, p2y,
                              "arriba") == true ||
             choque_triangulo(400, 000, 600, 400, q1x, q1y, q2x, q2y,
                              "arriba") == true ||
             choque_triangulo(400, 000, 600, 400, z1x, z1y, z2x, z2y,
                              "arriba") == true)
      return true;
    else if (choque_triangulo(100, 000, 250, 200, r1x, r1y, r2x, r2y,
                              "arriba") == true ||
             choque_triangulo(100, 000, 250, 200, b1x, b1y, b2x, b2y,
                              "arriba") == true ||
             choque_triangulo(100, 000, 250, 200, p1x, p1y, p2x, p2y,
                              "arriba") == true ||
             choque_triangulo(100, 000, 250, 200, q1x, q1y, q2x, q2y,
                              "arriba") == true ||
             choque_triangulo(100, 000, 250, 200, z1x, z1y, z2x, z2y,
                              "arriba") == true)
      return true;
  }
  if (num_nivel == 4) {
    // ===== TRIÁNGULOS DEL TECHO (base arriba, punta abajo) =====
    // Todos con tipo "arriba"

    // TRIÁNGULO 1: (100,0), (200,250), (300,0)
    // Izquierdo: (100,0) a (200,250)
    if (choque_triangulo(100, 0, 200, 250, r1x, r1y, r2x, r2y, "arriba") ||
        choque_triangulo(100, 0, 200, 250, b1x, b1y, b2x, b2y, "arriba") ||
        choque_triangulo(100, 0, 200, 250, p1x, p1y, p2x, p2y, "arriba") ||
        choque_triangulo(100, 0, 200, 250, q1x, q1y, q2x, q2y, "arriba") ||
        choque_triangulo(100, 0, 200, 250, z1x, z1y, z2x, z2y, "arriba"))
      return true;

    // Derecho: (200,250) a (300,0)
    if (choque_triangulo(200, 250, 300, 0, r1x, r1y, r2x, r2y, "arriba") ||
        choque_triangulo(200, 250, 300, 0, b1x, b1y, b2x, b2y, "arriba") ||
        choque_triangulo(200, 250, 300, 0, p1x, p1y, p2x, p2y, "arriba") ||
        choque_triangulo(200, 250, 300, 0, q1x, q1y, q2x, q2y, "arriba") ||
        choque_triangulo(200, 250, 300, 0, z1x, z1y, z2x, z2y, "arriba"))
      return true;

    // TRIÁNGULO 2: (400,0), (550,300), (700,0)
    // Izquierdo: (400,0) a (550,300)
    if (choque_triangulo(400, 0, 550, 300, r1x, r1y, r2x, r2y, "arriba") ||
        choque_triangulo(400, 0, 550, 300, b1x, b1y, b2x, b2y, "arriba") ||
        choque_triangulo(400, 0, 550, 300, p1x, p1y, p2x, p2y, "arriba") ||
        choque_triangulo(400, 0, 550, 300, q1x, q1y, q2x, q2y, "arriba") ||
        choque_triangulo(400, 0, 550, 300, z1x, z1y, z2x, z2y, "arriba"))
      return true;

    // Derecho: (550,300) a (700,0)
    if (choque_triangulo(550, 300, 700, 0, r1x, r1y, r2x, r2y, "arriba") ||
        choque_triangulo(550, 300, 700, 0, b1x, b1y, b2x, b2y, "arriba") ||
        choque_triangulo(550, 300, 700, 0, p1x, p1y, p2x, p2y, "arriba") ||
        choque_triangulo(550, 300, 700, 0, q1x, q1y, q2x, q2y, "arriba") ||
        choque_triangulo(550, 300, 700, 0, z1x, z1y, z2x, z2y, "arriba"))
      return true;

    // ===== TRIÁNGULOS DEL SUELO (base abajo, punta arriba) =====
    // Todos con tipo "abajo"

    // TRIÁNGULO 3: (250,500), (350,200), (450,500)
    // Izquierdo: (250,500) a (350,200)
    if (choque_triangulo(250, 500, 350, 200, r1x, r1y, r2x, r2y, "abajo") ||
        choque_triangulo(250, 500, 350, 200, b1x, b1y, b2x, b2y, "abajo") ||
        choque_triangulo(250, 500, 350, 200, p1x, p1y, p2x, p2y, "abajo") ||
        choque_triangulo(250, 500, 350, 200, q1x, q1y, q2x, q2y, "abajo") ||
        choque_triangulo(250, 500, 350, 200, z1x, z1y, z2x, z2y, "abajo"))
      return true;

    // Derecho: (350,200) a (450,500)
    if (choque_triangulo(350, 200, 450, 500, r1x, r1y, r2x, r2y, "abajo") ||
        choque_triangulo(350, 200, 450, 500, b1x, b1y, b2x, b2y, "abajo") ||
        choque_triangulo(350, 200, 450, 500, p1x, p1y, p2x, p2y, "abajo") ||
        choque_triangulo(350, 200, 450, 500, q1x, q1y, q2x, q2y, "abajo") ||
        choque_triangulo(350, 200, 450, 500, z1x, z1y, z2x, z2y, "abajo"))
      return true;

    // TRIÁNGULO 4: (550,500), (650,250), (740,500)
    // Izquierdo: (550,500) a (650,250)
    if (choque_triangulo(550, 500, 650, 250, r1x, r1y, r2x, r2y, "abajo") ||
        choque_triangulo(550, 500, 650, 250, b1x, b1y, b2x, b2y, "abajo") ||
        choque_triangulo(550, 500, 650, 250, p1x, p1y, p2x, p2y, "abajo") ||
        choque_triangulo(550, 500, 650, 250, q1x, q1y, q2x, q2y, "abajo") ||
        choque_triangulo(550, 500, 650, 250, z1x, z1y, z2x, z2y, "abajo"))
      return true;

    // Derecho: (650,250) a (740,500)
    if (choque_triangulo(650, 250, 740, 500, r1x, r1y, r2x, r2y, "abajo") ||
        choque_triangulo(650, 250, 740, 500, b1x, b1y, b2x, b2y, "abajo") ||
        choque_triangulo(650, 250, 740, 500, p1x, p1y, p2x, p2y, "abajo") ||
        choque_triangulo(650, 250, 740, 500, q1x, q1y, q2x, q2y, "abajo") ||
        choque_triangulo(650, 250, 740, 500, z1x, z1y, z2x, z2y, "abajo"))
      return true;
  }
  if (num_nivel == 5) {
    // =============================================
    // TECHO - Triángulos con base ARRIBA (y=0), punta ABAJO
    // Todos con tipo "arriba"
    // =============================================

    // --- TRIÁNGULO 1: (100,0), (200,250), (300,0) ---
    // Mitad izquierda: hipotenusa de (100,0) a (200,250) - pendiente positiva
    if (choque_triangulo(100, 0, 200, 250, r1x, r1y, r2x, r2y, "arriba") ||
        choque_triangulo(100, 0, 200, 250, b1x, b1y, b2x, b2y, "arriba") ||
        choque_triangulo(100, 0, 200, 250, p1x, p1y, p2x, p2y, "arriba") ||
        choque_triangulo(100, 0, 200, 250, q1x, q1y, q2x, q2y, "arriba") ||
        choque_triangulo(100, 0, 200, 250, z1x, z1y, z2x, z2y, "arriba"))
      return true;

    // Mitad derecha: hipotenusa de (200,250) a (300,0) - pendiente negativa
    if (choque_triangulo(200, 250, 300, 0, r1x, r1y, r2x, r2y, "arriba") ||
        choque_triangulo(200, 250, 300, 0, b1x, b1y, b2x, b2y, "arriba") ||
        choque_triangulo(200, 250, 300, 0, p1x, p1y, p2x, p2y, "arriba") ||
        choque_triangulo(200, 250, 300, 0, q1x, q1y, q2x, q2y, "arriba") ||
        choque_triangulo(200, 250, 300, 0, z1x, z1y, z2x, z2y, "arriba"))
      return true;

    // --- TRIÁNGULO 2: (400,0), (550,300), (700,0) ---
    // Mitad izquierda: hipotenusa de (400,0) a (550,300) - pendiente positiva
    if (choque_triangulo(400, 0, 550, 300, r1x, r1y, r2x, r2y, "arriba") ||
        choque_triangulo(400, 0, 550, 300, b1x, b1y, b2x, b2y, "arriba") ||
        choque_triangulo(400, 0, 550, 300, p1x, p1y, p2x, p2y, "arriba") ||
        choque_triangulo(400, 0, 550, 300, q1x, q1y, q2x, q2y, "arriba") ||
        choque_triangulo(400, 0, 550, 300, z1x, z1y, z2x, z2y, "arriba"))
      return true;

    // Mitad derecha: hipotenusa de (550,300) a (700,0) - pendiente negativa
    if (choque_triangulo(550, 300, 700, 0, r1x, r1y, r2x, r2y, "arriba") ||
        choque_triangulo(550, 300, 700, 0, b1x, b1y, b2x, b2y, "arriba") ||
        choque_triangulo(550, 300, 700, 0, p1x, p1y, p2x, p2y, "arriba") ||
        choque_triangulo(550, 300, 700, 0, q1x, q1y, q2x, q2y, "arriba") ||
        choque_triangulo(550, 300, 700, 0, z1x, z1y, z2x, z2y, "arriba"))
      return true;

    // =============================================
    // SUELO - Triángulos con base ABAJO (y=500), punta ARRIBA
    // Todos con tipo "abajo"
    // =============================================

    // --- TRIÁNGULO 3: (250,500), (350,100), (450,500) ---
    // Mitad izquierda: hipotenusa de (250,500) a (350,100) - pendiente negativa
    if (choque_triangulo(250, 500, 350, 100, r1x, r1y, r2x, r2y, "abajo") ||
        choque_triangulo(250, 500, 350, 100, b1x, b1y, b2x, b2y, "abajo") ||
        choque_triangulo(250, 500, 350, 100, p1x, p1y, p2x, p2y, "abajo") ||
        choque_triangulo(250, 500, 350, 100, q1x, q1y, q2x, q2y, "abajo") ||
        choque_triangulo(250, 500, 350, 100, z1x, z1y, z2x, z2y, "abajo"))
      return true;

    // Mitad derecha: hipotenusa de (350,100) a (450,500) - pendiente positiva
    if (choque_triangulo(350, 100, 450, 500, r1x, r1y, r2x, r2y, "abajo") ||
        choque_triangulo(350, 100, 450, 500, b1x, b1y, b2x, b2y, "abajo") ||
        choque_triangulo(350, 100, 450, 500, p1x, p1y, p2x, p2y, "abajo") ||
        choque_triangulo(350, 100, 450, 500, q1x, q1y, q2x, q2y, "abajo") ||
        choque_triangulo(350, 100, 450, 500, z1x, z1y, z2x, z2y, "abajo"))
      return true;

    // --- TRIÁNGULO 4: (550,500), (650,250), (740,500) ---
    // Mitad izquierda: hipotenusa de (550,500) a (650,250) - pendiente negativa
    if (choque_triangulo(550, 500, 650, 250, r1x, r1y, r2x, r2y, "abajo") ||
        choque_triangulo(550, 500, 650, 250, b1x, b1y, b2x, b2y, "abajo") ||
        choque_triangulo(550, 500, 650, 250, p1x, p1y, p2x, p2y, "abajo") ||
        choque_triangulo(550, 500, 650, 250, q1x, q1y, q2x, q2y, "abajo") ||
        choque_triangulo(550, 500, 650, 250, z1x, z1y, z2x, z2y, "abajo"))
      return true;

    // Mitad derecha: hipotenusa de (650,250) a (740,500) - pendiente positiva
    if (choque_triangulo(650, 250, 740, 500, r1x, r1y, r2x, r2y, "abajo") ||
        choque_triangulo(650, 250, 740, 500, b1x, b1y, b2x, b2y, "abajo") ||
        choque_triangulo(650, 250, 740, 500, p1x, p1y, p2x, p2y, "abajo") ||
        choque_triangulo(650, 250, 740, 500, q1x, q1y, q2x, q2y, "abajo") ||
        choque_triangulo(650, 250, 740, 500, z1x, z1y, z2x, z2y, "abajo"))
      return true;
  }

  // ===== NIVEL 6 =====
  if (num_nivel == 6) {
    // =============================================
    // TECHO - Triángulos con base ARRIBA (y=0), punta ABAJO
    // Todos con tipo "arriba" (igual que nivel 5)
    // =============================================

    // --- TRIÁNGULO 1: (100,0), (200,250), (300,0) ---
    if (choque_triangulo(100, 0, 200, 250, r1x, r1y, r2x, r2y, "arriba") ||
        choque_triangulo(100, 0, 200, 250, b1x, b1y, b2x, b2y, "arriba") ||
        choque_triangulo(100, 0, 200, 250, p1x, p1y, p2x, p2y, "arriba") ||
        choque_triangulo(100, 0, 200, 250, q1x, q1y, q2x, q2y, "arriba") ||
        choque_triangulo(100, 0, 200, 250, z1x, z1y, z2x, z2y, "arriba"))
      return true;

    if (choque_triangulo(200, 250, 300, 0, r1x, r1y, r2x, r2y, "arriba") ||
        choque_triangulo(200, 250, 300, 0, b1x, b1y, b2x, b2y, "arriba") ||
        choque_triangulo(200, 250, 300, 0, p1x, p1y, p2x, p2y, "arriba") ||
        choque_triangulo(200, 250, 300, 0, q1x, q1y, q2x, q2y, "arriba") ||
        choque_triangulo(200, 250, 300, 0, z1x, z1y, z2x, z2y, "arriba"))
      return true;

    // --- TRIÁNGULO 2: (400,0), (550,300), (700,0) ---
    if (choque_triangulo(400, 0, 550, 300, r1x, r1y, r2x, r2y, "arriba") ||
        choque_triangulo(400, 0, 550, 300, b1x, b1y, b2x, b2y, "arriba") ||
        choque_triangulo(400, 0, 550, 300, p1x, p1y, p2x, p2y, "arriba") ||
        choque_triangulo(400, 0, 550, 300, q1x, q1y, q2x, q2y, "arriba") ||
        choque_triangulo(400, 0, 550, 300, z1x, z1y, z2x, z2y, "arriba"))
      return true;

    if (choque_triangulo(550, 300, 700, 0, r1x, r1y, r2x, r2y, "arriba") ||
        choque_triangulo(550, 300, 700, 0, b1x, b1y, b2x, b2y, "arriba") ||
        choque_triangulo(550, 300, 700, 0, p1x, p1y, p2x, p2y, "arriba") ||
        choque_triangulo(550, 300, 700, 0, q1x, q1y, q2x, q2y, "arriba") ||
        choque_triangulo(550, 300, 700, 0, z1x, z1y, z2x, z2y, "arriba"))
      return true;

    // =============================================
    // SUELO - Triángulos con base ABAJO (y=500), punta ARRIBA
    // Todos con tipo "abajo"
    // =============================================

    // --- TRIÁNGULO 3: (200,500), (350,65), (500,500) ---
    // Es más puntiagudo que en nivel 5 (la punta llega a y=65)

    // Mitad izquierda: hipotenusa de (200,500) a (350,65) - pendiente negativa
    if (choque_triangulo(200, 500, 350, 65, r1x, r1y, r2x, r2y, "abajo") ||
        choque_triangulo(200, 500, 350, 65, b1x, b1y, b2x, b2y, "abajo") ||
        choque_triangulo(200, 500, 350, 65, p1x, p1y, p2x, p2y, "abajo") ||
        choque_triangulo(200, 500, 350, 65, q1x, q1y, q2x, q2y, "abajo") ||
        choque_triangulo(200, 500, 350, 65, z1x, z1y, z2x, z2y, "abajo"))
      return true;

    // Mitad derecha: hipotenusa de (350,65) a (500,500) - pendiente positiva
    if (choque_triangulo(350, 65, 500, 500, r1x, r1y, r2x, r2y, "abajo") ||
        choque_triangulo(350, 65, 500, 500, b1x, b1y, b2x, b2y, "abajo") ||
        choque_triangulo(350, 65, 500, 500, p1x, p1y, p2x, p2y, "abajo") ||
        choque_triangulo(350, 65, 500, 500, q1x, q1y, q2x, q2y, "abajo") ||
        choque_triangulo(350, 65, 500, 500, z1x, z1y, z2x, z2y, "abajo"))
      return true;

    // --- TRIÁNGULO 4: (550,500), (650,250), (740,500) ---
    // Mitad izquierda: hipotenusa de (550,500) a (650,250) - pendiente negativa
    if (choque_triangulo(550, 500, 650, 250, r1x, r1y, r2x, r2y, "abajo") ||
        choque_triangulo(550, 500, 650, 250, b1x, b1y, b2x, b2y, "abajo") ||
        choque_triangulo(550, 500, 650, 250, p1x, p1y, p2x, p2y, "abajo") ||
        choque_triangulo(550, 500, 650, 250, q1x, q1y, q2x, q2y, "abajo") ||
        choque_triangulo(550, 500, 650, 250, z1x, z1y, z2x, z2y, "abajo"))
      return true;

    // Mitad derecha: hipotenusa de (650,250) a (740,500) - pendiente positiva
    if (choque_triangulo(650, 250, 740, 500, r1x, r1y, r2x, r2y, "abajo") ||
        choque_triangulo(650, 250, 740, 500, b1x, b1y, b2x, b2y, "abajo") ||
        choque_triangulo(650, 250, 740, 500, p1x, p1y, p2x, p2y, "abajo") ||
        choque_triangulo(650, 250, 740, 500, q1x, q1y, q2x, q2y, "abajo") ||
        choque_triangulo(650, 250, 740, 500, z1x, z1y, z2x, z2y, "abajo"))
      return true;
  }
  if (num_nivel == 7) {
    // ... tus colisiones con techo y suelo ...
    if (choque_triangulo(0, 0, 200, 150, r1x, r1y, r2x, r2y, "arriba") ||
        choque_triangulo(0, 0, 200, 150, b1x, b1y, b2x, b2y, "arriba") ||
        choque_triangulo(0, 0, 200, 150, p1x, p1y, p2x, p2y, "arriba") ||
        choque_triangulo(0, 0, 200, 150, q1x, q1y, q2x, q2y, "arriba") ||
        choque_triangulo(0, 0, 200, 150, z1x, z1y, z2x, z2y, "arriba"))
      return true;

    // Hipotenusa derecha: (200,150) a (400,0)
    if (choque_triangulo(200, 150, 400, 0, r1x, r1y, r2x, r2y, "arriba") ||
        choque_triangulo(200, 150, 400, 0, b1x, b1y, b2x, b2y, "arriba") ||
        choque_triangulo(200, 150, 400, 0, p1x, p1y, p2x, p2y, "arriba") ||
        choque_triangulo(200, 150, 400, 0, q1x, q1y, q2x, q2y, "arriba") ||
        choque_triangulo(200, 150, 400, 0, z1x, z1y, z2x, z2y, "arriba"))
      return true;

    // Techo 2: (400,0), (600,120), (800,0)
    // Hipotenusa izquierda: (400,0) a (600,120)
    if (choque_triangulo(400, 0, 600, 120, r1x, r1y, r2x, r2y, "arriba") ||
        choque_triangulo(400, 0, 600, 120, b1x, b1y, b2x, b2y, "arriba") ||
        choque_triangulo(400, 0, 600, 120, p1x, p1y, p2x, p2y, "arriba") ||
        choque_triangulo(400, 0, 600, 120, q1x, q1y, q2x, q2y, "arriba") ||
        choque_triangulo(400, 0, 600, 120, z1x, z1y, z2x, z2y, "arriba"))
      return true;

    // Hipotenusa derecha: (600,120) a (800,0)
    if (choque_triangulo(600, 120, 800, 0, r1x, r1y, r2x, r2y, "arriba") ||
        choque_triangulo(600, 120, 800, 0, b1x, b1y, b2x, b2y, "arriba") ||
        choque_triangulo(600, 120, 800, 0, p1x, p1y, p2x, p2y, "arriba") ||
        choque_triangulo(600, 120, 800, 0, q1x, q1y, q2x, q2y, "arriba") ||
        choque_triangulo(600, 120, 800, 0, z1x, z1y, z2x, z2y, "arriba"))
      return true;

    // ============================================
    // TRIÁNGULOS DEL SUELO (apuntan hacia arriba, tipo "abajo")
    // ============================================
    // Suelo 1: (100,500), (200,400), (400,500)
    // Hipotenusa izquierda: (100,500) a (200,400)
    if (choque_triangulo(100, 500, 200, 400, r1x, r1y, r2x, r2y, "abajo") ||
        choque_triangulo(100, 500, 200, 400, b1x, b1y, b2x, b2y, "abajo") ||
        choque_triangulo(100, 500, 200, 400, p1x, p1y, p2x, p2y, "abajo") ||
        choque_triangulo(100, 500, 200, 400, q1x, q1y, q2x, q2y, "abajo") ||
        choque_triangulo(100, 500, 200, 400, z1x, z1y, z2x, z2y, "abajo"))
      return true;

    // Hipotenusa derecha: (200,400) a (400,500)
    if (choque_triangulo(200, 400, 400, 500, r1x, r1y, r2x, r2y, "abajo") ||
        choque_triangulo(200, 400, 400, 500, b1x, b1y, b2x, b2y, "abajo") ||
        choque_triangulo(200, 400, 400, 500, p1x, p1y, p2x, p2y, "abajo") ||
        choque_triangulo(200, 400, 400, 500, q1x, q1y, q2x, q2y, "abajo") ||
        choque_triangulo(200, 400, 400, 500, z1x, z1y, z2x, z2y, "abajo"))
      return true;

    // Suelo 2: (400,500), (600,290), (800,500)
    // Hipotenusa izquierda: (400,500) a (600,290)
    if (choque_triangulo(400, 500, 600, 290, r1x, r1y, r2x, r2y, "abajo") ||
        choque_triangulo(400, 500, 600, 290, b1x, b1y, b2x, b2y, "abajo") ||
        choque_triangulo(400, 500, 600, 290, p1x, p1y, p2x, p2y, "abajo") ||
        choque_triangulo(400, 500, 600, 290, q1x, q1y, q2x, q2y, "abajo") ||
        choque_triangulo(400, 500, 600, 290, z1x, z1y, z2x, z2y, "abajo"))
      return true;

    // Hipotenusa derecha: (600,290) a (800,500)
    if (choque_triangulo(600, 290, 800, 500, r1x, r1y, r2x, r2y, "abajo") ||
        choque_triangulo(600, 290, 800, 500, b1x, b1y, b2x, b2y, "abajo") ||
        choque_triangulo(600, 290, 800, 500, p1x, p1y, p2x, p2y, "abajo") ||
        choque_triangulo(600, 290, 800, 500, q1x, q1y, q2x, q2y, "abajo") ||
        choque_triangulo(600, 290, 800, 500, z1x, z1y, z2x, z2y, "abajo"))
      return true;
    // Colisiones con rocas
    for (int i = 0; i < MAX_ROCAS; i++) {
      if (roca_activa[i]) {
        float rx = roca_x[i];
        float ry = roca_y[i];

        // Dividir la roca (triángulo isósceles) en dos triángulos rectángulos
        // Triángulo izquierdo: hipotenusa de (rx-12, ry-6) a (rx, ry+12)
        if (choque_triangulo(rx - 12, ry - 6, rx, ry + 12, r1x, r1y, r2x, r2y,
                             "abajo") ||
            choque_triangulo(rx - 12, ry - 6, rx, ry + 12, b1x, b1y, b2x, b2y,
                             "abajo") ||
            choque_triangulo(rx - 12, ry - 6, rx, ry + 12, p1x, p1y, p2x, p2y,
                             "abajo") ||
            choque_triangulo(rx - 12, ry - 6, rx, ry + 12, q1x, q1y, q2x, q2y,
                             "abajo") ||
            choque_triangulo(rx - 12, ry - 6, rx, ry + 12, z1x, z1y, z2x, z2y,
                             "abajo"))
          return true;

        // Triángulo derecho: hipotenusa de (rx, ry+12) a (rx+12, ry-6)
        if (choque_triangulo(rx, ry + 12, rx + 12, ry - 6, r1x, r1y, r2x, r2y,
                             "abajo") ||
            choque_triangulo(rx, ry + 12, rx + 12, ry - 6, b1x, b1y, b2x, b2y,
                             "abajo") ||
            choque_triangulo(rx, ry + 12, rx + 12, ry - 6, p1x, p1y, p2x, p2y,
                             "abajo") ||
            choque_triangulo(rx, ry + 12, rx + 12, ry - 6, q1x, q1y, q2x, q2y,
                             "abajo") ||
            choque_triangulo(rx, ry + 12, rx + 12, ry - 6, z1x, z1y, z2x, z2y,
                             "abajo"))
          return true;
      }
    }
  }
  if (num_nivel == 8) {
    if (choque_triangulo(110, 0, 640, 225, r1x, r1y, r2x, r2y, "arriba") ||
        choque_triangulo(110, 0, 640, 225, b1x, b1y, b2x, b2y, "arriba") ||
        choque_triangulo(110, 0, 640, 225, p1x, p1y, p2x, p2y, "arriba") ||
        choque_triangulo(110, 0, 640, 225, q1x, q1y, q2x, q2y, "arriba") ||
        choque_triangulo(110, 0, 640, 225, z1x, z1y, z2x, z2y, "arriba"))
      return true;

    // Triángulo superior diagonal 2 (base abajo, punta arriba) -> tipo "abajo"
    // Hipotenusa: (110,225) a (640,0)
    if (choque_triangulo(110, 225, 640, 0, r1x, r1y, r2x, r2y, "abajo") ||
        choque_triangulo(110, 225, 640, 0, b1x, b1y, b2x, b2y, "abajo") ||
        choque_triangulo(110, 225, 640, 0, p1x, p1y, p2x, p2y, "abajo") ||
        choque_triangulo(110, 225, 640, 0, q1x, q1y, q2x, q2y, "abajo") ||
        choque_triangulo(110, 225, 640, 0, z1x, z1y, z2x, z2y, "abajo"))
      return true;

    // -------------------------------------------------
    // RECTÁNGULO INFERIOR (y: 285 a 500, x: 110 a 640)
    // Dividido en dos triángulos mediante diagonales opuestas
    // -------------------------------------------------
    // Triángulo inferior diagonal 1 (base arriba, punta abajo) -> tipo "arriba"
    // Hipotenusa: (110,285) a (640,500)
    if (choque_triangulo(110, 285, 640, 500, r1x, r1y, r2x, r2y, "arriba") ||
        choque_triangulo(110, 285, 640, 500, b1x, b1y, b2x, b2y, "arriba") ||
        choque_triangulo(110, 285, 640, 500, p1x, p1y, p2x, p2y, "arriba") ||
        choque_triangulo(110, 285, 640, 500, q1x, q1y, q2x, q2y, "arriba") ||
        choque_triangulo(110, 285, 640, 500, z1x, z1y, z2x, z2y, "arriba"))
      return true;

    // Triángulo inferior diagonal 2 (base abajo, punta arriba) -> tipo "abajo"
    // Hipotenusa: (110,500) a (640,285)
    if (choque_triangulo(110, 500, 640, 285, r1x, r1y, r2x, r2y, "abajo") ||
        choque_triangulo(110, 500, 640, 285, b1x, b1y, b2x, b2y, "abajo") ||
        choque_triangulo(110, 500, 640, 285, p1x, p1y, p2x, p2y, "abajo") ||
        choque_triangulo(110, 500, 640, 285, q1x, q1y, q2x, q2y, "abajo") ||
        choque_triangulo(110, 500, 640, 285, z1x, z1y, z2x, z2y, "abajo"))
      return true;
  }
  if (num_nivel == 9) {
    if (choque_circulo(circulos_estaticos[0].cx, circulos_estaticos[0].cy,
                       circulos_estaticos[0].r_colision, r1x, r1y, r2x, r2y) ||
        choque_circulo(circulos_estaticos[0].cx, circulos_estaticos[0].cy,
                       circulos_estaticos[0].r_colision, b1x, b1y, b2x, b2y) ||
        choque_circulo(circulos_estaticos[0].cx, circulos_estaticos[0].cy,
                       circulos_estaticos[0].r_colision, p1x, p1y, p2x, p2y) ||
        choque_circulo(circulos_estaticos[0].cx, circulos_estaticos[0].cy,
                       circulos_estaticos[0].r_colision, q1x, q1y, q2x, q2y) ||
        choque_circulo(circulos_estaticos[0].cx, circulos_estaticos[0].cy,
                       circulos_estaticos[0].r_colision, z1x, z1y, z2x, z2y))
      return true;

    // Círculo 1
    if (choque_circulo(circulos_estaticos[1].cx, circulos_estaticos[1].cy,
                       circulos_estaticos[1].r_colision, r1x, r1y, r2x, r2y) ||
        choque_circulo(circulos_estaticos[1].cx, circulos_estaticos[1].cy,
                       circulos_estaticos[1].r_colision, b1x, b1y, b2x, b2y) ||
        choque_circulo(circulos_estaticos[1].cx, circulos_estaticos[1].cy,
                       circulos_estaticos[1].r_colision, p1x, p1y, p2x, p2y) ||
        choque_circulo(circulos_estaticos[1].cx, circulos_estaticos[1].cy,
                       circulos_estaticos[1].r_colision, q1x, q1y, q2x, q2y) ||
        choque_circulo(circulos_estaticos[1].cx, circulos_estaticos[1].cy,
                       circulos_estaticos[1].r_colision, z1x, z1y, z2x, z2y))
      return true;

    // Círculo 2
    if (choque_circulo(circulos_estaticos[2].cx, circulos_estaticos[2].cy,
                       circulos_estaticos[2].r_colision, r1x, r1y, r2x, r2y) ||
        choque_circulo(circulos_estaticos[2].cx, circulos_estaticos[2].cy,
                       circulos_estaticos[2].r_colision, b1x, b1y, b2x, b2y) ||
        choque_circulo(circulos_estaticos[2].cx, circulos_estaticos[2].cy,
                       circulos_estaticos[2].r_colision, p1x, p1y, p2x, p2y) ||
        choque_circulo(circulos_estaticos[2].cx, circulos_estaticos[2].cy,
                       circulos_estaticos[2].r_colision, q1x, q1y, q2x, q2y) ||
        choque_circulo(circulos_estaticos[2].cx, circulos_estaticos[2].cy,
                       circulos_estaticos[2].r_colision, z1x, z1y, z2x, z2y))
      return true;
  }
  if (num_nivel == 10) {

    if (circulos_moviles[0].activo) {
      if (choque_circulo(circulos_moviles[0].x, circulos_moviles[0].y,
                         circulos_moviles[0].r_colision, r1x, r1y, r2x, r2y) ||
          choque_circulo(circulos_moviles[0].x, circulos_moviles[0].y,
                         circulos_moviles[0].r_colision, b1x, b1y, b2x, b2y) ||
          choque_circulo(circulos_moviles[0].x, circulos_moviles[0].y,
                         circulos_moviles[0].r_colision, p1x, p1y, p2x, p2y) ||
          choque_circulo(circulos_moviles[0].x, circulos_moviles[0].y,
                         circulos_moviles[0].r_colision, q1x, q1y, q2x, q2y) ||
          choque_circulo(circulos_moviles[0].x, circulos_moviles[0].y,
                         circulos_moviles[0].r_colision, z1x, z1y, z2x, z2y))
        return true;
    }
    // Círculo móvil 1
    if (circulos_moviles[1].activo) {
      if (choque_circulo(circulos_moviles[1].x, circulos_moviles[1].y,
                         circulos_moviles[1].r_colision, r1x, r1y, r2x, r2y) ||
          choque_circulo(circulos_moviles[1].x, circulos_moviles[1].y,
                         circulos_moviles[1].r_colision, b1x, b1y, b2x, b2y) ||
          choque_circulo(circulos_moviles[1].x, circulos_moviles[1].y,
                         circulos_moviles[1].r_colision, p1x, p1y, p2x, p2y) ||
          choque_circulo(circulos_moviles[1].x, circulos_moviles[1].y,
                         circulos_moviles[1].r_colision, q1x, q1y, q2x, q2y) ||
          choque_circulo(circulos_moviles[1].x, circulos_moviles[1].y,
                         circulos_moviles[1].r_colision, z1x, z1y, z2x, z2y))
        return true;
    }
    // Círculo móvil 2
    if (circulos_moviles[2].activo) {
      if (choque_circulo(circulos_moviles[2].x, circulos_moviles[2].y,
                         circulos_moviles[2].r_colision, r1x, r1y, r2x, r2y) ||
          choque_circulo(circulos_moviles[2].x, circulos_moviles[2].y,
                         circulos_moviles[2].r_colision, b1x, b1y, b2x, b2y) ||
          choque_circulo(circulos_moviles[2].x, circulos_moviles[2].y,
                         circulos_moviles[2].r_colision, p1x, p1y, p2x, p2y) ||
          choque_circulo(circulos_moviles[2].x, circulos_moviles[2].y,
                         circulos_moviles[2].r_colision, q1x, q1y, q2x, q2y) ||
          choque_circulo(circulos_moviles[2].x, circulos_moviles[2].y,
                         circulos_moviles[2].r_colision, z1x, z1y, z2x, z2y))
        return true;
    }
    // Círculo móvil 3
    if (circulos_moviles[3].activo) {
      if (choque_circulo(circulos_moviles[3].x, circulos_moviles[3].y,
                         circulos_moviles[3].r_colision, r1x, r1y, r2x, r2y) ||
          choque_circulo(circulos_moviles[3].x, circulos_moviles[3].y,
                         circulos_moviles[3].r_colision, b1x, b1y, b2x, b2y) ||
          choque_circulo(circulos_moviles[3].x, circulos_moviles[3].y,
                         circulos_moviles[3].r_colision, p1x, p1y, p2x, p2y) ||
          choque_circulo(circulos_moviles[3].x, circulos_moviles[3].y,
                         circulos_moviles[3].r_colision, q1x, q1y, q2x, q2y) ||
          choque_circulo(circulos_moviles[3].x, circulos_moviles[3].y,
                         circulos_moviles[3].r_colision, z1x, z1y, z2x, z2y))
        return true;
    }
    // Círculo móvil 4
    if (circulos_moviles[4].activo) {
      if (choque_circulo(circulos_moviles[4].x, circulos_moviles[4].y,
                         circulos_moviles[4].r_colision, r1x, r1y, r2x, r2y) ||
          choque_circulo(circulos_moviles[4].x, circulos_moviles[4].y,
                         circulos_moviles[4].r_colision, b1x, b1y, b2x, b2y) ||
          choque_circulo(circulos_moviles[4].x, circulos_moviles[4].y,
                         circulos_moviles[4].r_colision, p1x, p1y, p2x, p2y) ||
          choque_circulo(circulos_moviles[4].x, circulos_moviles[4].y,
                         circulos_moviles[4].r_colision, q1x, q1y, q2x, q2y) ||
          choque_circulo(circulos_moviles[4].x, circulos_moviles[4].y,
                         circulos_moviles[4].r_colision, z1x, z1y, z2x, z2y))
        return true;
    }
    // Círculo móvil 5
    if (circulos_moviles[5].activo) {
      if (choque_circulo(circulos_moviles[5].x, circulos_moviles[5].y,
                         circulos_moviles[5].r_colision, r1x, r1y, r2x, r2y) ||
          choque_circulo(circulos_moviles[5].x, circulos_moviles[5].y,
                         circulos_moviles[5].r_colision, b1x, b1y, b2x, b2y) ||
          choque_circulo(circulos_moviles[5].x, circulos_moviles[5].y,
                         circulos_moviles[5].r_colision, p1x, p1y, p2x, p2y) ||
          choque_circulo(circulos_moviles[5].x, circulos_moviles[5].y,
                         circulos_moviles[5].r_colision, q1x, q1y, q2x, q2y) ||
          choque_circulo(circulos_moviles[5].x, circulos_moviles[5].y,
                         circulos_moviles[5].r_colision, z1x, z1y, z2x, z2y))
        return true;
    }
    // Círculo móvil 6
    if (circulos_moviles[6].activo) {
      if (choque_circulo(circulos_moviles[6].x, circulos_moviles[6].y,
                         circulos_moviles[6].r_colision, r1x, r1y, r2x, r2y) ||
          choque_circulo(circulos_moviles[6].x, circulos_moviles[6].y,
                         circulos_moviles[6].r_colision, b1x, b1y, b2x, b2y) ||
          choque_circulo(circulos_moviles[6].x, circulos_moviles[6].y,
                         circulos_moviles[6].r_colision, p1x, p1y, p2x, p2y) ||
          choque_circulo(circulos_moviles[6].x, circulos_moviles[6].y,
                         circulos_moviles[6].r_colision, q1x, q1y, q2x, q2y) ||
          choque_circulo(circulos_moviles[6].x, circulos_moviles[6].y,
                         circulos_moviles[6].r_colision, z1x, z1y, z2x, z2y))
        return true;
    }
    // Círculo móvil 7
    if (circulos_moviles[7].activo) {
      if (choque_circulo(circulos_moviles[7].x, circulos_moviles[7].y,
                         circulos_moviles[7].r_colision, r1x, r1y, r2x, r2y) ||
          choque_circulo(circulos_moviles[7].x, circulos_moviles[7].y,
                         circulos_moviles[7].r_colision, b1x, b1y, b2x, b2y) ||
          choque_circulo(circulos_moviles[7].x, circulos_moviles[7].y,
                         circulos_moviles[7].r_colision, p1x, p1y, p2x, p2y) ||
          choque_circulo(circulos_moviles[7].x, circulos_moviles[7].y,
                         circulos_moviles[7].r_colision, q1x, q1y, q2x, q2y) ||
          choque_circulo(circulos_moviles[7].x, circulos_moviles[7].y,
                         circulos_moviles[7].r_colision, z1x, z1y, z2x, z2y))
        return true;
    }
  }
  return false;
}

// Función auxiliar: producto cruz para orientación
float cross(float ox, float oy, float px, float py, float qx, float qy) {
  return (px - ox) * (qy - oy) - (py - oy) * (qx - ox);
}

// Verifica si el punto (px, py) está dentro del triángulo (ax,ay), (bx,by),
// (cx,cy)
bool punto_en_triangulo(float px, float py, float ax, float ay, float bx,
                        float by, float cx, float cy) {
  float d1 = cross(ax, ay, bx, by, px, py);
  float d2 = cross(bx, by, cx, cy, px, py);
  float d3 = cross(cx, cy, ax, ay, px, py);
  bool tiene_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
  bool tiene_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
  return !(tiene_neg && tiene_pos); // Todos los signos iguales (o cero)
}

// Verifica si dos segmentos AB y CD se intersectan (incluyendo endpoints)
bool segmentos_intersectan(float ax, float ay, float bx, float by, float cx,
                           float cy, float dx, float dy) {
  float o1 = cross(ax, ay, bx, by, cx, cy);
  float o2 = cross(ax, ay, bx, by, dx, dy);
  float o3 = cross(cx, cy, dx, dy, ax, ay);
  float o4 = cross(cx, cy, dx, dy, bx, by);

  if (o1 == 0 && o2 == 0 && o3 == 0 && o4 == 0) {
    // Colineales: verificar si se superponen
    if (min(ax, bx) > max(cx, dx) || min(cx, dx) > max(ax, bx))
      return false;
    if (min(ay, by) > max(cy, dy) || min(cy, dy) > max(ay, by))
      return false;
    return true;
  }
  return (o1 * o2 <= 0) && (o3 * o4 <= 0);
}

bool choque_triangulo(float x1, float y1, float x2, float y2, float p1x,
                      float p1y, float p2x, float p2y, string tipo_triangulo) {

  // Asegurar que x1 < x2 para facilitar (la función se llama con hipotenusa de
  // izquierda a derecha)
  if (x1 > x2) {
    swap(x1, x2);
    swap(y1, y2);
  }

  float m = (y2 - y1) / (x2 - x1);
  float ax, ay, bx, by, cx, cy; // Vértices del triángulo

  // Determinar los tres vértices según la pendiente y el tipo
  if (tipo_triangulo == "arriba") { // Base arriba, punta abajo
    if (m > 0) { // Pendiente positiva: (x1,y1) es base izquierda, (x2,y2) es
                 // punta derecha
      ax = x1;
      ay = y1; // base izquierda
      bx = x2;
      by = y1; // base derecha (misma y que base izquierda)
      cx = x2;
      cy = y2; // punta derecha
    } else {   // m < 0: (x2,y2) es base derecha, (x1,y1) es punta izquierda
      ax = x1;
      ay = y2; // base izquierda (misma y que base derecha)
      bx = x2;
      by = y2; // base derecha
      cx = x1;
      cy = y1; // punta izquierda
    }
  } else {       // tipo "abajo": base abajo, punta arriba
    if (m > 0) { // Pendiente positiva: (x1,y1) es punta izquierda, (x2,y2) es
                 // base derecha
      ax = x1;
      ay = y1; // punta izquierda
      bx = x1;
      by = y2; // base izquierda (misma x que punta)
      cx = x2;
      cy = y2; // base derecha
    } else {   // m < 0: (x2,y2) es punta derecha, (x1,y1) es base izquierda
      ax = x1;
      ay = y1; // base izquierda
      bx = x2;
      by = y1; // base derecha (misma y que base izquierda)
      cx = x2;
      cy = y2; // punta derecha
    }
  }

  if (punto_en_triangulo(p1x, p1y, ax, ay, bx, by, cx, cy) ||
      punto_en_triangulo(p2x, p2y, ax, ay, bx, by, cx, cy))
    return true;

  if (segmentos_intersectan(p1x, p1y, p2x, p2y, ax, ay, bx, by))
    return true;
  if (segmentos_intersectan(p1x, p1y, p2x, p2y, bx, by, cx, cy))
    return true;
  if (segmentos_intersectan(p1x, p1y, p2x, p2y, cx, cy, ax, ay))
    return true;

  return false;
}
bool aterrizar(float cx, float cy, float vx, float vy, BITMAP *buffer,
               int num_nivel) {
  if (cy + 20 >= 450 && cx - 20 >= 10 && cx + 20 <= 100) {
    if (vy < 1) {
      return true;
    } else {
    }
  }
  return false;
}

bool game_over(float &cx, float &cy, float &vx, float &vy, int &num_nivel,
               float &combustible, BITMAP *buffer) {
  if (cx + 20 >= 740 || cx - 20 <= 0 || cy - 15 <= 0 || cy + 20 >= 500) {
    explocion(cx, cy, buffer, num_nivel, vx, vy);
    reiniciar_nivel(cx, cy, vx, vy, combustible, num_nivel);
    fps_counter = 0;
    nivel_y = -30;
    nivel_timer = 1500;
    // return true;
  }
  if (choque_nave(num_nivel, cx, cy) == true) {
    explocion(cx, cy, buffer, num_nivel, vx, vy);
    reiniciar_nivel(cx, cy, vx, vy, combustible, num_nivel);
    fps_counter = 0;
    nivel_y = -30;
    nivel_timer = 1500;
  } else if (cy + 20 >= 450 && cx - 20 >= 10 && cx + 20 <= 100 && vy > 0.09) {
    explocion(cx, cy, buffer, num_nivel, vx, vy);
    reiniciar_nivel(cx, cy, vx, vy, combustible, num_nivel);
    fps_counter = 0;
    nivel_y = -30;
    nivel_timer = 1500;
  }

  else if (cy + 20 >= 450 && (cx + 20 > 10 && cx - 20 < 100)) {

    if (cx - 20 < 10 || cx + 20 > 100) {
      explocion(cx, cy, buffer, num_nivel, vx, vy);
      reiniciar_nivel(cx, cy, vx, vy, combustible, num_nivel);
      nivel_y = -30;
      nivel_timer = 1500;
    }
  }
  return false;
}
void explocion(float cx, float cy, BITMAP *buffer, int num_nivel, float vx,
               float vy) {
  iniciar_circulos_moviles();

  if (musica_fondo != NULL) {
    stop_sample(musica_fondo);
  }
  if (sfx_motor)
    adjust_sample(sfx_motor, 0, 128, 1000, 1);
  motor_encendido = false;
  if (sfx_explosion) {
    play_sample(sfx_explosion, 255, 128, 1000,
                0); // 255 es volumen máximo, 0 es NO loop
  }
  float x[12] = {cx - 10, cx + 10, cx,      cx,     cx + 15, cx - 15,
                 cx + 5,  cx - 10, cx + 10, cx - 5, cx - 10, cx + 10};
  float y[12] = {cy,     cy,      cy - 15, cy + 15, cy - 15, cy + 15,
                 cy + 5, cy - 10, cy - 10, cy + 10, cy,      cy};
  float dx[6] = {1, 1, 0, -1, -1, 0};
  float dy[6] = {0, -1, -1, -1, -0, 1};
  clear(screen);
  do {
    clear(buffer);
    pintar_nivel(num_nivel, buffer, cx, cy, vx, vy);
    int j = 0;
    for (int i = 0; i < 10; i += 2) {
      line(buffer, x[i], y[i], x[i + 1], y[i + 1], 0xFFFFFF);
      rotar(x[i + 1], y[i + 1], x[i], y[i], 1);
      x[i] += dx[j];
      y[i] += dy[j];
      x[i + 1] += dx[j];
      y[i + 1] += dy[j];
      j++;
    }
    textout_centre_ex(buffer, font, "PRESS (A) FOR TRY AGAIN", 370, 240,
                      0xFBFF00, 0x000000);
    textout_centre_ex(buffer, font, "PRESS (ESC) TO EXIT", 370, 250, 0xFBFF00,
                      0x000000);
    blit(buffer, screen, 0, 0, 0, 0, 740, 500);
    rest(2);
  } while (!key[KEY_ESC] && !key[KEY_A]);
  if (key[KEY_A] && musica_fondo != NULL) {
    play_sample(musica_fondo, 160, 128, 1000, 1);
  }
}
void pintar_nivel(int num_nivel, BITMAP *buffer, float &cx, float &cy,
                  float &vx, float &vy) {

  // Transparencia del texto (0 a 255)
  if (num_nivel == 1) {
    rectfill(buffer, 10, 450, 100, 500, 0x999999);
  }
  if (num_nivel == 2) {
    triangle(buffer, 110, 100, 300, 500, 110, 500, 0x999999);
    triangle(buffer, 500, 500, 600, 300, 600, 500, 0x999999);
    triangle(buffer, 600, 300, 800, 500, 600, 500, 0x999999);
    triangle(buffer, 200, 0, 400, 350, 400, 0, 0x999999);
    rectfill(buffer, 10, 450, 100, 500, 0x999999);
  }
  if (num_nivel == 3) {
    triangle(buffer, 110, 300, 300, 500, 110, 500, 0x999999);
    triangle(buffer, 305, 300, 510, 500, 310, 500, 0x999999);
    triangle(buffer, 400, 000, 600, 400, 600, 000, 0x999999);
    triangle(buffer, 100, 0, 250, 200, 250, 0, 0x999999);
    rectfill(buffer, 10, 450, 100, 500, 0x999999);
  }
  if (num_nivel == 4) {
    int c_roca = 0x999999;

    triangle(buffer, 100, 0, 200, 250, 300, 0, c_roca);

    // Pico 2: En el medio
    triangle(buffer, 400, 0, 550, 300, 700, 0, c_roca);

    triangle(buffer, 250, 500, 350, 200, 450, 500, c_roca);

    triangle(buffer, 550, 500, 650, 250, 740, 500, c_roca);

    rectfill(buffer, 10, 450, 100, 500, c_roca);
  }
  if (num_nivel == 5) {
    int c_roca = 0x999999;

    triangle(buffer, 100, 0, 200, 250, 300, 0, c_roca);

    triangle(buffer, 400, 0, 550, 300, 700, 0, c_roca);

    triangle(buffer, 250, 500, 350, 100, 450, 500, c_roca);

    triangle(buffer, 550, 500, 650, 250, 740, 500, c_roca);

    rectfill(buffer, 10, 450, 100, 500, c_roca);
  }
  if (num_nivel == 6) {
    int c_roca = 0x999999;

    triangle(buffer, 100, 0, 200, 250, 300, 0, c_roca);

    triangle(buffer, 400, 0, 550, 300, 700, 0, c_roca);

    triangle(buffer, 200, 500, 350, 65, 500, 500, c_roca);

    triangle(buffer, 550, 500, 650, 250, 740, 500, c_roca);

    rectfill(buffer, 10, 450, 100, 500, c_roca);
  }
  if (num_nivel == 7) {
    int c_roca = 0x999999;
    dibujar_triangulos(buffer);
    triangle(buffer, 0, 0, 200, 150, 400, 0, c_roca);
    triangle(buffer, 400, 0, 600, 120, 800, 0, c_roca);

    triangle(buffer, 100, 500, 200, 400, 400, 500, c_roca);
    triangle(buffer, 400, 500, 600, 290, 800, 500, c_roca);
    rectfill(buffer, 10, 450, 100, 500, c_roca);
  }
  if (num_nivel == 8) {
    int c_roca = 0x999999;
    rectfill(buffer, 110, 0, 640, 225, c_roca);
    // Rectángulo inferior (obstáculo)
    rectfill(buffer, 110, 285, 640, 500, c_roca);
    rectfill(buffer, 10, 450, 100, 500, 0x9999999);
  }
  if (num_nivel == 9) {
    iniciar_circulos_estaticos();
    aplicar_gravedad_circulos_estaticos(cx, cy, vx, vy);
    dibujar_circulos(buffer);
    rectfill(buffer, 10, 450, 100, 500, 0x9999999);
  }
  if (num_nivel == 10) {
    if (!circulos_inicializados) {
      iniciar_circulos_moviles();

      circulos_inicializados = true;
    }
    dibujar_circulos_moviles(buffer);
    aplicar_gravedad_circulos_moviles(cx, cy, vx, vy);
    rectfill(buffer, 10, 450, 100, 500, 0x9999999);
  }
}
void medidor_combustible(float combustible, BITMAP *buffer) {
  textout_centre_ex(buffer, font, "Combustible", 100, 30, 0x999999, 0x000000);
  rectfill(buffer, 50, 50, 50 + 100, 55, 0x000000);
  rectfill(buffer, 50, 50, 50 + combustible, 55, 0x999999);
}
void pintar_motor(float da, float cx, float cy, BITMAP *buffer) {
  float c1, c2;
  c1 = cx;
  c2 = cy;
  if (da != 0)
    c2 += 14;
  float fuego[14] = {c1 - 5,  c2 + 5,  c1 - 10, c2 + 20, c1 - 5,
                     c2 + 20, c1,      c2 + 35, c1 + 5,  c2 + 20,
                     c1 + 10, c2 + 20, c1 + 5,  c2 + 5};
  for (int i = 0; i <= 12; i += 2) {
    rotar(fuego[i], fuego[i + 1], cx, cy, da);
  }
  for (int i = 0; i < 12; i += 2) {
    line(buffer, fuego[i], fuego[i + 1], fuego[i + 2], fuego[i + 3],
         makecol(255, 255, 0));
  }
}
void aceleracion(float da, float &vx, float &vy) {
  float ax = 0, ay = -0.09;
  rotar(ax, ay, 0, 0, da);
  vx = vx + ax;
  vy = vy + ay;
}
void pintar_nave(float cx, float cy, BITMAP *buffer) {
  float nav[26] = {cx - 20, cy + 20, cx - 20, cy + 10, cx - 10, cy,
                   cx - 10, cy - 10, cx,      cy - 15, cx + 10, cy - 10,
                   cx + 10, cy,      cx + 20, cy + 10, cx + 20, cy + 20,
                   cx - 10, cy,      cx + 10, cy};
  for (int i = 0; i <= 14; i += 2) {
    line(buffer, nav[i], nav[i + 1], nav[i + 2], nav[i + 3], 0x999999);
    if (i == 14)
      line(buffer, nav[i + 4], nav[i + 5], nav[i + 6], nav[i + 7], 0x999999);
  }
}

void mover_nave(float &cx, float &cy, float &vx, float &vy) {
  float ax = 0.000000, ay = 0.05;
  vx = vx + ax;
  vy = vy + ay;

  float limite = 4.0;
  if (vx > limite)
    vx = limite;
  if (vx < -limite)
    vx = -limite;
  if (vy > limite)
    vy = limite;
  if (vy < -limite)
    vy = -limite;

  cx = cx + vx;
  cy = cy + vy;
}
void rotar(float &x, float &y, float cx, float cy, float da) {
  float dx = x - cx;
  float dy = y - cy;
  float r = sqrt(dx * dx + dy * dy);
  float a = atan2(dy, dx);

  float da_rad = da / 180 * M_PI;
  a = a - da_rad;
  x = cx + r * cos(a);
  y = cy + r * sin(a);
}
