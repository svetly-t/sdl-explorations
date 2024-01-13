#ifndef OBJECT_H
#define OBJECT_H

#include "vector.h"

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

#endif