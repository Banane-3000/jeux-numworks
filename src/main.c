// Snake + jeu de blocs pour NumWorks (API EADK) - aucune ressource externe
#include "eadk.h"
#include <stdint.h>
#include <stdbool.h>

const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Jeux";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;

#define COL(r,g,b) ((uint16_t)((((r)&0xF8)<<8)|(((g)&0xFC)<<3)|(((b)&0xF8)>>3)))
#define BLACK COL(0,0,0)
#define WHITE COL(255,255,255)
#define GREY  COL(110,110,110)

static uint64_t keys, prev;

static void poll(void) { prev = keys; keys = eadk_keyboard_scan(); }
static bool down(eadk_key_t k) { return eadk_keyboard_key_down(keys, k); }
static bool press(eadk_key_t k) { return eadk_keyboard_key_down(keys, k) && !eadk_keyboard_key_down(prev, k); }

static void rect(int x, int y, int w, int h, uint16_t c) {
  eadk_rect_t r = {(uint16_t)x, (uint16_t)y, (uint16_t)w, (uint16_t)h};
  eadk_display_push_rect_uniform(r, c);
}
static void text(const char *s, int x, int y, uint16_t fg, uint16_t bg) {
  eadk_point_t p = {(uint16_t)x, (uint16_t)y};
  eadk_display_draw_string(s, p, false, fg, bg);
}
static void fmt(char *b, const char *l, int n) {
  int i = 0; while (*l) b[i++] = *l++;
  char t[8]; int k = 0;
  if (n == 0) t[k++] = '0';
  while (n > 0 && k < 7) { t[k++] = '0' + n % 10; n /= 10; }
  while (k > 0) b[i++] = t[--k];
  while (i < 12) b[i++] = ' ';
  b[i] = 0;
}
static void wait_ok(void) {
  while (1) { poll(); if (press(eadk_key_ok) || press(eadk_key_back)) return; eadk_timing_msleep(20); }
}

/* ---------------- SNAKE ---------------- */
#define SW 32
#define SH 22
#define SC 10
#define SY 20

static void scell(int x, int y, uint16_t c) { rect(x * SC, SY + y * SC, SC - 1, SC - 1, c); }

static void snake(void) {
  int8_t qx[SW * SH], qy[SW * SH];
  uint8_t occ[SH][SW];
  for (int y = 0; y < SH; y++) for (int x = 0; x < SW; x++) occ[y][x] = 0;
  int len = 3, dx = 1, dy = 0, ndx = 1, ndy = 0, score = 0, speed = 9, cnt = 0, fx, fy;
  char buf[16];
  rect(0, 0, 320, 240, BLACK);
  rect(0, SY - 2, 320, 2, GREY);
  for (int i = 0; i < len; i++) { qx[i] = 10 - i; qy[i] = 11; occ[11][10 - i] = 1; scell(10 - i, 11, COL(60, 220, 60)); }
  do { fx = eadk_random() % SW; fy = eadk_random() % SH; } while (occ[fy][fx]);
  scell(fx, fy, COL(230, 40, 40));
  fmt(buf, "Score ", score); text(buf, 4, 2, WHITE, BLACK);
  poll();
  while (1) {
    poll();
    if (press(eadk_key_back)) return;
    if (down(eadk_key_left)  && dx == 0) { ndx = -1; ndy = 0; }
    if (down(eadk_key_right) && dx == 0) { ndx = 1;  ndy = 0; }
    if (down(eadk_key_up)    && dy == 0) { ndx = 0;  ndy = -1; }
    if (down(eadk_key_down)  && dy == 0) { ndx = 0;  ndy = 1; }
    eadk_timing_msleep(16);
    if (++cnt < speed) continue;
    cnt = 0; dx = ndx; dy = ndy;
    int nx = qx[0] + dx, ny = qy[0] + dy;
    bool eat = (nx == fx && ny == fy);
    if (!eat) { int tx = qx[len - 1], ty = qy[len - 1]; occ[ty][tx] = 0; scell(tx, ty, BLACK); len--; }
    if (nx < 0 || nx >= SW || ny < 0 || ny >= SH || occ[ny][nx] || len >= SW * SH - 1) break;
    for (int i = len; i > 0; i--) { qx[i] = qx[i - 1]; qy[i] = qy[i - 1]; }
    qx[0] = nx; qy[0] = ny; len++; occ[ny][nx] = 1; scell(nx, ny, COL(60, 220, 60));
    if (eat) {
      score += 10; if (speed > 3 && score % 50 == 0) speed--;
      do { fx = eadk_random() % SW; fy = eadk_random() % SH; } while (occ[fy][fx]);
      scell(fx, fy, COL(230, 40, 40));
      fmt(buf, "Score ", score); text(buf, 4, 2, WHITE, BLACK);
    }
  }
  text("PERDU - OK", 120, 100, WHITE, BLACK);
  wait_ok();
}

