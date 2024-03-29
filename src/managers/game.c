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

static inline int _collatz(unsigned int num)
{
  int count = 0;
  unsigned int max = num;
  for (; num != 1; ++count)
  {
    if ((num < max) || ((num & (num - 1)) == 0))
      break;
    num = (num % 2 == 0) ? (num / 2) : ((3 * num + 1) / 2);
  }

  return count;
}

//------------------------------------------------------------------------------

static inline int _factors(int num)
{
  if (num == 0)
    return 999;
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
    {
      _primes[_num] = num;
      ++_num;
      return 0;
    }
    else
      factors = 999;
  }
  return factors;
}

//------------------------------------------------------------------------------

static inline int _perfect(int num)
{
  for (int i = 0; i < 100000; ++i)
  {
    int sqr = SQR(i);
    if (sqr > num)
      return 1;
    if (sqr == num)
      return 0;
  }
}

//------------------------------------------------------------------------------

static inline Color _process(unsigned int num)
{
  int count = _collatz(num);
  int alpha = Clamp(16 * count, 0, 255);
  return (Color){255, 255, 255, alpha};
}

//------------------------------------------------------------------------------

void game_manager_loop(void)
{
  bool running = true;
  RenderTexture2D *playfield = texture_manager_playfield();
  Rectangle src = {0.0f, 0.0f, (float)RASTER_WIDTH, (float)RASTER_HEIGHT};

  unsigned int num = 0;
  unsigned int square = 1;
  int step = 0;
  int steps = 1;
  int length = 0;

  BeginTextureMode(*playfield);
  ClearBackground(BLACK);
  EndTextureMode();

  while (running && num < 33554432)
  {
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
      float delta = (float)step / (float)steps;
      float angle = 360 - 360 * delta;
      float current = length + STEP * delta;
      Vector2 position = Vector2Rotate((Vector2){current, 0}, angle);
      position.y *= -1;
      position = Vector2Add(position, (Vector2){RASTER_WIDTH * 0.5, RASTER_HEIGHT * 0.5});
      Color colour = _process(num);
      if (colour.a > 0)
        DrawPixelV(position, colour);
      num += 1;
      step += 1;
      if (step == steps)
      {
        length += STEP;
        steps += 2;
        square += steps;
        step = 0;
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
