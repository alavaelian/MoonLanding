"""
MoonLanding - Lunar lander game
Port de C++/Allegro 5 a Python/pygame
10 niveles con fisica newtoniana, colisiones geometricas, circulos con gravedad.
"""
import pygame
import math
import random

# ─────────────────────────────────────────────
# CONSTANTES
# ─────────────────────────────────────────────
W, H = 740, 500
FPS = 60

# Colores
COL_ROCA     = (0x99, 0x99, 0x99)
COL_NEGRO    = (0, 0, 0)
COL_BLANCO   = (255, 255, 255)
COL_AMARILLO = (255, 255, 0)
COL_CYAN     = (0, 255, 255)
COL_VERDE    = (0, 255, 128)
COL_HUD      = (0xFB, 0xFF, 0x00)

# Estados
CLICK_TO_START   = 0
SPLASH_ESPERA    = 1
LIMPIEZA         = 2
JUGANDO          = 3
NIVEL_COMPLETADO = 4
EXPLOSION        = 5
VICTORIA         = 6

# ─────────────────────────────────────────────
# ESTADO GLOBAL DEL JUEGO
# ─────────────────────────────────────────────
class Juego:
    def __init__(self):
        self.estado = CLICK_TO_START
        self.cx, self.cy = 710, 100
        self.vx, self.vy = 0.0, 0.0
        self.combustible = 100.0
        self.num_nivel = 1
        self.splash_parpadeo = 0
        self.nivel_y = -30
        self.nivel_timer = 0
        self.parpadeo_victoria = 0
        self.parpadeo_nivel = 0
        self.nave_victoria_y = 500.0
        self.motor_encendido = False
        self.running = True

        # Explosion
        self.expl_x = [0.0]*12
        self.expl_y = [0.0]*12
        self.expl_dx = [1, 1, 0, -1, -1, 0]
        self.expl_dy = [0, -1, -1, -1, 0, 1]

        # Estrellas
        self.estrellas = []

        # Rocas (nivel 7)
        self.MAX_ROCAS = 15
        self.roca_x = [0.0]*self.MAX_ROCAS
        self.roca_y = [0.0]*self.MAX_ROCAS
        self.roca_vy = [0.0]*self.MAX_ROCAS
        self.roca_activa = [False]*self.MAX_ROCAS
        self.roca_timer = [30 + i*20 for i in range(self.MAX_ROCAS)]
        self.rocas_init = False

        # Circulos estaticos (nivel 9)
        self.circulos_estaticos = []

        # Circulos moviles (nivel 10)
        self.MAX_CIRCULOS_MOVILES = 8
        self.circulos_moviles = []
        self.circulos_inicializados = False

        # Para parpadeo combustible y system check
        self.parpadeo_comb = 0
        self.sys_timer = 0
        self.sys_temp = 42
        self.sys_psi = 14
        self.sys_o2 = 98
        self.sys_pwr = 87

g = Juego()

# ─────────────────────────────────────────────
# FISICA Y GEOMETRIA
# ─────────────────────────────────────────────
def rotar(x, y, rcx, rcy, da):
    """Rota el punto (x,y) alrededor de (rcx,rcy) por da grados. Devuelve (x,y)."""
    dx, dy = x - rcx, y - rcy
    r = math.sqrt(dx*dx + dy*dy)
    a = math.atan2(dy, dx)
    a -= da / 180.0 * math.pi
    return rcx + r * math.cos(a), rcy + r * math.sin(a)

def cross_prod(ox, oy, px, py, qx, qy):
    return (px-ox)*(qy-oy) - (py-oy)*(qx-ox)

def punto_en_triangulo(px, py, ax, ay, bx, by, tcx, tcy):
    d1 = cross_prod(ax, ay, bx, by, px, py)
    d2 = cross_prod(bx, by, tcx, tcy, px, py)
    d3 = cross_prod(tcx, tcy, ax, ay, px, py)
    neg = (d1 < 0) or (d2 < 0) or (d3 < 0)
    pos = (d1 > 0) or (d2 > 0) or (d3 > 0)
    return not (neg and pos)

def segmentos_intersectan(ax, ay, bx, by, scx, scy, dx, dy):
    o1 = cross_prod(ax, ay, bx, by, scx, scy)
    o2 = cross_prod(ax, ay, bx, by, dx, dy)
    o3 = cross_prod(scx, scy, dx, dy, ax, ay)
    o4 = cross_prod(scx, scy, dx, dy, bx, by)
    if o1 == 0 and o2 == 0 and o3 == 0 and o4 == 0:
        if min(ax, bx) > max(scx, dx) or min(scx, dx) > max(ax, bx):
            return False
        if min(ay, by) > max(scy, dy) or min(scy, dy) > max(ay, by):
            return False
        return True
    return (o1*o2 <= 0) and (o3*o4 <= 0)

