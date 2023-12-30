#include <iostream>
#include <unordered_map>
#include <vector>
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
  float Cross(const v2d &o) {
    return x * o.y - y * o.x;
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
  v2d Project(v2d axis) {
    if (axis.SqrMagnitude() == 0.0)
      return *this;
    axis = axis.Normalized();
    return axis * Dot(axis);
  }
  v2d ProjectTangent(v2d axis) {
    if (axis.SqrMagnitude() == 0.0)
      return *this;
    axis = axis.Normalized();
    axis = { axis.y, -axis.x };
    float proj = Dot(axis);
    return axis * proj;
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
  v2d &operator*=(const float f) {
    x *= f;
    y *= f;
    return *this;
  }
  v2d &operator/=(const float f) {
    x /= f;
    y /= f;
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

void DrawRay(v2d pos, v2d ray) {
  const float kRoot2Over2 = 0.70711;
  
  SDL_Point pts[3];
  pts[0].x = pos.x;
  pts[0].y = pos.y;
  pts[1].x = pos.x + ray.x;
  pts[1].y = pos.y + ray.y;
  
  ray = ray.Normalized();
  v2d tangent = { ray.y, -ray.x };

  pts[2].x = pts[1].x + tangent.x * kRoot2Over2;
  pts[2].y = pts[1].y + tangent.y * kRoot2Over2;

  SDL_RenderDrawLines(sdl_renderer, pts, 2);
}

/* Get a random uint32_t using ticks since start */
uint32_t Random() {
  return SDL_GetTicks();
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
    debug_.reserve(512);
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
    for (const Debug &d : debug_) {
      sdl::SetColor(100, 255, 230);
      sdl::DrawRay(d.pos, d.vec);
    }
    debug_.clear();
    sdl::EndDraw();
  }
  void DebugRay(v2d pos, v2d ray) {
    debug_.push_back({ pos, ray });
  }
 private:
  std::unordered_map<size_t, struct Attributes> map_;
  
  /* Struct for holding debug objects -- just rays for now */
  struct Debug {
    v2d pos;
    v2d vec;
  };
  std::vector<struct Debug> debug_;
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

/* Linearly interpolate x to y by percent p */
float Lerp(float x, float y, float p) {
  return x + (y - x) * p;
}

int main(int argv, char** args) {
  /* Initialize */
  Object ship;
  Object bullet;

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
  v2d vel_max = { 0.0, 0.0 };
  /* Whether the ship was far enough from the center of the screen to trigger a bullet */
  bool bullet_is_armed = false;

  /* Motion variables for the bullet */
  v2d bullet_vel;

  /* Booleans as stand-ins for an object handling system */
  bool bullet_is_active = false;
  bool ship_is_active = true;
  bool enemy_is_active[256] = {0};

  /*** Enemy info initialization ***/
  
  struct Enemy {
    Object obj;
    bool is_active = false;
    
    /* Enemy travels along these axes */
    v2d origin;
    v2d y_axis;
    v2d x_axis;

    float elapsed;
    float expiry;
    float t;

    /* ax^2 + bx + c = y, where a = beta / alpha^2 = coeff */
    float beta;
    float alpha;
    float coeff;
  };
  Enemy enemies[256];
  float enemy_spawn_timer = 1.0;

  /* Entrance and exit points for enemies */
  const size_t kSide = 16, kPoints = kSide * 4; 
  v2d enemy_spawn_points[kPoints];

  /* Set them up around the edges of the screen */
  {
    size_t p = 0;
    float x_off = sdl::kWindowX / kSide;
    float y_off = sdl::kWindowY / kSide;
    for (; x_off < sdl::kWindowX;) {
      /* top edge */
      enemy_spawn_points[kSide * 0 + p++] = { x_off, 0 };
      /* left edge */
      enemy_spawn_points[kSide * 1 + p++] = { 0, y_off };
      /* bottom edge */
      enemy_spawn_points[kSide * 2 + p++] = { x_off, sdl::kWindowY };
      /* right edge */
      enemy_spawn_points[kSide * 3 + p++] = { sdl::kWindowX, y_off };
      x_off += sdl::kWindowX / kSide;
      y_off += sdl::kWindowY / kSide;
    }
  }

  /**********************/

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

      v2d axis = ship.pos - center_of_screen;

      if (input.lmb.held) {
        v2d relative_mouse_offset = input.cursor - init_mouse_pos;
        v2d prev_ship_vel = vel;
        v2d prev_ship_pos = ship.pos;
        float distance_to_center =
          prev_ship_pos.Distance(center_of_screen);
        v2d goal_ship_pos =
          init_ship_pos +
          relative_mouse_offset /
          (distance_to_center / 64.0 + 1.0);
        Spring(ship.pos, vel, goal_ship_pos, 0.56, 8.0 * 3.14159, 0.016);

        if (distance_to_center > 40.0)
          if (!bullet_is_active)
            bullet_is_armed = true;
      } else {
        float distance_to_center = axis.Magnitude();

        v2d vel_norm = vel.Project(axis);
        v2d vel_tan = vel.ProjectTangent(axis);

        Spring(ship.pos, vel_norm, center_of_screen, 0.28, 8.0 * 3.14159, 0.016);
        Spring(ship.pos, vel_tan, center_of_screen, 0.28, 2.0 * 3.14159, 0.016);

        vel = vel_norm + vel_tan;

        drawer.DebugRay(ship.pos, vel_norm);
        drawer.DebugRay(ship.pos, vel_tan);

        /* If bullet is ready to fire, find the max velocity */
        if (!bullet_is_active)
          if (vel.SqrMagnitude() > vel_max.SqrMagnitude())
            vel_max = vel;

        /* Check if we're in range of the center. If so, initialize bullet */
        if (distance_to_center < 40.0)
          if (bullet_is_armed)
            if (!bullet_is_active) {
              bullet_is_active = true;
              bullet_is_armed = false;
              bullet_vel = vel.Normalized() * vel_max.Magnitude();
              vel_max = { 0.0, 0.0 };
              drawer.Enable(bullet);
            }
      }
    }

    if (bullet_is_active) {
      v2d axis = ship.pos - center_of_screen;

      Spring(bullet.pos, bullet_vel, center_of_screen, 0, 1.0 * 3.14159, 0.016);

      drawer.DebugRay(bullet.pos, bullet_vel);

      v2d offset_to_ship = bullet.pos - ship.pos;
    }

    for (size_t e = 0; e < 256; ++e) {
      if (enemies[e].is_active) {
        enemies[e].elapsed += 0.016;

        float percent_along_curve =
          enemies[e].elapsed / enemies[e].expiry;

        float x = Lerp(-1, 1, percent_along_curve);
        float sign = x < 0 ? -1 : 1;

        enemies[e].t =
          enemies[e].alpha *
          (x);//* sign;

        enemies[e].obj.pos =
          enemies[e].origin +
          enemies[e].x_axis * enemies[e].t +
          enemies[e].y_axis * enemies[e].coeff * enemies[e].t * enemies[e].t;

        /* Unregister if enemy traverses the whole path set out for it */
        if (enemies[e].elapsed > enemies[e].expiry) {
          enemies[e].is_active = false;
          drawer.Unregister(enemies[e].obj);
        }
      } else if (enemy_spawn_timer <= 0.0) {
        /***  found an inactive enemy object -- set it up ***/
        enemy_spawn_timer = 1.0;

        /* Clamp random number in range of spawn points */
        uint32_t r = sdl::Random() % kPoints;
        
        /* Pick a set of axes */
        enemies[e].origin = center_of_screen;
        enemies[e].y_axis = (enemy_spawn_points[r] - enemies[e].origin).Normalized();
        enemies[e].x_axis = { enemies[e].y_axis.y, -enemies[e].y_axis.x };

        /* 
         * Pick a spawn point and use it to calculate coeff
         * The spawn point must have a positive dot product w/ the axis
         * (i.e. it is on the same side of the x-axis as en_sp_pt[r])
         * (also must not be the same as the initial point)
         */
        v2d spawn_offset;
        for (size_t r2 = (r + kSide) % kPoints;;) {
          if (r2 == r) continue;
          r2 = (r2 + 1) % kPoints;
          spawn_offset = enemy_spawn_points[r2] - enemies[e].origin;
          if (spawn_offset.Dot(enemies[e].y_axis) > 0.0) break;
        }

        enemies[e].beta = spawn_offset.Dot(enemies[e].y_axis);
        enemies[e].alpha = spawn_offset.Dot(enemies[e].x_axis);
        enemies[e].coeff = enemies[e].beta / (enemies[e].alpha * enemies[e].alpha);
        enemies[e].elapsed = 0.0;
        enemies[e].expiry = 4.0;

        enemies[e].is_active = true;
        drawer.Register(enemies[e].obj);
      }
    }

    if (enemy_spawn_timer > 0.0)
        enemy_spawn_timer -= 0.016;

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