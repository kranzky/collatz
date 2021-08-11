#include <raylib.h>
#include <flecs.h>

#include "../defines.h"

#include "texture.h"
#include "sound.h"
#include "music.h"
#include "font.h"
#include "shader.h"
#include "data.h"
#include "component.h"
#include "entity.h"
#include "system.h"

#include "game.h"

//==============================================================================

static ecs_world_t *_world = NULL;
static int _primes[100000] = {0};
static int _num = 0;

//==============================================================================

static void
_fini(ecs_world_t *world, void *context)
{
  if (IsAudioDeviceReady())
  {
    CloseAudioDevice();
  }
  CloseWindow();
}

//------------------------------------------------------------------------------

static inline void _init_flecs()
{
  _world = ecs_init();
  ecs_atfini(_world, _fini, NULL);
  // ecs_set_threads(world, 12);
  // TODO: pre-allocate memory with ecs_dim and ecs_dim_type
}

//------------------------------------------------------------------------------

static inline void _init_raylib()
{
#ifdef RELEASE
  SetTraceLogLevel(LOG_ERROR);
#endif
#ifdef DEBUG
  SetTraceLogLevel(LOG_TRACE);
#endif

  int flags = FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT;

#ifdef MAC
  flags = flags | FLAG_WINDOW_HIGHDPI;
#endif
#ifdef DEBUG
  flags = flags | FLAG_WINDOW_RESIZABLE;
#endif

  SetConfigFlags(flags);
  SetTargetFPS(60);

  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Collatz");
  InitAudioDevice();
}

//------------------------------------------------------------------------------

static inline void _show_loading_screen()
{
  for (int i = 0; i < 2; ++i)
  {
    BeginDrawing();
    ClearBackground(BLACK);
    DrawTextEx(GetFontDefault(), "LOADING...", (Vector2){20, 20}, 24, 0, YELLOW);
    EndDrawing();
  }
}

//------------------------------------------------------------------------------

static inline void _init_managers()
{
  texture_manager_init(_world);
  sound_manager_init(_world);
  music_manager_init(_world);
  shader_manager_init(_world);
  font_manager_init(_world);
  data_manager_init(_world);
  component_manager_init(_world);
  entity_manager_init(_world);
  system_manager_init(_world);
}

//==============================================================================

void game_manager_init(void)
{
  _init_flecs();
  _init_raylib();
  _show_loading_screen();
  _init_managers();
}

//------------------------------------------------------------------------------

static inline int _collatz(int num)
{
  int count = 0;
  for (; num != 1; ++count)
    num = (num % 2 == 0) ? num / 2 : 3 * num + 1;
  return count;
}

//------------------------------------------------------------------------------

static int _factors(int num)
{
  int factors = 0;
  for (int i = 0; i < _num; ++i)
  {
    if (SQR(_primes[i]) > num)
      break;
    if (num % _primes[i] == 0)
      ++factors;
  }
  if (factors == 0 && num > 1)
  {
    if (_num < 100000)
      _primes[_num++] = num;
    else
      factors = 999;
  }
  return factors;
}

//------------------------------------------------------------------------------

static inline Color _process(int num)
{
  return WHITE;
  switch (_factors(num))
  {
  case 0:
    return (Color){255, 255, 255, 255};
  case 999:
    return RED;
  default:
    return (Color){0};
  }
}

//------------------------------------------------------------------------------

void game_manager_loop(void)
{
  bool running = true;
  RenderTexture2D *playfield = texture_manager_playfield();
  Rectangle src = {0.0f, 0.0f, (float)RASTER_WIDTH, (float)RASTER_HEIGHT};

  float angle = 0;
  float length = STEP;
  int num = 1;

  // entity_manager_spawn_scene(SCENE_SPLASH);

  BeginTextureMode(*playfield);
  ClearBackground(BLACK);
  EndTextureMode();

  while (running)
  {
    //  running = ecs_progress(_world, GetFrameTime()) && !WindowShouldClose();
    running = !WindowShouldClose();

    int window_width = GetScreenWidth();
    int window_height = GetScreenHeight();

#ifdef MAC
    window_width *= GetWindowScaleDPI().x;
    window_height *= GetWindowScaleDPI().y;
#endif

    float scale = MIN((float)window_width / RASTER_WIDTH, (float)window_height / RASTER_HEIGHT);
    Rectangle dst = {
        (window_width - ((float)RASTER_WIDTH * scale)) * 0.5f,
        (window_height - ((float)RASTER_HEIGHT * scale)) * 0.5f,
        (float)RASTER_WIDTH * scale,
        (float)RASTER_HEIGHT * scale};

    BeginTextureMode(*playfield);

    for (int i = 0; i < 1000; ++i)
    {
      float delta = angle * 0.002777777777777778; // how far around circle
      float current = length + STEP * delta;      // length to draw this frame
      float angle_delta = 2 * asinf(STEP / current) * RAD2DEG;
      Vector2 position = Vector2Rotate((Vector2){current, 0}, angle);
      position = Vector2Add(position, (Vector2){RASTER_WIDTH * 0.5, RASTER_HEIGHT * 0.5});
      Color colour = _process(num);
      if (colour.a > 0)
        DrawPixelV(position, colour);
      num += 1;
      angle += angle_delta;
      while (angle > 360)
      {
        angle -= 360;
        length += STEP;
      }
    }

    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexturePro(playfield->texture, src, dst, (Vector2){0}, 0.0f, WHITE);
    EndDrawing();
  }
}

//------------------------------------------------------------------------------

void game_manager_fini(void)
{
  ecs_fini(_world);
  _world = NULL;
}