def choque_triangulo(x1, y1, x2, y2, p1x, p1y, p2x, p2y, tipo):
    if x1 > x2:
        x1, x2 = x2, x1
        y1, y2 = y2, y1
    m = (y2 - y1) / (x2 - x1) if (x2 - x1) != 0 else 0
    if tipo == "arriba":
        if m > 0:
            ax, ay, bx, by, tcx, tcy = x1, y1, x2, y1, x2, y2
        else:
            ax, ay, bx, by, tcx, tcy = x1, y2, x2, y2, x1, y1
    else:
        if m > 0:
            ax, ay, bx, by, tcx, tcy = x1, y1, x1, y2, x2, y2
        else:
            ax, ay, bx, by, tcx, tcy = x1, y1, x2, y1, x2, y2
    if punto_en_triangulo(p1x, p1y, ax, ay, bx, by, tcx, tcy): return True
    if punto_en_triangulo(p2x, p2y, ax, ay, bx, by, tcx, tcy): return True
    if segmentos_intersectan(p1x, p1y, p2x, p2y, ax, ay, bx, by): return True
    if segmentos_intersectan(p1x, p1y, p2x, p2y, bx, by, tcx, tcy): return True
    if segmentos_intersectan(p1x, p1y, p2x, p2y, tcx, tcy, ax, ay): return True
    return False

def choque_circulo(centro_x, centro_y, radio, p1x, p1y, p2x, p2y):
    dx = p2x - p1x
    dy = p2y - p1y
    fx = p1x - centro_x
    fy = p1y - centro_y
    a = dx*dx + dy*dy
    if a == 0:
        return (fx*fx + fy*fy) <= radio*radio
    b = 2.0 * (fx*dx + fy*dy)
    c = fx*fx + fy*fy - radio*radio
    disc = b*b - 4.0*a*c
    if disc < 0:
        return False
    disc = math.sqrt(disc)
    t1 = (-b - disc) / (2.0*a)
    t2 = (-b + disc) / (2.0*a)
    if (0.0 <= t1 <= 1.0) or (0.0 <= t2 <= 1.0):
        return True
    if (fx*fx + fy*fy) <= radio*radio:
        return True
    fx2 = p2x - centro_x
    fy2 = p2y - centro_y
    if (fx2*fx2 + fy2*fy2) <= radio*radio:
        return True
    return False

# ─────────────────────────────────────────────
# CIRCULOS
# ─────────────────────────────────────────────
def iniciar_circulos_estaticos():
    # {cx, cy, r_colision, r_influencia, gravedad}
    g.circulos_estaticos = [
        {"cx": 200, "cy": 250, "rc": 40, "ri": 190, "grav": 0.5},
        {"cx": 500, "cy": 150, "rc": 30, "ri": 150, "grav": 0.4},
        {"cx": 600, "cy": 400, "rc": 25, "ri": 120, "grav": 0.2},
    ]

def aplicar_gravedad_circulos_estaticos():
    for c in g.circulos_estaticos:
        dx = c["cx"] - g.cx
        dy = c["cy"] - g.cy
        dist = math.sqrt(dx*dx + dy*dy)
        if c["ri"] > dist > 5.0:
            dir_x = dx / dist
            dir_y = dy / dist
            factor = 1.0 - dist / c["ri"]
            acel = c["grav"] * factor
            escala = 0.25
            g.vx += dir_x * acel * escala
            g.vy += dir_y * acel * escala

def iniciar_circulos_moviles():
    g.circulos_moviles = []
    for i in range(g.MAX_CIRCULOS_MOVILES):
        g.circulos_moviles.append({
            "x": 0.0, "y": 0.0, "vx": 0.0, "vy": 0.0,
            "rc": 0.0, "ri": 0.0, "grav": 0.0,
            "activo": False, "timer": 10 + random.randint(0, 29)
        })

def actualizar_circulos_moviles():
    for c in g.circulos_moviles:
        if c["activo"]:
            c["x"] += c["vx"]
            c["y"] += c["vy"]
            if c["x"] < -100 or c["x"] > 840 or c["y"] < -100 or c["y"] > 600:
                c["activo"] = False
                c["timer"] = 50 + random.randint(0, 99)
        else:
            c["timer"] -= 1
            if c["timer"] <= 0:
                lado = random.randint(0, 3)
                if lado == 0:
                    x, y = -40, random.randint(0, 499); mvx = 1.5 + random.randint(0,4)*0.3; mvy = (random.randint(0,99)-50)/50.0
                elif lado == 1:
                    x, y = 780, random.randint(0, 499); mvx = -1.5 - random.randint(0,4)*0.3; mvy = (random.randint(0,99)-50)/50.0
                elif lado == 2:
                    x, y = random.randint(0, 739), -40; mvx = (random.randint(0,99)-50)/50.0; mvy = 1.5 + random.randint(0,4)*0.3
                else:
                    x, y = random.randint(0, 739), 540; mvx = (random.randint(0,99)-50)/50.0; mvy = -1.5 - random.randint(0,4)*0.3
                c["x"], c["y"] = x, y
                c["vx"], c["vy"] = mvx, mvy
                c["rc"] = 15 + random.randint(0, 19)
                c["ri"] = c["rc"] * 3
                c["grav"] = 0.04 + random.randint(0, 9) * 0.02
                c["activo"] = True

