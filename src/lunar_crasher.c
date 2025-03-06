#include <pebble.h>
#include <math.h>

#define GRAVITY 0.05f
#define THRUST 0.15f
#define ROTATION_SPEED 20.0f
#define GAME_TICK_MS 50
#define GROUND_HEIGHT 10
#define LANDER_SIZE 8
#define PAD_WIDTH 20
#define MAP_COUNT 5
#define CURRENT_MAP 0
#define TRIG_MAX_ANGLE 65536
#define TRIG_MAX_RATIO 65536
#define SCALE_FACTOR 100

#define DEG_TO_RAD(deg) ((deg) * M_PI / 180.0f)

typedef struct {
    int32_t x;          // x position (scaled by SCALE_FACTOR)
    int32_t y;          // y position (scaled by SCALE_FACTOR)
    int32_t dx;         // x velocity (scaled by SCALE_FACTOR)
    int32_t dy;         // y velocity (scaled by SCALE_FACTOR)
    int32_t rotation;   // rotation (scaled by SCALE_FACTOR)
    int thrusting;      // boolean or integer flag
    int game_over;      // boolean or integer flag
    int landed;         // boolean or integer flag
    int fuel;           // integer value
    int score;          // integer value
    int current_map;    // integer value
} GameState;

static Window *s_main_window;
static Layer *s_game_layer;
static AppTimer *s_game_timer;
static GameState s_game_state;

typedef struct {
  GPoint terrain[10];
  int terrain_size;
  int pad_start_index;
  int pad_end_index;
} GameMap;

static GameMap s_maps[MAP_COUNT];

static void init_maps() {
  s_maps[0].terrain[0] = GPoint(0, 140);
  s_maps[0].terrain[1] = GPoint(30, 100);
  s_maps[0].terrain[2] = GPoint(60, 120);
  s_maps[0].terrain[3] = GPoint(80, 120);
  s_maps[0].terrain[4] = GPoint(100, 120);
  s_maps[0].terrain[5] = GPoint(130, 110);
  s_maps[0].terrain[6] = GPoint(160, 80);
  s_maps[0].terrain[7] = GPoint(180, 140);
  s_maps[0].terrain_size = 8;
  s_maps[0].pad_start_index = 3;
  s_maps[0].pad_end_index = 4;
  
  s_maps[1].terrain[0] = GPoint(0, 100);
  s_maps[1].terrain[1] = GPoint(40, 140);
  s_maps[1].terrain[2] = GPoint(70, 140);
  s_maps[1].terrain[3] = GPoint(90, 140);
  s_maps[1].terrain[4] = GPoint(120, 140);
  s_maps[1].terrain[5] = GPoint(160, 90);
  s_maps[1].terrain[6] = GPoint(180, 120);
  s_maps[1].terrain_size = 7;
  s_maps[1].pad_start_index = 2;
  s_maps[1].pad_end_index = 3;
  
  s_maps[2].terrain[0] = GPoint(0, 130);
  s_maps[2].terrain[1] = GPoint(30, 90);
  s_maps[2].terrain[2] = GPoint(60, 110);
  s_maps[2].terrain[3] = GPoint(90, 70);
  s_maps[2].terrain[4] = GPoint(110, 70);
  s_maps[2].terrain[5] = GPoint(130, 70);
  s_maps[2].terrain[6] = GPoint(150, 100);
  s_maps[2].terrain[7] = GPoint(180, 140);
  s_maps[2].terrain_size = 8;
  s_maps[2].pad_start_index = 4;
  s_maps[2].pad_end_index = 5;
  
  s_maps[3].terrain[0] = GPoint(0, 80);
  s_maps[3].terrain[1] = GPoint(30, 140);
  s_maps[3].terrain[2] = GPoint(60, 90);
  s_maps[3].terrain[3] = GPoint(90, 130);
  s_maps[3].terrain[4] = GPoint(110, 130); 
  s_maps[3].terrain[5] = GPoint(130, 130); 
  s_maps[3].terrain[6] = GPoint(150, 70);
  s_maps[3].terrain[7] = GPoint(180, 120);
  s_maps[3].terrain_size = 8;
  s_maps[3].pad_start_index = 4;
  s_maps[3].pad_end_index = 5;
  
  s_maps[4].terrain[0] = GPoint(0, 120);
  s_maps[4].terrain[1] = GPoint(40, 60);
  s_maps[4].terrain[2] = GPoint(70, 100);
  s_maps[4].terrain[3] = GPoint(100, 50);
  s_maps[4].terrain[4] = GPoint(130, 50); 
  s_maps[4].terrain[5] = GPoint(150, 50);
  s_maps[4].terrain[6] = GPoint(170, 90);
  s_maps[4].terrain[7] = GPoint(180, 130);
  s_maps[4].terrain_size = 8;
  s_maps[4].pad_start_index = 4;
  s_maps[4].pad_end_index = 5;
}

