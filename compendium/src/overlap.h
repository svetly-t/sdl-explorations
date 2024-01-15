#ifndef OVERLAP_H
#define OVERLAP_H

#include <list>
#include <vector>
#include <unordered_map>

#include "object.h"
#include "vector.h"

/* 
 * Reduce the number of collider-collider checks we need
 * to do by recursively partitioning space into fourths.
 */
// template <typename CONTAINER>
// class Quadtree {
//  public:
//   Quadtree(unsigned level, struct rect bounds) {
//     bounds_ = bounds;
//     level_ = level;
//   }

//   ~Quadtree() {
//     Clear();
//   }
  
//   void Clear() {
//     objs_.clear();
//     /* Recursively clear child nodes */
//     for (Quadtree<CONTAINER> &node : nodes_)
//       node.Clear();
//   }

//   void Split() {

//   }

//   void Insert();

//   CONTAINER Retrieve();
//  private:
//   /*
//    * Get the index, from 0-3, of the subtree that this query fits in.
//    * If the query fits in multiple subnodes, return -1 to mean "parent"
//    */
//   int GetIndex(rect query) {
//     int idx = -1;
    
//   }

//   static const unsigned kMaxObjects_ = 16;
//   static const unsigned kMaxLevels_ = 4;

//   struct rect bounds_;

//   unsigned level_;
//   std::vector<Quadtree<CONTAINER>> nodes_;

//   CONTAINER objs_;
// }; // class Quadtree

class Collision {
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
    /* collision layer mask a la unity */
    unsigned mask;
    /* Data to be returned when someone overlaps with this object */
    void *data;
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
    return false;
  }
  Attributes *CheckAgainst(const Object &query, unsigned mask) {
    for (auto &[_, attr] : map_) {
      /* Skip objects whose layermask doesn't match the query */
      if ((attr.mask & mask) == 0)
        continue;
      if (Check(query, *attr.obj))
        return &attr;
    }
    return nullptr;
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
}; // class Collision

#endif