def aplicar_gravedad_circulos_moviles():
    for c in g.circulos_moviles:
        if not c["activo"]:
            continue
        dx = c["x"] - g.cx
        dy = c["y"] - g.cy
        dist = math.sqrt(dx*dx + dy*dy)
        if c["ri"] > dist > 1.0:
            dir_x = dx / dist
            dir_y = dy / dist
            acel = c["grav"]
            g.vx += dir_x * acel
            g.vy += dir_y * acel

def dibujar_circulos_estaticos(surf):
    for c in g.circulos_estaticos:
        pygame.draw.circle(surf, COL_ROCA, (int(c["cx"]), int(c["cy"])), int(c["rc"]))
        pygame.draw.circle(surf, (100,100,100), (int(c["cx"]), int(c["cy"])), int(c["ri"]), 1)

def dibujar_circulos_moviles(surf):
    for c in g.circulos_moviles:
        if c["activo"]:
            pygame.draw.circle(surf, COL_ROCA, (int(c["x"]), int(c["y"])), int(c["rc"]))
            pygame.draw.circle(surf, (100,100,100), (int(c["x"]), int(c["y"])), int(c["ri"]), 1)

# ─────────────────────────────────────────────
# COLISIONES DE NAVE
# ─────────────────────────────────────────────
def choque_nave(n, ncx, ncy):
    # Las 5 hitboxes de la nave (lineas)
    r1x, r1y, r2x, r2y = ncx-21, ncy+9, ncx-18, ncy+21
    b1x, b1y, b2x, b2y = ncx-21, ncy-1, ncx-9, ncy+11
    p1x, p1y, p2x, p2y = ncx+18, ncy+9, ncx+21, ncy+21
    q1x, q1y, q2x, q2y = ncx+9, ncy-1, ncx+21, ncy+11
    z1x, z1y, z2x, z2y = ncx-11, ncy-16, ncx+11, ncy+1

    def CHK(X1, Y1, X2, Y2, T):
        return (choque_triangulo(X1,Y1,X2,Y2,r1x,r1y,r2x,r2y,T) or
                choque_triangulo(X1,Y1,X2,Y2,b1x,b1y,b2x,b2y,T) or
                choque_triangulo(X1,Y1,X2,Y2,p1x,p1y,p2x,p2y,T) or
                choque_triangulo(X1,Y1,X2,Y2,q1x,q1y,q2x,q2y,T) or
                choque_triangulo(X1,Y1,X2,Y2,z1x,z1y,z2x,z2y,T))

    if n == 2:
        if CHK(110,100,300,500,"abajo"): return True
        if CHK(500,500,600,300,"abajo"): return True
        if CHK(600,300,800,500,"abajo"): return True
        if CHK(200,0,400,350,"arriba"): return True
    if n == 3:
        if CHK(110,300,300,500,"abajo"): return True
        if CHK(305,300,510,500,"abajo"): return True
        if CHK(400,0,600,400,"arriba"): return True
        if CHK(100,0,250,200,"arriba"): return True
    if n == 4:
        if CHK(100,0,200,250,"arriba"): return True
        if CHK(200,250,300,0,"arriba"): return True
        if CHK(400,0,550,300,"arriba"): return True
        if CHK(550,300,700,0,"arriba"): return True
        if CHK(250,500,350,200,"abajo"): return True
        if CHK(350,200,450,500,"abajo"): return True
        if CHK(550,500,650,250,"abajo"): return True
        if CHK(650,250,740,500,"abajo"): return True
    if n == 5:
        if CHK(100,0,200,250,"arriba"): return True
        if CHK(200,250,300,0,"arriba"): return True
        if CHK(400,0,550,300,"arriba"): return True
        if CHK(550,300,700,0,"arriba"): return True
        if CHK(250,500,350,100,"abajo"): return True
        if CHK(350,100,450,500,"abajo"): return True
        if CHK(550,500,650,250,"abajo"): return True
        if CHK(650,250,740,500,"abajo"): return True
    if n == 6:
        if CHK(100,0,200,250,"arriba"): return True
        if CHK(200,250,300,0,"arriba"): return True
        if CHK(400,0,550,300,"arriba"): return True
        if CHK(550,300,700,0,"arriba"): return True
        if CHK(200,500,350,65,"abajo"): return True
        if CHK(350,65,500,500,"abajo"): return True
        if CHK(550,500,650,250,"abajo"): return True
        if CHK(650,250,740,500,"abajo"): return True
    if n == 7:
        if CHK(0,0,200,150,"arriba"): return True
        if CHK(200,150,400,0,"arriba"): return True
        if CHK(400,0,600,120,"arriba"): return True
        if CHK(600,120,800,0,"arriba"): return True
        if CHK(100,500,200,400,"abajo"): return True
        if CHK(200,400,400,500,"abajo"): return True
        if CHK(400,500,600,290,"abajo"): return True
        if CHK(600,290,800,500,"abajo"): return True
        for i in range(g.MAX_ROCAS):
            if g.roca_activa[i]:
                rx, ry = g.roca_x[i], g.roca_y[i]
                if CHK(rx-12,ry-6,rx,ry+12,"abajo"): return True
                if CHK(rx,ry+12,rx+12,ry-6,"abajo"): return True
    if n == 8:
        if CHK(110,0,640,225,"arriba"): return True
        if CHK(110,225,640,0,"abajo"): return True
        if CHK(110,285,640,500,"arriba"): return True
        if CHK(110,500,640,285,"abajo"): return True
    if n == 9:
        for c in g.circulos_estaticos:
            if (choque_circulo(c["cx"],c["cy"],c["rc"],r1x,r1y,r2x,r2y) or
                choque_circulo(c["cx"],c["cy"],c["rc"],b1x,b1y,b2x,b2y) or
                choque_circulo(c["cx"],c["cy"],c["rc"],p1x,p1y,p2x,p2y) or
                choque_circulo(c["cx"],c["cy"],c["rc"],q1x,q1y,q2x,q2y) or
                choque_circulo(c["cx"],c["cy"],c["rc"],z1x,z1y,z2x,z2y)):
                return True
    if n == 10:
        for c in g.circulos_moviles:
            if c["activo"]:
                if (choque_circulo(c["x"],c["y"],c["rc"],r1x,r1y,r2x,r2y) or
                    choque_circulo(c["x"],c["y"],c["rc"],b1x,b1y,b2x,b2y) or
                    choque_circulo(c["x"],c["y"],c["rc"],p1x,p1y,p2x,p2y) or
                    choque_circulo(c["x"],c["y"],c["rc"],q1x,q1y,q2x,q2y) or
                    choque_circulo(c["x"],c["y"],c["rc"],z1x,z1y,z2x,z2y)):
                    return True
    return False

