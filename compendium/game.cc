#include <iostream>
#include <unordered_map>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <cstdlib>
#include <cmath>

struct v2d {
  float x = 0;
  float y = 0;
  v2d() {}
  v2d(float _x, float _y) {
    x = _x;
    y = _y;
  }
  float Dot(const v2d &o) {
    return x * o.x + y * o.y;
  }
  float Distance(const v2d &o) {
    float dist;
    dist  = (x - o.x) * (x - o.x);
    dist += (y - o.y) * (y - o.y);
    dist = sqrt(dist);
    return dist;
  }
  float SqrMagnitude() {
    return x * x + y * y;
  }
  float Magnitude() {
    return sqrt(x * x + y * y);
  }
  v2d Normalized() {
    float m = Magnitude();
    return v2d(x / m, y / m );
  }
  v2d operator*(const float f) const {
    return v2d(x * f, y * f);
  }
  v2d operator*(const double d) const {
    return v2d(x * d, y * d);
  }
  v2d operator/(const float f) const {
    return v2d(x / f, y / f);
  }
  v2d operator/(const double d) const {
    return v2d(x / d, y / d);
  }
  v2d &operator+=(const v2d &rhs) {
    x += rhs.x;
    y += rhs.y;
    return *this;
  }
  v2d &operator-=(const v2d &rhs) {
    x -= rhs.x;
    y -= rhs.y;
    return *this;
  }
  /* overload unary minus */
  v2d operator-() {
    return v2d(-x, -y);
  }
  /* overload subtract */
  v2d operator-(const v2d &rhs) {
    return v2d(x - rhs.x, y - rhs.y);
  }
  /* overload add */
  v2d operator+(const v2d &rhs) {
    return v2d(x + rhs.x, y + rhs.y);
  }
};

/*
 * Base Object class.
 * Every Object has a unique Key (its address in memory)
 * that can be used to associate it with its subsystem
 * attributes.
 */
struct Object {
  size_t key;
  v2d pos;
  Object() { key = (size_t)this; }
};

/* 
 * Input is a virtualization of the controller inputs.
 * You access buttons via Input::up, down, left, right, space...
 * But you have to register new buttons in the buttons array
 * so that their just-frame states (up, down) can be cleared
 * at the ends of frames. 
 */
struct Input {
  struct Button {
    bool up = false;
    bool pressed = false;
    bool held = false;
  };

  Button *buttons[6];
  Button up;
  Button down;
  Button left;
  Button right;
  Button space;
  Button lmb;

  v2d cursor;

  Input() {
    buttons[0] = &up;
    buttons[1] = &down;
    buttons[2] = &left;
    buttons[3] = &right;
    buttons[4] = &space;
    buttons[5] = &lmb;
  }

  void AtFrameEnd() {
    /* Clear the transient states */
    for (Button *button : buttons) {
      button->pressed = false;
      button->up = false;
    }
  }
};

namespace sdl {

/* Static SDL components */
SDL_Surface *sdl_surface;
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

  sdl_surface = SDL_GetWindowSurface(sdl_window);
  if (!sdl_surface) {
    std::cout << "No surface " << SDL_GetError() << std::endl;
    abort();
  }

  sdl_renderer = SDL_CreateSoftwareRenderer(sdl_surface);
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

/* Clear the view buffer, etc. */
void StartDraw() {
  SDL_FillRect(sdl_surface, NULL, SDL_MapRGB(sdl_surface->format, 0, 0, 0));
}

void EndDraw() {
  SDL_UpdateWindowSurface(sdl_window);
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

} // namespace sdl

class Drawer {
 public:
  struct Attributes {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    float size = 20;
    bool enabled = true;
    /* Pointer back to the object */
    Object *obj;
  };

