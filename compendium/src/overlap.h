#ifndef OVERLAP_H
#define OVERLAP_H

#include <unordered_map>

#include "object.h"
#include "vector.h"

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

#endif