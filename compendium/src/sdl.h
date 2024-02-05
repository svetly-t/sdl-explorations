#ifndef SDL_WRAP_H
#define SDL_WRAP_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "input.h"

namespace sdl {

/* Static SDL components */
SDL_Renderer *sdl_renderer;
SDL_Window *sdl_window;
const int kWindowX = 768;
const int kWindowY = 432;

/* EventToInput encapsulates translation of events from event loop
 * into the Input struct. Disentangles our input system from that of
 * SDL. */
class EventToInput {
 private:
  enum Buttons {
    kBaseScancodes = 284,
    kMaxScancodes = 286
  };
  Input::Button *keycode_map_[Buttons::kMaxScancodes] = {0};
 public:
  /* 
   * Define "scancodes" for mouse buttons, so that we can register them 
   * using the same interface. 
   */
  const static int SDL_SCANCODE_LMB = Buttons::kBaseScancodes;
  const static int SDL_SCANCODE_RMB = Buttons::kBaseScancodes + 1;
  void RegisterButton(int sc, Input::Button &button) {
    keycode_map_[sc] = &button;
  }
  void TranslateKeyDown(int sc) {
    /* Ignore unbound keypresses */
    if (!keycode_map_[sc]) return;

    keycode_map_[sc]->up = false;
    
    /* If the button's already held, don't set down */
    if (keycode_map_[sc]->held) {
      keycode_map_[sc]->pressed = false;
      return;
    }

    /* Else set down and held */
    keycode_map_[sc]->pressed = true;
    keycode_map_[sc]->held = true;
  }
  void TranslateKeyUp(int sc) {
    /* Ignore unbound keypresses */
    if (!keycode_map_[sc]) return;

    /* Unset held and down, and set up */
    keycode_map_[sc]->pressed = false;
    keycode_map_[sc]->held = false;
    keycode_map_[sc]->up = true;
  }
};

/* Static object that translates from SDL events to inputs */
EventToInput event_to_input;

/* Initialize SDL stuff */
void Initialize() {
  SDL_Init(SDL_INIT_EVERYTHING);
  
  sdl_window = SDL_CreateWindow(
    "Newboy",
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    kWindowX,
    kWindowY,
    SDL_WINDOW_ALLOW_HIGHDPI
  );

  if (!sdl_window) {
    std::cout << "Could not create window " << SDL_GetError() << std::endl;
    abort();
  }

  // Constrain mouse to screen
  // SDL_SetRelativeMouseMode(SDL_TRUE);

  // sdl_surface = SDL_GetWindowSurface(sdl_window);
  // if (!sdl_surface) {
  //   std::cout << "No surface " << SDL_GetError() << std::endl;
  //   abort();
  // }

  // sdl_renderer = SDL_CreateSoftwareRenderer(sdl_surface);

  sdl_renderer = SDL_CreateRenderer(sdl_window, -1, 0);

  if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
    std::cout << "could not load dlls for PNG display" << std::endl;
    abort();
  }
}

void SetInput(Input &input) {
  event_to_input.RegisterButton(SDL_SCANCODE_A, input.left);
  event_to_input.RegisterButton(SDL_SCANCODE_S, input.down);
  event_to_input.RegisterButton(SDL_SCANCODE_D, input.right);
  event_to_input.RegisterButton(SDL_SCANCODE_W, input.up);
  event_to_input.RegisterButton(SDL_SCANCODE_SPACE, input.space);
  event_to_input.RegisterButton(EventToInput::SDL_SCANCODE_LMB, input.lmb);
}

/* Update our buttons/window state if there's anything in the event queue */
int GetEvents(Input &input) {
  SDL_Event sdl_event;

  for (;SDL_PollEvent(&sdl_event) > 0;) {
    switch (sdl_event.type) {
      case SDL_QUIT:
        return -1;
      case SDL_MOUSEMOTION:
        input.cursor.x = sdl_event.motion.x;
        input.cursor.y = sdl_event.motion.y;
        break;
      case SDL_MOUSEBUTTONDOWN:
        event_to_input.TranslateKeyDown(EventToInput::SDL_SCANCODE_LMB);
        break;
      case SDL_MOUSEBUTTONUP:
        event_to_input.TranslateKeyUp(EventToInput::SDL_SCANCODE_LMB);
        break;
      case SDL_KEYDOWN:
        event_to_input.TranslateKeyDown(sdl_event.key.keysym.scancode);
        break;
      case SDL_KEYUP:
        event_to_input.TranslateKeyUp(sdl_event.key.keysym.scancode);
        break;
    }
  }

  return 0;
}