# ─────────────────────────────────────────────
# FUNCIONES DE JUEGO
# ─────────────────────────────────────────────
def reiniciar_nivel(n):
    g.combustible = 100.0
    g.vx = 0.0
    g.vy = 0.0
    if n == 4:
        g.cx, g.cy, g.vy = 710, 80, 0
    else:
        g.cx, g.cy, g.vy = 710, 100, -0.05

def mover_nave():
    ax, ay = 0.0, 0.05
    g.vx += ax
    g.vy += ay
    lim = 4.0
    g.vx = max(-lim, min(lim, g.vx))
    g.vy = max(-lim, min(lim, g.vy))
    g.cx += g.vx
    g.cy += g.vy

def aceleracion(da):
    ax, ay = rotar(0, -0.09, 0, 0, da)
    g.vx += ax
    g.vy += ay

def aterrizar(n):
    return (g.cy+20 >= 450 and g.cx-20 >= 10 and g.cx+20 <= 100 and g.vy < 1)

def check_game_over():
    if g.cx+20 >= W or g.cx-20 <= 0 or g.cy-15 <= 0 or g.cy+20 >= H:
        return True
    if choque_nave(g.num_nivel, g.cx, g.cy):
        return True
    if g.cy+20 >= 450 and g.cx-20 >= 10 and g.cx+20 <= 100 and g.vy >= 1.0:
        return True
    if g.cy+20 >= 450 and (g.cx+20 > 10 and g.cx-20 < 100) and (g.cx-20 < 10 or g.cx+20 > 100):
        return True
    return False

# ─────────────────────────────────────────────
# DIBUJO
# ─────────────────────────────────────────────
def pintar_nave(surf, nx, ny):
    nav = [nx-20,ny+20, nx-20,ny+10, nx-10,ny, nx-10,ny-10, nx,ny-15,
           nx+10,ny-10, nx+10,ny, nx+20,ny+10, nx+20,ny+20, nx-10,ny, nx+10,ny]
    for i in range(0, 15, 2):
        pygame.draw.line(surf, COL_ROCA, (nav[i], nav[i+1]), (nav[i+2], nav[i+3]), 1)
        if i == 14:
            pygame.draw.line(surf, COL_ROCA, (nav[i+4], nav[i+5]), (nav[i+6], nav[i+7]), 1)

def pintar_motor(surf, da, mcx, mcy):
    c1, c2 = mcx, mcy
    if da != 0:
        c2 += 14
    fuego = [c1-5,c2+5, c1-10,c2+20, c1-5,c2+20, c1,c2+35, c1+5,c2+20, c1+10,c2+20, c1+5,c2+5]
    pts = []
    for i in range(0, 14, 2):
        rx, ry = rotar(fuego[i], fuego[i+1], mcx, mcy, da)
        pts.append((rx, ry))
    for i in range(len(pts)-1):
        pygame.draw.line(surf, COL_AMARILLO, pts[i], pts[i+1], 1)