/* ---------------- BLOCS ---------------- */
#define BX 110
#define BY 20

static const int8_t P[7][4][2] = {
  {{0,1},{1,1},{2,1},{3,1}}, {{1,0},{2,0},{1,1},{2,1}}, {{1,0},{0,1},{1,1},{2,1}},
  {{1,0},{2,0},{0,1},{1,1}}, {{0,0},{1,0},{1,1},{2,1}}, {{0,0},{0,1},{1,1},{2,1}},
  {{2,0},{0,1},{1,1},{2,1}}
};
static const uint16_t PAL[8] = {
  BLACK, COL(0,200,220), COL(240,220,0), COL(170,60,200), COL(60,200,60),
  COL(230,50,50), COL(50,80,230), COL(240,140,20)
};
static const int SCORES[5] = {0, 100, 300, 500, 800};

static void cells(int p, int r, int px, int py, int *cx, int *cy) {
  for (int i = 0; i < 4; i++) {
    int x = P[p][i][0], y = P[p][i][1];
    for (int k = 0; k < r; k++) { int t = x; x = 3 - y; y = t; }
    cx[i] = px + x; cy[i] = py + y;
  }
}
static bool fits(uint8_t b[20][10], int p, int r, int px, int py) {
  int cx[4], cy[4]; cells(p, r, px, py, cx, cy);
  for (int i = 0; i < 4; i++)
    if (cx[i] < 0 || cx[i] >= 10 || cy[i] < 0 || cy[i] >= 20 || b[cy[i]][cx[i]]) return false;
  return true;
}
static void hud(int score, int lines, int level) {
  char buf[16];
  fmt(buf, "Score ", score); text(buf, 8, 40, WHITE, BLACK);
  fmt(buf, "Lignes ", lines); text(buf, 8, 64, WHITE, BLACK);
  fmt(buf, "Niveau ", level + 1); text(buf, 8, 88, WHITE, BLACK);
}
static void show_next(int np) {
  rect(230, 60, 44, 44, BLACK);
  int cx[4], cy[4]; cells(np, 0, 0, 0, cx, cy);
  for (int i = 0; i < 4; i++) rect(231 + cx[i] * 10, 61 + cy[i] * 10, 9, 9, PAL[np + 1]);
}