/* Load a texture from a file and return a SDL_Texture pointer */
SDL_Texture *LoadTexture(std::string file) {
  SDL_Surface *surf = IMG_Load(file.c_str());
  if (!surf)
    std::cout << "loading img returned error" << std::endl;
  
  SDL_Texture *text = SDL_CreateTextureFromSurface(sdl_renderer, surf);
  if (!text)
    std::cout << "creating texture failed error: " << SDL_GetError() << std::endl;
  
  SDL_FreeSurface(surf);
  return text;
}

/* Clear the view buffer, etc. */
void StartDraw() {
  // SDL_FillRect(sdl_surface, NULL, SDL_MapRGB(sdl_surface->format, 0, 0, 0));
  SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
  SDL_RenderClear(sdl_renderer);
}

void EndDraw() {
  // SDL_UpdateWindowSurface(sdl_window);
  SDL_RenderPresent(sdl_renderer);
}

/* Set color for the next draw thing */
void SetColor(int r, int g, int b) {
  SDL_SetRenderDrawColor(sdl_renderer, r, g, b, 255);
}

void DrawRect(v2d pos, float side) {
  SDL_Rect rect;
  int half = side / 2;
  rect.x = pos.x - half;
  rect.y = pos.y - half;
  rect.w = side;
  rect.h = side;
  SDL_RenderDrawRect(sdl_renderer, &rect);
}

/* Draw a rectangle with the top-right corner pointing at "corner" */
void DrawRect(v2d pos, float side, v2d corner) {
  const float kRoot2 = 1.41421356;
  float diag = side / 2.0 * kRoot2;
  corner = corner.Normalized();
  corner *= diag;
  
  v2d perpen = { -corner.y, corner.x };
  SDL_Point pts[5];
  pts[0].x = pts[4].x = pos.x + corner.x;
  pts[0].y = pts[4].y = pos.y + corner.y;
  pts[1].x = pos.x - perpen.x;
  pts[1].y = pos.y - perpen.y;
  pts[2].x = pos.x - corner.x;
  pts[2].y = pos.y - corner.y;
  pts[3].x = pos.x + perpen.x;
  pts[3].y = pos.y + perpen.y;
  SDL_RenderDrawLines(sdl_renderer, pts, 5);
}

void DrawLine(v2d pos, v2d ray, bool nub) {
  const float kNibLength = 10.0;

  SDL_Point pts[3];
  pts[0].x = pos.x;
  pts[0].y = pos.y;
  pts[1].x = pos.x + ray.x;
  pts[1].y = pos.y + ray.y;

  ray = ray.Normalized();
  v2d tangent = { ray.y, -ray.x };

  ray *= kNibLength;
  pts[2] = pts[1];
  pts[2].x -= ray.x;
  pts[2].y -= ray.y;
  pts[2].x += tangent.x * kNibLength;
  pts[2].y += tangent.y * kNibLength;

  int count = 2 + nub;

  SDL_RenderDrawLines(sdl_renderer, pts, count);  
}

void DrawTexture(SDL_Texture *texture, v2d dest, v2d source, float h, float w, float theta) {
  SDL_Rect src_rect;
  src_rect.x = source.x;
  src_rect.y = source.y;
  src_rect.w = w;
  src_rect.h = h;
  SDL_Rect dst_rect;
  dst_rect.x = dest.x;
  dst_rect.y = dest.y;
  dst_rect.w = w;
  dst_rect.h = h;
  SDL_RenderCopyEx(
    sdl_renderer,
    texture,
    &src_rect,
    &dst_rect,
    theta,
    nullptr,
    SDL_FLIP_NONE
  );
}

/* Get a random uint32_t using ticks since start */
uint32_t Random() {
  return SDL_GetTicks();
}

} // namespace sdl

#endif