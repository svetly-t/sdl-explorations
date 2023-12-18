#include <unordered_map>

struct v2d {
  float x = 0;
  float y = 0;
  v2d() {}
  v2d(float _x, float _y) {
    x = _x;
    y = _y;
  }
  float Distance(const v2d &o) {
    float dist;
    dist  = (x - o.x) * (x - o.x);
    dist += (y - o.y) * (y - o.y);
    dist = sqrt(dist);
    return dist;
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
  Object() { key = (size_t)this; }
  size_t key;
  v2d position;
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
    bool down = false;
    bool held = false;
  };

  Input() {
    buttons[0] = &up;
    buttons[1] = &down;
    buttons[2] = &left;
    buttons[3] = &right;
    buttons[4] = &space;
  }

  void AtFrameEnd() {
    /* Clear the transient states */
    for (Button *button : buttons) {
      button->down = false;
      button->up = false;
    }
  }

  Button *buttons[5];
  Button up;
  Button down;
  Button left;
  Button right;
  Button space;
};

namespace sdl {

/* Static SDL components */
SDL_Surface *sdl_surface;
SDL_Renderer *sdl_renderer;
SDL_Window *sdl_window;
const int kWindowX = 800;
const int kWindowY = 600;

/* EventToInput encapsulates translation of events from event loop
 * into the Input struct. Disentangles our input system from that of
 * SDL. */
class EventToInput {
 public:
  void RegisterButton(SDL_Scancode sc, Input::Button &button) {
    keycode_map_[(int)sc] = &button;
  }
  void TranslateKeyDown(SDL_Scancode sc) {
    /* Ignore unbound keypresses */
    if (!keycode_map_[(int)sc]) return;

    keycode_map_[(int)sc]->up = false;
    
    /* If the button's already held, don't set down */
    if (keycode_map_[(int)sc]->held) {
      keycode_map_[(int)sc]->down = false;
      return;
    }

    /* Else set down and held */
    keycode_map_[(int)sc]->down = true;
    keycode_map_[(int)sc]->held = true;
  }
  void TranslateKeyUp(SDL_Scancode sc) {
    /* Ignore unbound keypresses */
    if (!keycode_map_[(int)sc]) return;

    /* Unset held and down, and set up */
    keycode_map_[(int)sc]->down = false;
    keycode_map_[(int)sc]->held = false;
    keycode_map_[(int)sc]->up = true;
  }
 private:
  const int kButtons = 284;
  Input::Button *keycode_map_[kButtons] = {0};
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
  SDL_SetRelativeMouseMode(SDL_TRUE);

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
}

/* Update our buttons/window state if there's anything in the event queue */
void GetEvents(Input &input) {
  SDL_Event sdl_event;

  for (;SDL_PollEvent(&sdl_event) > 0;) {
    switch (sdl_event.type) {
      case SDL_QUIT:
        exit = true;
        break;
      case SDL_MOUSEMOTION:
        break;
      case SDL_MOUSEBUTTONDOWN:
        break;
      case SDL_MOUSEBUTTONUP:
        break;
      case SDL_KEYDOWN:
        event_to_input.TranslateKeyDown(sdl_event.key.keysym.scancode);
        break;
      case SDL_KEYUP:
        event_to_input.TranslateKeyUp(sdl_event.key.keysym.scancode);
        break;
    }
  }
}

/* Clear the view buffer, etc. */
void StartDraw() {
  SDL_FillRect(sdl_surface, NULL, SDL_MapRGB(sdl_surface->format, 0, 0, 0));
}

void DrawRect(v2d pos) {
  SDL_Rect rect;
  rect.x = pos.x - 10;
  rect.x = pos.x - 10;
  rect.w = 20;
  rect.h = 20;
  SDL_RenderDrawRect(sdl_renderer, &rect);
}

} // namespace sdl

class Drawer {
 public:
  struct Attributes {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
  };

  Drawer() {
    map_.reserve(512);
  }
  void Register(Object &object) {
    map_[object.key] = struct Attributes();
  }
  void Unregister(Object &object) {
    if (map_.find(object.key) == map_.end()) return;
    map_.erase(object.key);
  }
  void Draw() {
    sdl::StartDraw();
    for (const auto &[obj, attr] : map_) {
      sdl::SetColor();
      sdl::DrawRect(obj.pos);
    }
    sdl::EndDraw();
  }
  void Update(Object &object, struct Attributes attr) {

  }
 private:
  std::unordered_map<size_t, struct Attributes> map_;
} // class Drawer


int main() {
  /* Initialize */
  Object ship;
  Object bullet;
  Object enemies[256];

  Input input;

  Drawer drawer;

  sdl::Initialize();
  sdl::SetInput(input);

  for (;;) {
    /* Event loop */
    sdl::GetEvents(input);

    /*** Update objects ***/

    /* move the ship */
    if (input.up.held) --ship.pos.y;
    if (input.down.held) ++ship.pos.y;
    if (input.left.held) --ship.pos.x;
    if (input.right.held) ++ship.pos.x;
    
    /**/

    input.AtFrameEnd();

    /* Draw all the objects */
    drawer.Draw();
  }
}