void init_game() {
  GameState s_game_state = {
    .x = 72,       // 72.0 becomes 7200
    .y = 84,       // 84.0 becomes 8400
    .dx = 1 * SCALE_FACTOR,       // 1.0 becomes 100
    .dy = -2 * SCALE_FACTOR,      // -2.0 becomes -200
    .rotation = 45 * SCALE_FACTOR,// 45.0 becomes 4500
    .thrusting = 1,
    .game_over = 0,
    .landed = 0,
    .fuel = 100,
    .score = 0,
    .current_map = 1
  };

  APP_LOG(APP_LOG_LEVEL_INFO, "Game state x=%d", s_game_state.x);
  APP_LOG(APP_LOG_LEVEL_INFO, "Game state y=%d", s_game_state.y);
  APP_LOG(APP_LOG_LEVEL_INFO, "Game state dx=%d", s_game_state.dx);
  APP_LOG(APP_LOG_LEVEL_INFO, "Game state dy=%d", s_game_state.dy);
  APP_LOG(APP_LOG_LEVEL_INFO, "Game state rotation=%d", s_game_state.rotation);
  APP_LOG(APP_LOG_LEVEL_INFO, "Game state thrusting=%d", s_game_state.thrusting);
  APP_LOG(APP_LOG_LEVEL_INFO, "Game state game_over=%d", s_game_state.game_over);
  APP_LOG(APP_LOG_LEVEL_INFO, "Game state landed=%d", s_game_state.landed);
  APP_LOG(APP_LOG_LEVEL_INFO, "Game state fuel=%d", s_game_state.fuel);
  APP_LOG(APP_LOG_LEVEL_INFO, "Game state score=%d", s_game_state.score);
  APP_LOG(APP_LOG_LEVEL_INFO, "Game state current_map=%d", s_game_state.current_map);
  
}

static void draw_lander(GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorWhite);
  
  GPoint lander_center = GPoint((int)s_game_state.x, (int)s_game_state.y);
  
  int32_t angle = (int32_t)(s_game_state.rotation * TRIG_MAX_ANGLE / 360);
  
  const GPoint lander_points[] = {
    GPoint(0, -LANDER_SIZE),
    GPoint(-LANDER_SIZE, LANDER_SIZE),
    GPoint(LANDER_SIZE, LANDER_SIZE)
  };
  
  GPoint rotated_points[3];
  for (int i = 0; i < 3; i++) {
    int x = lander_points[i].x;
    int y = lander_points[i].y;
    rotated_points[i].x = lander_center.x + (x * cos_lookup(angle) / TRIG_MAX_RATIO - 
                                             y * sin_lookup(angle) / TRIG_MAX_RATIO);
    rotated_points[i].y = lander_center.y + (x * sin_lookup(angle) / TRIG_MAX_RATIO + 
                                             y * cos_lookup(angle) / TRIG_MAX_RATIO);
  }
  
  graphics_draw_line(ctx, rotated_points[0], rotated_points[1]);
  graphics_draw_line(ctx, rotated_points[1], rotated_points[2]);
  graphics_draw_line(ctx, rotated_points[2], rotated_points[0]);

  if (s_game_state.thrusting && s_game_state.fuel > 0) {
    GPoint thrust_base = rotated_points[1]; 
    GPoint flame_left = {thrust_base.x - 3, thrust_base.y + 5};
    GPoint flame_right = {thrust_base.x + 3, thrust_base.y + 5};

    graphics_context_set_stroke_color(ctx, GColorRed);
    graphics_draw_line(ctx, thrust_base, flame_left);
    graphics_draw_line(ctx, thrust_base, flame_right);
  }
}