  Drawer() {
    map_.reserve(512);
  }
  /* Default register */
  void Register(Object &object) {
    map_[object.key] = Attributes();
    map_[object.key].obj = &object;
  }
  void Register(Object &object, struct Attributes attr) {
    map_[object.key] = attr;
    map_[object.key].obj = &object;
  }
  void Unregister(Object &object) {
    if (map_.find(object.key) == map_.end()) return;
    map_.erase(object.key);
  }
  void Enable(Object &o) {
    if (map_.find(o.key) == map_.end()) return;
    map_[o.key].enabled = true;
  }
  void Disable(Object &o) {
    if (map_.find(o.key) == map_.end()) return;
    map_[o.key].enabled = false;
  }
  void Draw() {
    sdl::StartDraw();
    for (const auto &[_, attr] : map_) {
      if (!attr.enabled) continue;
      sdl::SetColor(attr.r, attr.g, attr.b);
      sdl::DrawRect(attr.obj->pos, attr.size);
    }
    sdl::EndDraw();
  }
 private:
  std::unordered_map<size_t, struct Attributes> map_;
}; // class Drawer

class Overlap {
 public:
  struct Attributes {
    enum Type {
      AABB,
      Circle
    };
    Type type = Circle;
    /* type-specific traits; width and height for aabb, radius for circle */
    union {
      struct {
        float w;
        float h;
      };
      struct {
        float r;
      };
    } traits;
    /* Pointer back to the object */
    Object *obj;
  };
  void Register(Object &object) {
    map_[object.key] = Attributes();
    map_[object.key].obj = &object;
  }
  void Register(Object &object, struct Attributes attr) {
    map_[object.key] = attr;
    map_[object.key].obj = &object;
  }
  void Unregister(Object &object) {
    if (map_.find(object.key) == map_.end()) return;
    map_.erase(object.key);
  }
  bool Check(const Object &a, const Object &b) {
    /* Check both objects are registered in the subsystem */
    if (map_.find(a.key) == map_.end()) return false;
    if (map_.find(b.key) == map_.end()) return false;
    /* Get their attributes */
    struct Attributes *at = &map_[a.key];
    struct Attributes *bt = &map_[b.key];
    /* Call appropriate overlap function */
    if (at->type == Attributes::AABB && bt->type == Attributes::AABB)
      return AabbAabb(
        at->traits.w, at->traits.h, at->obj->pos,
        bt->traits.w, bt->traits.h, bt->obj->pos
      );
    if (at->type == Attributes::Circle && bt->type == Attributes::Circle)
      return CircleCircle(
        at->traits.r, at->obj->pos,
        bt->traits.r, bt->obj->pos
      );
  }
 private:
  /* type-type overlap functions */
  bool AabbAabb(float w1, float h1, v2d pos1, float w2, float h2, v2d pos2) {
    return (pos1.x - w1) <= (pos2.x + w2) &&
           (pos1.x + w1) >= (pos2.x - w2) &&
           (pos1.y - h1) <= (pos2.y + h2) &&
           (pos1.y + h1) >= (pos2.y - h2);
  }
  bool CircleCircle(float r1, v2d pos1, float r2, v2d pos2) {
    return pos1.Distance(pos2) < (r1 + r2);
  }

