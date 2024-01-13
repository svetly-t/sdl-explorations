#include <cstdlib>
#include <cmath>

#include "drawer.h"
#include "input.h"
#include "object.h"
#include "overlap.h"
#include "sdl.h"
#include "vector.h"

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

v2d Lerp(v2d x, v2d xt, float p) {
  return x + (xt - x) * p;
}

/* Interpolate from zero to one according to h */
float InvTween(float h, float scale) {
  return 1.0 - 1.0 / (h * scale + 1.0);
}

float Deg2Rad(float degrees) {
  return degrees / 360.0 * 2.0 * 3.14159;
}

int main(int argv, char** args) {
  /* Initialize */
  Input input;

  Drawer drawer;

  Overlap overlap;

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
    bool is_active = true;
    
    enum State {
      kIdle,
      kFalling,
      kGrounded
    };
    State state = State::kIdle;

    v2d vel = { 0.0, 0.0 };
    v2d rot = { 0.0, 0.0 };
    float timer = 0.0;
    float theta = 0.0;
    float ground;

    const float kDefaultSpin = 2.0;
    float spin_magnitude = kDefaultSpin;

    int hits = 0;
  };
  Bullet bullet;

  /* Reset bullet attributes with the Drawer */
  Drawer::Attributes attr = Drawer::Attributes();
  attr.size = 40;
  attr.r = 255;
  attr.g = 255;
  attr.b = 255;
  drawer.Register(bullet.obj, attr);

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

  /*** Soul initialization ***/

  struct Soul {
    Object obj;
    bool is_active = false;

    enum State {
      kFollowingEnemy,
      kFollowingBullet,
      kFollowingShip,
      kBouncing,
      kWaiting
    };
    State state;
    Object *follow;

    v2d axis;

    const float kOmega = 2.0 * 3.14159;
    const float kStopTime = 3.0;
    v2d pos;
    v2d vel;
    v2d acc;

    float timer;
  };
  Soul souls[256];

  struct SoulEmitter {
    v2d position;
    float initial_speed = 0.0;
    size_t count = 0;
    size_t index = 0;
  };
  SoulEmitter soul_emitter;

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
          float speed =
            (bullet.state == Bullet::kIdle) ?
            ship.kSpeed :
            ship.kSpeed * 2.0;
          v = v.Normalized() * speed;

          /* Use spring to interpolate velocity */
          Spring(ship.vel, ship.acc, v, 0.23, 4.0 * 3.14159, 0.016);
          /* Move the ship */
          SemiImplicitEuler(ship.pos, ship.vel, {0.0, 0.0}, 0.016);
          /* If the ship is out of bounds, push it back according to vel */
          if (ship.pos.x > sdl::kWindowX)
            ship.pos.x -= ship.vel.x * 0.016;
          if (ship.pos.x < 0.0)
            ship.pos.x -= ship.vel.x * 0.016;
          if (ship.pos.y > sdl::kWindowY)
            ship.pos.y -= ship.vel.y * 0.016;
          if (ship.pos.y < 0.0)
            ship.pos.y -= ship.vel.y * 0.016;
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
          const float kThrowDamp = 1.0;
          
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
          /* Increase rotation speed of bullet based on length of mouse offset */
          bullet.spin_magnitude = relative_mouse_offset.Magnitude() * kThrowDamp;

          ship.timer += 0.016;

          if (input.lmb.up) {
            /* Initialize bullet if not active */
            if (bullet.state == Bullet::kIdle) {
              bullet.is_active = true;
              bullet.vel = -relative_mouse_offset * kThrowDamp;
              bullet.obj.pos = ship.pos + bullet.vel * 0.016;
              bullet.rot = { 1.0, 0.0 };
              bullet.timer = 0.0;
              bullet.theta = 0.0;
              bullet.ground = ship.pos.y;
              bullet.state = Bullet::kFalling;
              bullet.hits = 0;
            }
            /* Switch to moving mode */
            ship.state = Ship::kMoving;
            ship.rot_vel = {0.0, 0.0};
            ship.timer = 0.0;
          }
        }
      }

      if (bullet.is_active) {
        if (bullet.state == Bullet::kFalling || bullet.state == Bullet::kGrounded) {
          /* Draw the ground line that the bullet's gonna hit */
          struct Drawer::Attributes line;
          v2d start = { 0.0, bullet.ground };
          v2d dir = { sdl::kWindowX, 0.0 };
          line.r = 60;
          line.g = 60;
          line.b = 60;
          drawer.Line(start, dir, line);
          /* Spin magnitude is velocity while falling... */
          bullet.spin_magnitude = bullet.vel.Magnitude();
          /* Disable the bullet if it overlaps with the ship */
          bool reunite =
            ship.is_active &&
            bullet.vel.y >= 0.0 &&
            overlap.CircleCircle(10.0, ship.obj.pos, 20.0, bullet.obj.pos);
          if (reunite) {
            bullet.state = Bullet::kIdle;
            bullet.spin_magnitude = bullet.kDefaultSpin;
          }
        }

        if (bullet.state == Bullet::kFalling) {
          /* Bullet kinematics if it's above the "ground line" */
          float kGravity = (bullet.vel.y > 0.0) ? 4e2 : 2e2;
          SemiImplicitEuler(bullet.obj.pos, bullet.vel, { 0, kGravity }, 0.016);
          if (bullet.obj.pos.y > bullet.ground) {
            soul_emitter.initial_speed = bullet.vel.Magnitude();
            bullet.state = Bullet::kGrounded;
            bullet.vel = { 0.0, 0.0 };
          }
          /* Reverse direction on x-bounds */
          if (bullet.obj.pos.x < 0 || bullet.obj.pos.x > sdl::kWindowX)
            bullet.vel.x = -bullet.vel.x;
        }

        if (bullet.state == Bullet::kIdle) {
          bullet.obj.pos = Lerp(bullet.obj.pos, ship.pos, 10.0 * 0.016);
        }

        /* Spin animation */
        const float kRotScale = 0.1;
        bullet.rot = { (float)cos(bullet.theta), (float)sin(bullet.theta) };
        drawer.PointAt(bullet.obj, bullet.rot);
        bullet.theta += 0.016 * bullet.spin_magnitude * kRotScale;

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

          bool hit = false;
          if (bullet.state == Bullet::kFalling)
            hit = overlap.CircleCircle(10.0, enemies[e].obj.pos, 20.0, bullet.obj.pos);

          /* handle collision with bullet */
          if (hit) {
            bullet.hits++;
            soul_emitter.count++;
            soul_emitter.position = enemies[e].obj.pos;
            hitstop_timer = 0.2 * bullet.hits;
          }

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

    /* Let souls animate outside of hitstop (this should be cool) */
    for (size_t s = 0; s < 256; ++s) {
      if (!souls[s].is_active) {
        if (soul_emitter.count <= 0) continue;
        soul_emitter.count--;

        souls[s].timer = 0.0;
        souls[s].obj.pos = soul_emitter.position;
        souls[s].state = Soul::kFollowingBullet;
        souls[s].follow = &bullet.obj;
        souls[s].is_active = true;
        drawer.Register(souls[s].obj);
      } else {
        souls[s].timer += 0.016;

        /** handle various soul states **/
        
        if (souls[s].state == Soul::kFollowingBullet) {
          if (bullet.state == Bullet::kIdle) {
            /* Bullet must have transitioned into Idle b/c it touched ship */
            souls[s].state = Soul::kFollowingShip;
            souls[s].follow = &ship.obj;
          }
          if (bullet.state == Bullet::kGrounded) {
            /* Shoot off this soul in a random direction */
            float radians = Deg2Rad(rand() % 180);
            float speed = soul_emitter.initial_speed;
            v2d rnd = { (float)cos(radians), -(float)sin(radians) };
            souls[s].vel = rnd * speed;
            souls[s].pos = souls[s].obj.pos;
            /* Use kStopTime to figure out what the deacceleration should be */
            float acc = speed / souls[s].kStopTime;
            souls[s].acc = -rnd * acc; 
            souls[s].timer = 0.0;
            souls[s].state = Soul::kBouncing;
            souls[s].follow = nullptr;
          }
        } else if (souls[s].state == Soul::kFollowingEnemy) {
          v2d pos = souls[s].obj.pos;
          if (
            pos.y > sdl::kWindowY ||
            pos.x > sdl::kWindowX ||
            pos.y < 0.0 ||
            pos.x < 0.0
          ) {
            souls[s].is_active = false;
            drawer.Unregister(souls[s].obj);
          }
        } else if (souls[s].state == Soul::kFollowingShip) {
          
        } else if (souls[s].state == Soul::kBouncing) {
          /* Check for edges of screen */
          v2d pos = souls[s].obj.pos;
          if (
            pos.y > sdl::kWindowY ||
            pos.x > sdl::kWindowX ||
            pos.y < 0.0 ||
            pos.x < 0.0
          ) {
            souls[s].vel = -souls[s].vel;
            souls[s].acc = -souls[s].vel.Normalized() * souls[s].acc.Magnitude();
          }
          /* Slow down */
          SemiImplicitEuler(souls[s].pos, souls[s].vel, souls[s].acc, 0.016);
          /* Cut off acceleration if we're nearly stopped */
          if (souls[s].vel.SqrMagnitude() < 2.0) {
            souls[s].state == Soul::kWaiting;
            souls[s].acc = { 0.0, 0.0 };
          }
          /* Apply bounce motion to the y pos */
          const float kPeriodScale = 3.14159 * 2.0 * 1.5;
          const float kHeightScale = souls[s].vel.Magnitude() / 5.0;
          float theta = souls[s].timer / souls[s].kStopTime * kPeriodScale;
          float scale = abs(sin(theta));
          v2d bounce = { 0.0, -kHeightScale };
          souls[s].obj.pos = souls[s].pos + bounce * scale;
        } else if (souls[s].state == Soul::kWaiting) {

        }

        /********************************/

        if (souls[s].follow) {
          /* Rotate elliptically around the follow center */
          v2d follow_pos = souls[s].follow->pos;
          v2d axis_a = { 20.0, -20.0 };
          v2d axis_b = { -10.0, -10.0 };
          float tween_factor = InvTween(souls[s].timer, 1.0);
          float omega = (8.0 - 7.0 * tween_factor) * souls[s].kOmega;
          follow_pos += axis_a * sin(souls[s].timer * omega);
          follow_pos += axis_b * cos(souls[s].timer * omega);
          souls[s].obj.pos = Lerp(souls[s].obj.pos, follow_pos, tween_factor);
        }

        if (!souls[s].follow) {
          /* Check against all the enemies */

        }
      }
    }

    /**********************/

    /* Clear transient state for buttons */
    input.AtFrameEnd();

    /* Draw all the objects */
    drawer.Draw();

    /* Drawer shouldn't clear transient objects (for now, lines) if game in hitstop */
    if (hitstop_timer <= 0.0)
      drawer.ClearTransient();

    /* "Frame time" */
    SDL_Delay(15);
  }

  return 0;
}