static void draw_ground(GContext *ctx, GRect bounds) {
  GameMap current_map = s_maps[s_game_state.current_map];
  
  graphics_context_set_stroke_color(ctx, GColorWhite);
  
  for (int i = 0; i < current_map.terrain_size - 1; i++) {
    if (i >= current_map.pad_start_index && i < current_map.pad_end_index) {
      #if defined(PBL_COLOR)
        graphics_context_set_stroke_color(ctx, GColorYellow);
        graphics_context_set_fill_color(ctx, GColorYellow);
      #else
        graphics_context_set_stroke_width(ctx, 2);
        graphics_context_set_fill_color(ctx, GColorWhite);
      #endif
      
      graphics_draw_line(ctx, current_map.terrain[i], current_map.terrain[i + 1]);
          
      int pad_height = 5;
      GRect pad_rect = GRect(
        current_map.terrain[i].x,
        current_map.terrain[i].y,
        current_map.terrain[i + 1].x - current_map.terrain[i].x,
        pad_height
      );
      graphics_fill_rect(ctx, pad_rect, 0, GCornerNone);

      graphics_context_set_stroke_color(ctx, GColorWhite);
      #if !defined(PBL_COLOR)
        graphics_context_set_stroke_width(ctx, 1);
      #endif
    } else {
      graphics_draw_line(ctx, current_map.terrain[i], current_map.terrain[i + 1]);
    }
  }
}

static bool check_collision() {
  GameMap current_map = s_maps[s_game_state.current_map];
  APP_LOG(APP_LOG_LEVEL_INFO, "check_collision on site");
   
  for (int i = 0; i < current_map.terrain_size - 1; i++) {
    if (s_game_state.x >= current_map.terrain[i].x && s_game_state.x <= current_map.terrain[i + 1].x) {
      int32_t dx = current_map.terrain[i + 1].x - current_map.terrain[i].x;
      int32_t dy = current_map.terrain[i + 1].y - current_map.terrain[i].y;

      int32_t slope = (dy << 16) / dx;  // This uses fixed point arithmetic to maintain precision
      int32_t expected_y = current_map.terrain[i].y + ((s_game_state.x - current_map.terrain[i].x) * slope) >> 16;

      if (s_game_state.y + LANDER_SIZE >= expected_y) {
        s_game_state.landed = (i >= current_map.pad_start_index && i < current_map.pad_end_index);
        return true;
      }
    }
  }
  return false;
}