  std::unordered_map<size_t, struct Attributes> map_;
}; // class Overlap

class Update {
 public:
  struct Attributes {
    void (*func)(void *ctx);
    void *ctx;
    bool enabled = true;
  };
  Update() {
    map_.reserve(512);
  }
  void Register(Object &o, Attributes attr) {
    map_[o.key] = attr;
  }
  void Enable(Object &o) {
    if (map_.find(o.key) == map_.end()) return;
    map_[o.key].enabled = true;
  }
  void Disable(Object &o) {
    if (map_.find(o.key) == map_.end()) return;
    map_[o.key].enabled = false;
  }
 private:
  /* Map holds update functions and contexts */
  std::unordered_map<size_t, struct Attributes> map_;
}; // class Update

/* Pulled from https://allenchou.net/2015/04/game-math-precise-control-over-numeric-springing/ */
void Spring(float &x, float &v, float xt, float zeta, float omega, float h) {
  float f = 1.0f + 2.0f * h * zeta * omega;
  float oo = omega * omega;
  float hoo = h * oo;
  float hhoo = h * hoo;
  float detInv = 1.0f / (f + hhoo);
  float detX = f * x + h * v + hhoo * xt;
  float detV = v + hoo * (xt - x);
  x = detX * detInv;
  v = detV * detInv;
}

/* ditto for 2d vectors */
void Spring(v2d &i, v2d &v, v2d t, float zeta, float omega, float h) {
  Spring(i.x, v.x, t.x, zeta, omega, h);
  Spring(i.y, v.y, t.y, zeta, omega, h);
}

void Verlet(float &x, float xp, float a, float h) {
  float xn = x;
  x = 2 * xn - xp + h * h * a;
}

void SemiImplicitEuler(float &x, float &v, float a, float h) {
  v = v + a * h;
  x = x + v * h;
}

void SemiImplicitEuler(v2d &pos, v2d &vel, v2d a, float h) {
  SemiImplicitEuler(pos.x, vel.x, a.x, h);
  SemiImplicitEuler(pos.y, vel.y, a.y, h);
}

int main(int argv, char** args) {
  /* Initialize */
  Object ship;
  Object bullet;
  Object enemies[256];
  Object souls[256];
  Object mouse;

  Input input;

  Drawer drawer;

  Overlap overlap;

  Update update;

  /* Set up SDL */
  sdl::Initialize();
  sdl::SetInput(input);

  /* Register game objects with the Drawer */
  struct Drawer::Attributes attr;
  attr.size = 20;
  attr.r = 200;
  attr.g = 150;
  attr.b = 0;
  drawer.Register(ship, attr);

  /* Reset the attributes for the bullet */
  attr = Drawer::Attributes();
  attr.size = 40;
  drawer.Register(bullet, attr);
  drawer.Disable(bullet);

  /* start the ship in the middle of the screen */
  v2d center_of_screen = { sdl::kWindowX / 2, sdl::kWindowY / 2 }; 
  ship.pos = center_of_screen;
  bullet.pos = center_of_screen;

  /* When lmb is pressed, remember offset from these positions */
  v2d init_mouse_pos;
  v2d init_ship_pos;
  /* Motion variables for the ship */
  v2d vel;
  v2d max_vel = { 0.0, 0.0 };
  /* Whether the ship was far enough from the center of the screen to trigger a bullet */
  bool bullet_is_armed = false;

  /* Motion variables for the bullet */
  v2d bullet_vel;

  /* Booleans as stand-ins for an object handling system */
  bool bullet_is_active = false;
  bool ship_is_active = true;

  for (;;) {
    /* Get events from SDL's event system */
    if (sdl::GetEvents(input)) break;

    /*** Update objects ***/

    if (ship_is_active) {
      if (input.lmb.pressed) {
        init_mouse_pos = input.cursor;
        init_ship_pos = ship.pos;
        vel = { 0.0, 0.0 };
      }

      if (input.lmb.held) {
        v2d relative_mouse_offset = input.cursor - init_mouse_pos;
        v2d prev_ship_pos = ship.pos;
        float distance_to_center =
          prev_ship_pos.Distance(center_of_screen);
        v2d goal_ship_pos =
          init_ship_pos +
          relative_mouse_offset /
          (distance_to_center / 64.0 + 1.0);
        Spring(ship.pos, vel, goal_ship_pos, 0.28, 8.0 * 3.14159, 0.016);

        if (distance_to_center > 20.0)
          if (!bullet_is_active)
            bullet_is_armed = true;
        
        if (bullet_is_armed) {
          if (abs(vel.x) > abs(max_vel.x))
            max_vel.x = vel.x;
          if (abs(vel.y) > abs(max_vel.y))
            max_vel.y = vel.y;
        }
      } else {
        Spring(ship.pos, vel, center_of_screen, 0.21, 8.0 * 3.14159, 0.016);

        /* Check if we're in range of the center. If so, initialize bullet */
        float distance_to_center = ship.pos.Distance(center_of_screen);
        if (distance_to_center < 20.0)
          if (bullet_is_armed)
            if (!bullet_is_active) {
              bullet_is_active = true;
              bullet_is_armed = false;
              bullet_vel = -max_vel;
              drawer.Enable(bullet);
            }
      }
    }

    if (bullet_is_active) {
      v2d vec_to_center = center_of_screen - bullet.pos;

      /* Bullet should travel in arc */
      v2d acc = vec_to_center.Normalized() * 400.0;

      /* Hack for when bullet is in exact center of screen */
      if (vec_to_center.SqrMagnitude() < 1.0) acc = { 0.0, 0.0 };

      SemiImplicitEuler(bullet.pos, bullet_vel, acc, 0.016);

      /* Check if bullet is in range of center -- if yes, then disable */
      float distance_to_center = vec_to_center.Magnitude();
      // if (distance_to_center < 4.0) {
      //   v2d center_to_here = bullet.pos - center_of_screen;
      //   if (bullet_vel.Dot(center_to_here) < 0) {
      //     bullet_is_active = false;
      //     drawer.Disable(bullet);
      //   } 
      // }
    }

    /**********************/

    /* Clear transient state for buttons */
    input.AtFrameEnd();

    /* Draw all the objects */
    drawer.Draw();

    /* "Frame time" */
    SDL_Delay(15);
  }

  return 0;
}