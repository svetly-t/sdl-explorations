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
  float SqrDistance(const v2d &o) {
    return (x - o.x) * (x - o.x) + (y - o.y) * (y - o.y);
  }
  float Distance(const v2d &o) {
    return sqrt(SqrDistance(o));
  }
  float SqrMagnitude() {
    return x * x + y * y;
  }
  float Magnitude() {
    return sqrt(x * x + y * y);
  }
  v2d Normalized() {
    float m = Magnitude();
    /* Can't normalize a zero-length vector */
    if (m == 0.0)
      return { 0.0, 0.0 };
    return v2d(x / m, y / m);
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
    /* In lieu of an angle, use a vector for rotation */
    v2d point_at = { 1.0, 0.0 };
    /* Pointer back to the object */
    Object *obj = nullptr;
  };

  Drawer() {
    map_.reserve(512);
    lines_.reserve(512);
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
  void PointAt(Object &o, v2d dir) {
    if (map_.find(o.key) == map_.end()) return;
    map_[o.key].point_at = dir;
  }
  void Draw() {
    sdl::StartDraw();
    for (const LineAttributes &l : lines_) {
      sdl::SetColor(l.attr.r, l.attr.g, l.attr.b);
      sdl::DrawLine(l.pos, l.vec, l.nub);
    }
    for (const auto &[_, attr] : map_) {
      if (!attr.enabled) continue;
      sdl::SetColor(attr.r, attr.g, attr.b);
      sdl::DrawRect(attr.obj->pos, attr.size, attr.point_at);
    }
    lines_.clear();
    sdl::EndDraw();
  }
  void Ray(v2d pos, v2d ray, struct Attributes attr) {
    lines_.push_back({ pos, ray, true, attr });
  }
  void Line(v2d pos, v2d vec, struct Attributes attr) {
    lines_.push_back({ pos, vec, false, attr });
  }
 private:
  std::unordered_map<size_t, struct Attributes> map_;
  
  /* Struct for holding lines */
  struct LineAttributes {
    v2d pos;
    v2d vec;
    bool nub;
    struct Attributes attr;
  };
  std::vector<struct LineAttributes> lines_;
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

 private:
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

/* Interpolate from zero to one according to h */
float InvTween(float h, float scale) {
  return 1.0 - 1.0 / (h * scale + 1.0);
}

int main(int argv, char** args) {
  /* Initialize */
  Input input;

  Drawer drawer;

  Overlap overlap;

  Update update;

  /* Set up SDL */
  sdl::Initialize();
  sdl::SetInput(input);

  v2d center_of_screen = { sdl::kWindowX / 2, sdl::kWindowY / 2 }; 

  /* When lmb is pressed, remember offset from these positions */
  v2d init_mouse_pos;

  /* Oh you're gonna love this */
  float hitstop_timer = 0.0;

  /*** Bullet initialization ***/

  struct Bullet {
    Object obj;
    bool is_active = false;
    
    v2d vel;
    v2d rot;
    float timer;
    float theta;
    float ground;
  };
  Bullet bullet;

  /* Reset bullet attributes with the Drawer */
  Drawer::Attributes attr = Drawer::Attributes();
  attr.size = 40;
  drawer.Register(bullet.obj, attr);
  drawer.Disable(bullet.obj);

  /*** Player initialization ***/

  struct Ship {
    Object obj;
    bool is_active = true;

    enum State {
      kMoving,
      kAiming,
      kThrowing
    };
    State state;

    /* Motion variables */
    const float kSpeed = 120.0;
    float timer;
    v2d acc;
    v2d vel;
    v2d pos;

    /* Rotation variables */
    v2d rot_vel;
    v2d rot;
  };
  Ship ship;
  ship.state = Ship::kMoving;
  ship.acc = { 0.0, 0.0 };
  ship.vel = { 0.0, 0.0 };
  ship.pos = center_of_screen;
  ship.rot_vel = { 0.0, 0.0 };
  ship.rot = { 1.0, 0.0 };
  ship.timer = 0.0;

  /* Register ship attributes with the Drawer */
  attr = Drawer::Attributes();
  attr.size = 20;
  attr.r = 200;
  attr.g = 150;
  attr.b = 0;
  drawer.Register(ship.obj, attr);

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

    if (hitstop_timer > 0.0) {
      hitstop_timer -= 0.016;
    } else {
      if (ship.is_active) {
        if (ship.state == Ship::kMoving) {
          /* Determine the goal velocity for this frame */
          v2d v = { 0.0, 0.0 };
          if (input.up.held)
            v.y -= 1.0;
          if (input.down.held)
            v.y += 1.0; 
          if (input.right.held)
            v.x += 1.0;
          if (input.left.held)
            v.x -= 1.0;
          v = v.Normalized() * ship.kSpeed;

          /* Use spring to interpolate velocity */
          Spring(ship.vel, ship.acc, v, 0.23, 4.0 * 3.14159, 0.016);
          /* Move the ship */
          SemiImplicitEuler(ship.pos, ship.vel, {0.0, 0.0}, 0.016);
          /* Apply cute bounce motion to the visual position of the ship */
          const float kSqrMaxShipSpeed = ship.kSpeed * ship.kSpeed;
          const float kBouncePeriodScale = 3.14159 / 2.0 * 7.5;
          float theta = ship.timer * kBouncePeriodScale;
          float bounce_scale = ship.vel.SqrMagnitude() / kSqrMaxShipSpeed;
          v2d bounce = { 0.0, -5.0 };
          bounce.y *= abs(sin(theta)) * bounce_scale;
          ship.obj.pos = ship.pos + bounce;
          /* Rotate the box to neutral position */
          float lerp_factor = InvTween(ship.timer, 8.0);
          ship.rot.x = Lerp(ship.rot.x, 1.0, lerp_factor);
          ship.rot.y = Lerp(ship.rot.y, -1.0, lerp_factor);
          drawer.PointAt(ship.obj, ship.rot);
          
          ship.timer += 0.016;

          if (input.lmb.pressed) {
            /* Store initial position in order to calc. offset */
            init_mouse_pos = input.cursor;
            /* Switch to aiming mode */
            ship.state = Ship::kAiming;
            /* Drop the bounce offset */
            ship.obj.pos = ship.pos;
            /* Reset the state_timer */
            ship.timer = 0.0;
          }
        } else if (ship.state == Ship::kAiming) {
          v2d relative_mouse_offset = input.cursor - init_mouse_pos;
          /* Draw the prediction arrow */
          struct Drawer::Attributes line;
          line.r = 255;
          line.g = 225;
          line.b = 140;
          drawer.Ray(ship.pos, -relative_mouse_offset, line);
          /* Rotate the box in the direction of the mouse */
          Spring(ship.rot, ship.rot_vel, -relative_mouse_offset, 0.23, 4.0 * 3.14159, 0.016);
          drawer.PointAt(ship.obj, ship.rot);
          /* Draw the "ground line" for the bullet */
          float lerp_factor = InvTween(ship.timer, 4.0);
          float line_length = Lerp(0, sdl::kWindowX, lerp_factor);
          v2d start = ship.pos;
          v2d dir = { 0.0, 0.0 };
          start.x -= line_length;
          dir.x += line_length * 2.0;
          line.r = 60;
          line.g = 60;
          line.b = 60;
          drawer.Line(start, dir, line);

          ship.timer += 0.016;

          if (input.lmb.up) {
            /* Initialize bullet if not active */
            if (!bullet.is_active) {
              const float kThrowDamp = 1.0;
              bullet.is_active = true;
              bullet.vel = -relative_mouse_offset * kThrowDamp;
              bullet.obj.pos = ship.pos + bullet.vel * 0.016;
              bullet.rot = { 1.0, 0.0 };
              bullet.timer = 0.0;
              bullet.ground = ship.pos.y;
              drawer.Enable(bullet.obj);
            }
            /* Switch to moving mode */
            ship.state = Ship::kMoving;
            ship.rot_vel = {0.0, 0.0};
            ship.timer = 0.0;
          }
        }
      }

      if (bullet.is_active) {
        /* Only move the bullet if it's above the "ground line" */
        if (bullet.obj.pos.y < bullet.ground) {
          float kGravity = (bullet.vel.y > 0.0) ? 4e2 : 2e2;
          SemiImplicitEuler(bullet.obj.pos, bullet.vel, { 0, kGravity }, 0.016);
        } else {
          bullet.vel = { 0.0, 0.0 };
        }

        /* visually rotate the bullet box */
        const float kRotScale = 0.1;
        bullet.rot = { (float)cos(bullet.theta), (float)sin(bullet.theta) };
        drawer.PointAt(bullet.obj, bullet.rot);
        bullet.theta += 0.016 * abs(bullet.vel.y) * kRotScale;
        
        bullet.timer += 0.016;
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
            (x);

          enemies[e].obj.pos =
            enemies[e].origin +
            enemies[e].x_axis * enemies[e].t +
            enemies[e].y_axis * enemies[e].coeff * enemies[e].t * enemies[e].t;

          bool hit =
            (!bullet.is_active) ?
            false :
            overlap.CircleCircle(10.0, enemies[e].obj.pos, 20.0, bullet.obj.pos);

          /* five-frame hitstop if collision with bullet */
          if (hit)
            hitstop_timer = 0.016 * 5;

          /* Unregister if enemy traverses the whole path set out for it (or hit by bullet) */
          if (enemies[e].elapsed > enemies[e].expiry || hit) {
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