def medidor_combustible(surf, font, c):
    g.parpadeo_comb += 1
    mostrar = True
    col_texto = COL_ROCA
    col_barra = COL_ROCA
    if c < 18:
        col_texto = (255, 50, 50)
        col_barra = (255, 50, 50)
        if (g.parpadeo_comb % 30) < 15:
            mostrar = False
    if mostrar:
        txt = font.render("Combustible", True, col_texto)
        surf.blit(txt, (100 - txt.get_width()//2, 24))
    pygame.draw.rect(surf, COL_NEGRO, (50, 50, 100, 5))
    pygame.draw.rect(surf, COL_ROCA, (50, 50, 100, 5), 1)
    if c > 0:
        pygame.draw.rect(surf, col_barra, (50, 50, int(c), 5))

def dibujar_triangulos(surf):
    if not g.rocas_init:
        for i in range(g.MAX_ROCAS):
            g.roca_activa[i] = False
            g.roca_timer[i] = 30 + i*20
        g.rocas_init = True
    for i in range(g.MAX_ROCAS):
        if g.roca_activa[i]:
            g.roca_y[i] += g.roca_vy[i]
            g.roca_vy[i] += 0.08
            if g.roca_y[i] > 500:
                g.roca_activa[i] = False
                g.roca_timer[i] = 60 + random.randint(0, 119)
        else:
            g.roca_timer[i] -= 1
            if g.roca_timer[i] <= 0:
                g.roca_x[i] = 100 + random.randint(0, 499)
                g.roca_y[i] = 30
                g.roca_vy[i] = 1.5 + random.randint(0, 2)*0.5
                g.roca_activa[i] = True
    for i in range(g.MAX_ROCAS):
        if g.roca_activa[i]:
            rx, ry = g.roca_x[i], g.roca_y[i]
            pygame.draw.polygon(surf, COL_ROCA, [(rx-12,ry-6),(rx+12,ry-6),(rx,ry+12)])

def pintar_nivel(surf, n):
    c = COL_ROCA
    if n == 1:
        pygame.draw.rect(surf, c, (10, 450, 90, 50))
    if n == 2:
        pygame.draw.polygon(surf, c, [(110,100),(300,500),(110,500)])
        pygame.draw.polygon(surf, c, [(500,500),(600,300),(600,500)])
        pygame.draw.polygon(surf, c, [(600,300),(800,500),(600,500)])
        pygame.draw.polygon(surf, c, [(200,0),(400,350),(400,0)])
        pygame.draw.rect(surf, c, (10, 450, 90, 50))
    if n == 3:
        pygame.draw.polygon(surf, c, [(110,300),(300,500),(110,500)])
        pygame.draw.polygon(surf, c, [(305,300),(510,500),(310,500)])
        pygame.draw.polygon(surf, c, [(400,0),(600,400),(600,0)])
        pygame.draw.polygon(surf, c, [(100,0),(250,200),(250,0)])
        pygame.draw.rect(surf, c, (10, 450, 90, 50))
    if n == 4:
        pygame.draw.polygon(surf, c, [(100,0),(200,250),(300,0)])
        pygame.draw.polygon(surf, c, [(400,0),(550,300),(700,0)])
        pygame.draw.polygon(surf, c, [(250,500),(350,200),(450,500)])
        pygame.draw.polygon(surf, c, [(550,500),(650,250),(740,500)])
        pygame.draw.rect(surf, c, (10, 450, 90, 50))
    if n == 5:
        pygame.draw.polygon(surf, c, [(100,0),(200,250),(300,0)])
        pygame.draw.polygon(surf, c, [(400,0),(550,300),(700,0)])
        pygame.draw.polygon(surf, c, [(250,500),(350,100),(450,500)])
        pygame.draw.polygon(surf, c, [(550,500),(650,250),(740,500)])
        pygame.draw.rect(surf, c, (10, 450, 90, 50))
    if n == 6:
        pygame.draw.polygon(surf, c, [(100,0),(200,250),(300,0)])
        pygame.draw.polygon(surf, c, [(400,0),(550,300),(700,0)])
        pygame.draw.polygon(surf, c, [(200,500),(350,65),(500,500)])
        pygame.draw.polygon(surf, c, [(550,500),(650,250),(740,500)])
        pygame.draw.rect(surf, c, (10, 450, 90, 50))
    if n == 7:
        dibujar_triangulos(surf)
        pygame.draw.polygon(surf, c, [(0,0),(200,150),(400,0)])
        pygame.draw.polygon(surf, c, [(400,0),(600,120),(800,0)])
        pygame.draw.polygon(surf, c, [(100,500),(200,400),(400,500)])
        pygame.draw.polygon(surf, c, [(400,500),(600,290),(800,500)])
        pygame.draw.rect(surf, c, (10, 450, 90, 50))
    if n == 8:
        pygame.draw.rect(surf, c, (110, 0, 530, 225))
        pygame.draw.rect(surf, c, (110, 285, 530, 215))
        pygame.draw.rect(surf, c, (10, 450, 90, 50))
    if n == 9:
        iniciar_circulos_estaticos()
        dibujar_circulos_estaticos(surf)
        pygame.draw.rect(surf, c, (10, 450, 90, 50))
    if n == 10:
        if not g.circulos_inicializados:
            iniciar_circulos_moviles()
            g.circulos_inicializados = True
        actualizar_circulos_moviles()
        dibujar_circulos_moviles(surf)
        pygame.draw.rect(surf, c, (10, 450, 90, 50))

def inicializar_estrellas():
    g.estrellas = []
    for i in range(100):
        x = random.randint(0, W-1)
        y = random.randint(0, H-1)
        gr = 100 + random.randint(0, 154)
        g.estrellas.append([x, y, (gr, gr, gr)])

def dibujar_estrellas(surf):
    for s in g.estrellas:
        if random.randint(0, 99) < 5:
            gr = 100 + random.randint(0, 154)
            s[2] = (gr, gr, gr)
        surf.set_at((s[0], s[1]), s[2])

def iniciar_explosion(ecx, ecy):
    g.expl_x = [ecx-10, ecx+10, ecx, ecx, ecx+15, ecx-15, ecx+5, ecx-10, ecx+10, ecx-5, ecx-10, ecx+10]
    g.expl_y = [ecy, ecy, ecy-15, ecy+15, ecy-15, ecy+15, ecy+5, ecy-10, ecy-10, ecy+10, ecy, ecy]

# ─────────────────────────────────────────────
# AUDIO
# ─────────────────────────────────────────────
class Audio:
    def __init__(self):
        self.ok = False
        try:
            pygame.mixer.init()
            self.ok = True
        except Exception:
            self.ok = False
        self.splash = self.cargar("pdpsong.wav")
        self.start = self.cargar("sonidostart.wav")
        self.motor = self.cargar("thrust.wav")
        self.boom = self.cargar("boom.wav")
        self.musica = self.cargar("gamesong.wav")
        self.victoria = self.cargar("victory_fixed.wav")
        self.canal_motor = None

    def cargar(self, ruta):
        if not self.ok:
            return None
        try:
            return pygame.mixer.Sound(ruta)
        except Exception:
            return None

    def play(self, snd, loops=0, vol=1.0):
        if snd:
            ch = snd.play(loops=-1 if loops else 0)
            if ch:
                ch.set_volume(vol)
            return ch
        return None

    def stop(self, snd):
        if snd:
            snd.stop()

audio = None

# ─────────────────────────────────────────────
# GAME TICK (logica de estados)
# ─────────────────────────────────────────────
def game_tick(keys, mouse_pressed):
    if g.estado == CLICK_TO_START:
        if any(keys) or mouse_pressed:
            audio.play(audio.splash, loops=1, vol=1.0)
            g.splash_parpadeo = 0
            g.estado = SPLASH_ESPERA

    elif g.estado == SPLASH_ESPERA:
        g.splash_parpadeo += 1
        if g.splash_parpadeo > 120 and any(keys):
            audio.stop(audio.splash)
            g.estado = LIMPIEZA

    elif g.estado == LIMPIEZA:
        if not keys[pygame.K_a] and not any(keys):
            audio.play(audio.start, loops=0, vol=1.0)
            audio.play(audio.musica, loops=1, vol=0.6)
            inicializar_estrellas()
            g.nivel_y = -30
            g.nivel_timer = 180
            g.estado = JUGANDO

    elif g.estado == JUGANDO:
        motores = 0
        mover_nave()
        if keys[pygame.K_UP] and g.combustible >= 0:
            aceleracion(0); motores += 1
        if keys[pygame.K_RIGHT] and g.combustible >= 0:
            aceleracion(-90); motores += 1
        if keys[pygame.K_LEFT] and g.combustible >= 0:
            aceleracion(90); motores += 1
        if motores > 0:
            g.combustible -= 0.2 * motores

        if g.num_nivel == 9:
            aplicar_gravedad_circulos_estaticos()
        if g.num_nivel == 10:
            actualizar_circulos_moviles()
            aplicar_gravedad_circulos_moviles()

        # Sonido motor
        thrust = keys[pygame.K_UP] or keys[pygame.K_LEFT] or keys[pygame.K_RIGHT]
        if thrust and g.combustible > 0:
            if not g.motor_encendido:
                g.canal_motor = audio.play(audio.motor, loops=1, vol=0.6)
                g.motor_encendido = True
        else:
            if g.motor_encendido:
                audio.stop(audio.motor)
                g.motor_encendido = False

        if g.nivel_timer > 0:
            g.nivel_timer -= 1

        if keys[pygame.K_ESCAPE]:
            g.running = False
            return

        if check_game_over():
            audio.stop(audio.musica)
            if g.motor_encendido:
                audio.stop(audio.motor)
                g.motor_encendido = False
            audio.play(audio.boom, loops=0, vol=1.0)
            iniciar_explosion(g.cx, g.cy)
            reiniciar_nivel(g.num_nivel)
            g.estado = EXPLOSION
            return

        if aterrizar(g.num_nivel):
            if g.motor_encendido:
                audio.stop(audio.motor)
                g.motor_encendido = False
            if g.num_nivel == 10:
                g.nave_victoria_y = 500
                g.parpadeo_victoria = 0
                audio.stop(audio.musica)
                audio.play(audio.victoria, loops=0, vol=1.0)
                g.estado = VICTORIA
            else:
                g.parpadeo_nivel = 0
                g.estado = NIVEL_COMPLETADO

    elif g.estado == NIVEL_COMPLETADO:
        g.parpadeo_nivel += 1
        if keys[pygame.K_a]:
            g.num_nivel += 1
            if g.num_nivel == 9:
                iniciar_circulos_estaticos()
            if g.num_nivel == 10:
                iniciar_circulos_moviles()
                g.circulos_inicializados = True
            reiniciar_nivel(g.num_nivel)
            g.nivel_y = -30
            g.nivel_timer = 180
            g.estado = JUGANDO

    elif g.estado == EXPLOSION:
        j = 0
        for i in range(0, 10, 2):
            rx, ry = rotar(g.expl_x[i+1], g.expl_y[i+1], g.expl_x[i], g.expl_y[i], 1)
            g.expl_x[i+1], g.expl_y[i+1] = rx, ry
            g.expl_x[i] += g.expl_dx[j]; g.expl_y[i] += g.expl_dy[j]
            g.expl_x[i+1] += g.expl_dx[j]; g.expl_y[i+1] += g.expl_dy[j]
            j += 1
        if keys[pygame.K_a]:
            g.nivel_y = -30
            g.nivel_timer = 180
            audio.play(audio.musica, loops=1, vol=0.6)
            g.estado = JUGANDO
        if keys[pygame.K_ESCAPE]:
            g.running = False

    elif g.estado == VICTORIA:
        if g.nave_victoria_y > 200:
            g.nave_victoria_y -= 1.5
        g.parpadeo_victoria += 1
        if keys[pygame.K_a]:
            audio.stop(audio.victoria)
            g.num_nivel = 1
            g.circulos_inicializados = False
            reiniciar_nivel(g.num_nivel)
            g.nivel_y = -30
            g.nivel_timer = 180
            audio.play(audio.musica, loops=1, vol=0.6)
            g.estado = JUGANDO
        if keys[pygame.K_ESCAPE]:
            g.running = False

# ─────────────────────────────────────────────
# GAME DRAW
# ─────────────────────────────────────────────
def game_draw(screen, game_surf, font, splash_img, victoria_img):
    game_surf.fill(COL_NEGRO)

    if g.estado == CLICK_TO_START:
        t1 = font.render("MOONLANDING", True, COL_BLANCO)
        t2 = font.render("[ CLICK OR PRESS ANY KEY TO START ]", True, COL_CYAN)
        game_surf.blit(t1, (W//2 - t1.get_width()//2, H//2 - 30))
        game_surf.blit(t2, (W//2 - t2.get_width()//2, H//2 + 10))

    elif g.estado == SPLASH_ESPERA:
        if splash_img:
            game_surf.blit(pygame.transform.scale(splash_img, (W, H)), (0, 0))
        else:
            t = font.render("MOONLANDING", True, COL_BLANCO)
            game_surf.blit(t, (W//2 - t.get_width()//2, H//2 - 20))
        if (g.splash_parpadeo % 80) < 40:
            t = font.render("PRESS A KEY TO START", True, COL_AMARILLO)
            game_surf.blit(t, (W//2 - t.get_width()//2, H - 50))

    elif g.estado == LIMPIEZA:
        t = font.render("Loading...", True, COL_BLANCO)
        game_surf.blit(t, (W//2 - t.get_width()//2, H//2))

    elif g.estado == JUGANDO:
        dibujar_estrellas(game_surf)
        pintar_nivel(game_surf, g.num_nivel)
        keys = pygame.key.get_pressed()
        if keys[pygame.K_UP] and g.combustible >= 0:
            pintar_motor(game_surf, 0, g.cx, g.cy)
        if keys[pygame.K_RIGHT] and g.combustible >= 0:
            pintar_motor(game_surf, -90, g.cx, g.cy)
        if keys[pygame.K_LEFT] and g.combustible >= 0:
            pintar_motor(game_surf, 90, g.cx, g.cy)
        pintar_nave(game_surf, g.cx, g.cy)
        medidor_combustible(game_surf, font, g.combustible)
        # Cartilla de nivel
        if g.nivel_timer > 0:
            if g.nivel_y < 25 and g.nivel_timer > 60:
                g.nivel_y += 2
            elif g.nivel_timer < 40:
                g.nivel_y -= 2
            if g.nivel_y > -30:
                pygame.draw.rect(game_surf, COL_NEGRO, (W//2-110, g.nivel_y-8, 220, 26))
                pygame.draw.rect(game_surf, COL_BLANCO, (W//2-110, g.nivel_y-8, 220, 26), 1)
                t = font.render(f"MISION - NIVEL {g.num_nivel}", True, COL_BLANCO)
                game_surf.blit(t, (W//2 - t.get_width()//2, g.nivel_y - 4))

    elif g.estado == NIVEL_COMPLETADO:
        dibujar_estrellas(game_surf)
        pintar_nivel(game_surf, g.num_nivel)
        pintar_nave(game_surf, g.cx, g.cy)
        if (g.parpadeo_nivel % 80) < 40:
            t = font.render("PRESS (A) FOR THE NEXT LEVEL", True, COL_HUD)
            game_surf.blit(t, (370 - t.get_width()//2, 245))

    elif g.estado == EXPLOSION:
        pintar_nivel(game_surf, g.num_nivel)
        for i in range(0, 10, 2):
            pygame.draw.line(game_surf, COL_BLANCO, (g.expl_x[i], g.expl_y[i]), (g.expl_x[i+1], g.expl_y[i+1]), 1)
        t1 = font.render("PRESS (A) FOR TRY AGAIN", True, COL_HUD)
        t2 = font.render("PRESS (ESC) TO EXIT", True, COL_HUD)
        game_surf.blit(t1, (370 - t1.get_width()//2, 235))
        game_surf.blit(t2, (370 - t2.get_width()//2, 250))

    elif g.estado == VICTORIA:
        if victoria_img:
            game_surf.blit(pygame.transform.scale(victoria_img, (W, H)), (0, 0))
        else:
            dibujar_estrellas(game_surf)
        pintar_nave(game_surf, 370, g.nave_victoria_y)
        if g.nave_victoria_y > 200:
            pintar_motor(game_surf, 0, 370, g.nave_victoria_y)
        pygame.draw.rect(game_surf, COL_CYAN, (150, 120, 440, 240), 1)
        pygame.draw.rect(game_surf, COL_CYAN, (152, 122, 436, 236), 1)
        for txt, col, y in [("** MISION CUMPLIDA **", COL_CYAN, 140),
                             ("HAS COMPLETADO TODOS LOS NIVELES", COL_BLANCO, 195),
                             ("ALUNIZAJE PERFECTO, COMANDANTE", COL_VERDE, 215)]:
            t = font.render(txt, True, col)
            game_surf.blit(t, (370 - t.get_width()//2, y))
        if (g.parpadeo_victoria % 80) < 50:
            t1 = font.render("PRESS (A) TO PLAY AGAIN", True, COL_VERDE)
            t2 = font.render("PRESS (ESC) TO EXIT", True, COL_BLANCO)
            game_surf.blit(t1, (370 - t1.get_width()//2, 310))
            game_surf.blit(t2, (370 - t2.get_width()//2, 330))

    # Escalar el surface del juego a la ventana manteniendo aspect ratio
    dw, dh = screen.get_size()
    escala = min(dw / W, dh / H)
    nw, nh = int(W * escala), int(H * escala)
    dx, dy = (dw - nw) // 2, (dh - nh) // 2
    screen.fill((8, 8, 12))
    scaled = pygame.transform.scale(game_surf, (nw, nh))
    screen.blit(scaled, (dx, dy))
    # Marco
    pygame.draw.rect(screen, (60,60,70), (dx-3, dy-3, nw+6, nh+6), 2)
    pygame.display.flip()

# ─────────────────────────────────────────────
# MAIN
# ─────────────────────────────────────────────
def main():
    global audio
    pygame.init()
    screen = pygame.display.set_mode((1280, 720), pygame.RESIZABLE)
    pygame.display.set_caption("MoonLanding")
    clock = pygame.time.Clock()
    font = pygame.font.SysFont("monospace", 14, bold=True)
    game_surf = pygame.Surface((W, H))

    audio = Audio()

    # Cargar imagenes (opcional)
    def cargar_img(ruta):
        try:
            return pygame.image.load(ruta)
        except Exception:
            return None
    splash_img = cargar_img("splash_nix.bmp")
    victoria_img = cargar_img("victoria_art.bmp")

    inicializar_estrellas()

    while g.running:
        mouse_pressed = False
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                g.running = False
            if event.type == pygame.MOUSEBUTTONDOWN:
                mouse_pressed = True
            if event.type == pygame.VIDEORESIZE:
                screen = pygame.display.set_mode((event.w, event.h), pygame.RESIZABLE)

        keys = pygame.key.get_pressed()
        game_tick(keys, mouse_pressed)
        if not g.running:
            break
        game_draw(screen, game_surf, font, splash_img, victoria_img)
        clock.tick(FPS)

    pygame.quit()

if __name__ == "__main__":
    main()
