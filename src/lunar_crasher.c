#include <pebble.h>
#include <math.h>

#define GRAVITY 8
#define THRUST 25
#define ROTATION_SPEED (15 * SCALE_FACTOR)
#define GAME_TICK_MS 50
#define GROUND_HEIGHT 10
#define LANDER_SIZE 8
#define PAD_WIDTH 20
#define MAP_COUNT 5
#define CURRENT_MAP 0
#define TRIG_MAX_ANGLE 65536
#define TRIG_MAX_RATIO 65536
#define SCALE_FACTOR 100
#define MAX_ROTATION_ANGLE (45 * SCALE_FACTOR)
#define MIN_ROTATION_ANGLE (-45 * SCALE_FACTOR) 

#define DEG_TO_RAD(deg) ((deg) * M_PI / 180.0f)

typedef struct {
    int32_t x;          
    int32_t y;          
    int32_t dx;         
    int32_t dy;         
    int32_t rotation;   
    int thrusting;      
    int game_over;      
    int landed;         
    int fuel;           
    int score;          
    int current_map;    
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
  // Start higher up and with less initial movement
  s_game_state.x = 72 * SCALE_FACTOR;       // Initial x position
  s_game_state.y = 20 * SCALE_FACTOR;       // Start much higher (changed from 84)
  s_game_state.dx = 0;                      // Start with no horizontal movement
  s_game_state.dy = 0;                      // Start with no vertical movement
  s_game_state.rotation = 0;                // Start pointing straight up
  s_game_state.thrusting = 0;               // Start with thrusters off
  s_game_state.game_over = 0;
  s_game_state.landed = 0;
  s_game_state.fuel = 100;
  s_game_state.score = 0;

  srand(time(NULL));
  s_game_state.current_map = rand() % MAP_COUNT;
}