static void blocs(void) {
  uint8_t b[20][10], shown[20][10], view[20][10];
  for (int y = 0; y < 20; y++) for (int x = 0; x < 10; x++) { b[y][x] = 0; shown[y][x] = 255; }
  int p = eadk_random() % 7, np = eadk_random() % 7, r = 0, px = 3, py = 0;
  int lines = 0, score = 0, level = 0, tick = 0, hl = 0, hr = 0;
  rect(0, 0, 320, 240, BLACK);
  rect(BX - 2, BY - 2, 104, 204, GREY);
  rect(BX, BY, 100, 200, BLACK);
  text("Next", 230, 40, WHITE, BLACK);
  hud(score, lines, level); show_next(np);
  poll();
  while (1) {
    poll();
    if (press(eadk_key_back)) return;
    if (press(eadk_key_left)) { if (fits(b, p, r, px - 1, py)) px--; hl = 0; }
    else if (down(eadk_key_left)) { hl++; if (hl > 8 && hl % 3 == 0 && fits(b, p, r, px - 1, py)) px--; }
    else hl = 0;
    if (press(eadk_key_right)) { if (fits(b, p, r, px + 1, py)) px++; hr = 0; }
    else if (down(eadk_key_right)) { hr++; if (hr > 8 && hr % 3 == 0 && fits(b, p, r, px + 1, py)) px++; }
    else hr = 0;
    if (press(eadk_key_up) && p != 1) {
      int nr = (r + 1) & 3; static const int8_t K[5] = {0, -1, 1, -2, 2};
      for (int k = 0; k < 5; k++) if (fits(b, p, nr, px + K[k], py)) { r = nr; px += K[k]; break; }
    }
    if (press(eadk_key_ok)) { while (fits(b, p, r, px, py + 1)) py++; tick = 999; }
    int interval = 30 - level * 3; if (interval < 3) interval = 3;
    if (down(eadk_key_down) && interval > 2) interval = 2;
    if (++tick >= interval) {
      tick = 0;
      if (fits(b, p, r, px, py + 1)) py++;
      else {
        int cx[4], cy[4]; cells(p, r, px, py, cx, cy);
        for (int i = 0; i < 4; i++) b[cy[i]][cx[i]] = p + 1;
        int cleared = 0;
        for (int y = 19; y >= 0; y--) {
          bool full = true; for (int x = 0; x < 10; x++) if (!b[y][x]) full = false;
          if (full) {
            for (int yy = y; yy > 0; yy--) for (int x = 0; x < 10; x++) b[yy][x] = b[yy - 1][x];
            for (int x = 0; x < 10; x++) b[0][x] = 0;
            cleared++; y++;
          }
        }
        score += SCORES[cleared] * (level + 1); lines += cleared; level = lines / 10;
        p = np; np = eadk_random() % 7; r = 0; px = 3; py = 0;
        hud(score, lines, level); show_next(np);
        if (!fits(b, p, r, px, py)) { text("PERDU - OK", 120, 100, WHITE, BLACK); wait_ok(); return; }
      }
    }
    for (int y = 0; y < 20; y++) for (int x = 0; x < 10; x++) view[y][x] = b[y][x];
    { int cx[4], cy[4]; cells(p, r, px, py, cx, cy);
      for (int i = 0; i < 4; i++) if (cy[i] >= 0 && cy[i] < 20 && cx[i] >= 0 && cx[i] < 10) view[cy[i]][cx[i]] = p + 1; }
    for (int y = 0; y < 20; y++) for (int x = 0; x < 10; x++) {
      if (view[y][x] != shown[y][x]) {
        shown[y][x] = view[y][x];
        rect(BX + x * 10, BY + y * 10, 10, 10, BLACK);
        if (view[y][x]) rect(BX + x * 10, BY + y * 10, 9, 9, PAL[view[y][x]]);
      }
    }
    eadk_timing_msleep(16);
  }
}

/* ---------------- MENU ---------------- */
int main(int argc, char *argv[]) {
  int sel = 0; bool redraw = true;
  keys = 0; prev = 0;
  while (1) {
    if (redraw) {
      rect(0, 0, 320, 240, BLACK);
      text("JEUX", 140, 40, WHITE, BLACK);
      text(sel == 0 ? "> Snake" : "  Snake", 110, 100, WHITE, BLACK);
      text(sel == 1 ? "> Blocs" : "  Blocs", 110, 130, WHITE, BLACK);
      text("OK: jouer   Back: menu", 70, 200, GREY, BLACK);
      redraw = false;
    }
    poll();
    if (press(eadk_key_up) || press(eadk_key_down)) { sel = 1 - sel; redraw = true; }
    if (press(eadk_key_ok)) { if (sel == 0) snake(); else blocs(); redraw = true; }
    eadk_timing_msleep(20);
  }
  return 0;
}
