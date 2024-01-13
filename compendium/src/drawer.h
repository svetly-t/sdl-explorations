#ifndef DRAWER_H
#define DRAWER_H

#include <iostream>
#include <vector>
#include <unordered_map>

#include "input.h"
#include "object.h"
#include "sdl.h"

class Drawer {
 public:
  struct Texture {
    SDL_Texture *ptr;
  };

  struct Sprite {
    struct Texture texture;
    v2d off;
    float w;
    float h;
    float theta;
  };
  void SetSprite(Object &o, std::string filename) {
    size_t hash = std::hash<std::string>{}(filename);

    if (textures_.find(hash) == textures_.end())
      LoadTexture(filename, hash);
    if (map_.find(o.key) == map_.end()) return;
    
    map_[o.key].type = Attributes::kSprite;
    map_[o.key].sprite.texture = textures_[hash];
  }
  Sprite *GetSprite(Object &o) {
    if (map_.find(o.key) == map_.end()) return nullptr;
    return &map_[o.key].sprite;
  }

  struct Attributes {
    bool enabled = true;
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    float size = 20;
    enum Type {
      kPrimitive,
      kSprite
    } type;
    struct Sprite sprite;
    /**
     * Use a vector for rotation when drawing lines
     */
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
  void ClearTransient() {
    lines_.clear();
  }
  void Draw() {
    sdl::StartDraw();
    for (const LineAttributes &l : lines_) {
      sdl::SetColor(l.attr.r, l.attr.g, l.attr.b);
      sdl::DrawLine(l.pos, l.vec, l.nub);
    }
    for (const auto &[_, attr] : map_) {
      if (!attr.enabled) continue;
      if (attr.type == Attributes::kPrimitive) {
        sdl::SetColor(attr.r, attr.g, attr.b);
        sdl::DrawRect(attr.obj->pos, attr.size, attr.point_at);
      }
      if (attr.type == Attributes::kSprite) {
        sdl::SetColor(attr.r, attr.g, attr.b);
        sdl::DrawTexture(attr.sprite.texture.ptr, attr.obj->pos, attr.sprite.off, attr.sprite.h, attr.sprite.w, attr.sprite.theta);
      }
    }
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
  
  struct LineAttributes {
    v2d pos;
    v2d vec;
    bool nub;
    struct Attributes attr;
  };
  std::vector<struct LineAttributes> lines_;

  std::unordered_map<size_t, struct Texture> textures_;
  void LoadTexture(std::string filename, size_t hash) {
    struct Texture nt;
    nt.ptr = sdl::LoadTexture(filename);
    if (!nt.ptr) {
      /* On error: Use a default sprite? */
    }
    textures_[hash] = nt;
  }
}; // class Drawer

#endif