static void draw_lander(GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorWhite);
  
  GPoint lander_center = GPoint(
    (int)(s_game_state.x / SCALE_FACTOR), 
    (int)(s_game_state.y / SCALE_FACTOR)
  );
  
  int32_t angle = (s_game_state.rotation * TRIG_MAX_ANGLE) / (360 * SCALE_FACTOR);
  
  const GPoint lander_points[] = {
    GPoint(0, -LANDER_SIZE),
    GPoint(-LANDER_SIZE, LANDER_SIZE),
    GPoint(LANDER_SIZE, LANDER_SIZE)
  };
  
  GPoint rotated_points[3];
  for (int i = 0; i < 3; i++) {
    int32_t x = lander_points[i].x;
    int32_t y = lander_points[i].y;
    
    rotated_points[i].x = lander_center.x + 
      ((x * cos_lookup(angle) - y * sin_lookup(angle)) / TRIG_MAX_RATIO);
    rotated_points[i].y = lander_center.y + 
      ((x * sin_lookup(angle) + y * cos_lookup(angle)) / TRIG_MAX_RATIO);
  }
  
  graphics_draw_line(ctx, rotated_points[0], rotated_points[1]);
  graphics_draw_line(ctx, rotated_points[1], rotated_points[2]);
  graphics_draw_line(ctx, rotated_points[2], rotated_points[0]);

  if (s_game_state.thrusting && s_game_state.fuel > 0) {
    GPoint thrust_base = {
      (rotated_points[1].x + rotated_points[2].x) / 2,
      (rotated_points[1].y + rotated_points[2].y) / 2
    };
    
    int32_t flame_length = LANDER_SIZE / 2;
    GPoint flame_tip = {
      thrust_base.x + ((flame_length * sin_lookup(angle)) / TRIG_MAX_RATIO),
      thrust_base.y - ((flame_length * cos_lookup(angle)) / TRIG_MAX_RATIO)
    };
    
    int32_t flame_width = LANDER_SIZE / 3;
    GPoint flame_left = {
      thrust_base.x - ((flame_width * cos_lookup(angle)) / TRIG_MAX_RATIO),
      thrust_base.y - ((flame_width * sin_lookup(angle)) / TRIG_MAX_RATIO)
    };
    
    GPoint flame_right = {
      thrust_base.x + ((flame_width * cos_lookup(angle)) / TRIG_MAX_RATIO),
      thrust_base.y + ((flame_width * sin_lookup(angle)) / TRIG_MAX_RATIO)
    };

    graphics_context_set_stroke_color(ctx, GColorRed);
    graphics_draw_line(ctx, thrust_base, flame_left);
    graphics_draw_line(ctx, thrust_base, flame_right);
    graphics_draw_line(ctx, flame_left, flame_tip);
    graphics_draw_line(ctx, flame_right, flame_tip);
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
  int32_t screen_x = s_game_state.x / SCALE_FACTOR;
  int32_t screen_y = s_game_state.y / SCALE_FACTOR;

  if (screen_y < 0) return false;   

  for (int i = 0; i < current_map.terrain_size - 1; i++) {
    if (screen_x >= current_map.terrain[i].x && screen_x <= current_map.terrain[i + 1].x) {
      int32_t dx = current_map.terrain[i + 1].x - current_map.terrain[i].x;
      int32_t dy = current_map.terrain[i + 1].y - current_map.terrain[i].y;
      
      if (dx == 0) continue;  

      int32_t slope = (dy << 16) / dx;
      int32_t expected_y = current_map.terrain[i].y + 
                          (((screen_x - current_map.terrain[i].x) * slope) >> 16);

      if (screen_y + LANDER_SIZE > expected_y) {
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
  if (s_game_state.game_over) {
    return;
  }

  GRect bounds = layer_get_bounds(s_game_layer);

  if (s_game_state.thrusting && s_game_state.fuel > 0) {
    int32_t angle = (s_game_state.rotation * TRIG_MAX_ANGLE) / (360 * SCALE_FACTOR);

    s_game_state.dx += (sin_lookup(angle) * THRUST) / TRIG_MAX_RATIO;
    s_game_state.dy -= (cos_lookup(angle) * THRUST) / TRIG_MAX_RATIO;

    s_game_state.fuel--;
  }

  s_game_state.dy += GRAVITY;

  s_game_state.x += s_game_state.dx;
  s_game_state.y += s_game_state.dy;

  if (s_game_state.x < 0) {
    s_game_state.x = bounds.size.w * SCALE_FACTOR;
  } else if (s_game_state.x > bounds.size.w * SCALE_FACTOR) {
    s_game_state.x = 0;
  }

  if (check_collision()) {
    s_game_state.game_over = true;

    GameMap current_map = s_maps[s_game_state.current_map];
    bool soft_landing = (abs(s_game_state.dy / SCALE_FACTOR) < 2 && abs(s_game_state.dx / SCALE_FACTOR) < 1);
    bool upright = (abs(s_game_state.rotation) < 15 * SCALE_FACTOR || abs(s_game_state.rotation) > 345 * SCALE_FACTOR);

    if (s_game_state.landed && soft_landing && upright) {
      int difficulty_bonus = (current_map.pad_end_index - current_map.pad_start_index) * 5;
      s_game_state.score = s_game_state.fuel * 10 + difficulty_bonus;
      vibes_short_pulse();
    } else {
      s_game_state.landed = false;
      vibes_short_pulse();
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
    if (!s_game_state.game_over) {
        if (s_game_state.rotation < MAX_ROTATION_ANGLE) {
            s_game_state.rotation += ROTATION_SPEED;
        }
    }
}

static void select_release_handler(ClickRecognizerRef recognizer, void *context) {
  s_game_state.thrusting = false;
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_game_state.game_over) {
    init_game();
  } else { 
    s_game_state.thrusting = true;
  }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (!s_game_state.game_over) {
        if (s_game_state.rotation > MIN_ROTATION_ANGLE) {
            s_game_state.rotation -= ROTATION_SPEED;
        }
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

    s_game_layer = layer_create(bounds);
    layer_set_update_proc(s_game_layer, game_layer_update_proc);
    layer_add_child(window_layer, s_game_layer);

    init_maps();
    srand(time(NULL));

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