static void draw_ui(GContext *ctx, GRect bounds) {
  char s_buffer[32];
  
  snprintf(s_buffer, sizeof(s_buffer), "FUEL: %d", s_game_state.fuel);
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, s_buffer, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                    GRect(5, 5, bounds.size.w - 10, 20),
                    GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  snprintf(s_buffer, sizeof(s_buffer), "MAP: %d", s_game_state.current_map + 1);
  graphics_draw_text(ctx, s_buffer, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                    GRect(5, 25, bounds.size.w - 10, 20),
                    GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  if (s_game_state.game_over) {
    const char* message = s_game_state.landed ? "LANDED SAFELY!" : "CRASHED!";
    
    #if defined(PBL_COLOR)
      graphics_context_set_text_color(ctx, s_game_state.landed ? GColorGreen : GColorRed);
    #else
      graphics_context_set_text_color(ctx, GColorWhite);
    #endif
    
    graphics_draw_text(ctx, message, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                      GRect(0, bounds.size.h / 2 - 20, bounds.size.w, 20),GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    
    graphics_context_set_text_color(ctx, GColorWhite);
    
    snprintf(s_buffer, sizeof(s_buffer), "SCORE: %d", s_game_state.score);
    graphics_draw_text(ctx, s_buffer, fonts_get_system_font(FONT_KEY_GOTHIC_18),
                      GRect(0, bounds.size.h / 2, bounds.size.w, 20),
                      GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    
    graphics_draw_text(ctx, "Press Select to Restart", fonts_get_system_font(FONT_KEY_GOTHIC_14),
                      GRect(0, bounds.size.h / 2 + 25, bounds.size.w, 20),
                      GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }
}

static void update_game_state() {
  APP_LOG(APP_LOG_LEVEL_INFO, "update_game_state");
  APP_LOG(APP_LOG_LEVEL_INFO, "game_over=%d", s_game_state.game_over);
  // if (s_game_state.game_over) {
  //   return;
  // }

  GRect bounds = layer_get_bounds(s_game_layer);

  if (s_game_state.thrusting && s_game_state.fuel > 0) {
    APP_LOG(APP_LOG_LEVEL_INFO, "thrusting");
    int32_t angle = (s_game_state.rotation * TRIG_MAX_ANGLE) / (360 * SCALE_FACTOR);

    s_game_state.dx += (sin_lookup(angle) * THRUST) / TRIG_MAX_RATIO;
    s_game_state.dy -= (cos_lookup(angle) * THRUST) / TRIG_MAX_RATIO;

    s_game_state.fuel--;
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "dy += GRAVITY");
  s_game_state.dy += GRAVITY;

  s_game_state.x += s_game_state.dx;
  s_game_state.y += s_game_state.dy;

  if (s_game_state.x < 0) {
    s_game_state.x = bounds.size.w * SCALE_FACTOR;
  } else if (s_game_state.x > bounds.size.w * SCALE_FACTOR) {
    s_game_state.x = 0;
  }

  if (check_collision()) {
    APP_LOG(APP_LOG_LEVEL_INFO, "check_collision 2");
    s_game_state.game_over = true;

    GameMap current_map = s_maps[s_game_state.current_map];
    bool soft_landing = (abs(s_game_state.dy) < 150 && abs(s_game_state.dx) < 50);
    bool upright = (abs(s_game_state.rotation) < 1500 || abs(s_game_state.rotation) > 34500);

    if (s_game_state.landed && soft_landing && upright) {
      int difficulty_bonus = (current_map.pad_end_index - current_map.pad_start_index) * 5;
      s_game_state.score = s_game_state.fuel * 10 + difficulty_bonus;
      vibes_short_pulse();
    } else {
      s_game_state.landed = false;
      //vibes_double_pulse();
    }

    s_game_state.dx = 0;
    s_game_state.dy = 0;
  }

  layer_mark_dirty(s_game_layer);
}


static void game_timer_callback(void *data) {
  update_game_state();
  
  s_game_timer = app_timer_register(GAME_TICK_MS, game_timer_callback, NULL);
}

static void game_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  draw_ground(ctx, bounds);
  draw_lander(ctx);
  draw_ui(ctx, bounds);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (!s_game_state.game_over && s_game_state.rotation < 45.0f) {
        s_game_state.rotation += ROTATION_SPEED;
    }
}

static void select_release_handler(ClickRecognizerRef recognizer, void *context) {
  s_game_state.thrusting = false;
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_game_state.game_over) {
    APP_LOG(APP_LOG_LEVEL_INFO, "select_click_handler");
    init_game();
  } else { 
    s_game_state.thrusting = true;
  }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (!s_game_state.game_over && s_game_state.rotation > -45.0f) {
        s_game_state.rotation -= ROTATION_SPEED;
    }
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  
  window_raw_click_subscribe(BUTTON_ID_SELECT, select_click_handler, select_release_handler, NULL);
}

static void main_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Initializing game...");

    // Create and add game layer
    s_game_layer = layer_create(bounds);
    layer_set_update_proc(s_game_layer, game_layer_update_proc);
    layer_add_child(window_layer, s_game_layer);

    // Initialize other stuff first
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Initializing maps...");
    init_maps();
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Initializing random number generator...");
    srand(time(NULL));

    // Now call init_game AFTER the layer is properly added
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Initializing game state...");
    init_game();

    s_game_timer = app_timer_register(GAME_TICK_MS, game_timer_callback, NULL);
}


static void main_window_unload(Window *window) {
  if (s_game_timer) {
    app_timer_cancel(s_game_timer);
  }
  
  layer_destroy(s_game_layer);
}

static void init(void) {
  s_main_window = window_create();
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  
  window_stack_push(s_main_window, true);
}

static void deinit(void) {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}