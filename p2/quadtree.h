#ifndef QUADTREE_H_
#define QUADTREE_H_

#include <stddef.h>

#include "line.h"
#include "vec.h"

#define QT_SOFT_CAPACITY 1

// Parallelogram formed by line at current time and line at the next time step
struct LinePg {
  Line now;
  Line next;
};
typedef struct LinePg LinePg;

// Axis aligned bounding box
struct AABB {
  Vec center;
  vec_dimension half_dim;
};
typedef struct AABB AABB;

// Returns true if the LinePg is entirely contained within the bounding box
// defined by the AABB
bool AABB_contains(const AABB* const aabb, const LinePg* const pg);

struct LinePgListNode {
  // The list doesn't own the pg
  const LinePg* pg;
  struct LinePgListNode* next;
};
typedef struct LinePgListNode LinePgListNode;

struct QuadTree {
  AABB boundary;

  LinePgListNode* contained;
  size_t contained_sz;

  struct QuadTree* nw;
  struct QuadTree* ne;
  struct QuadTree* sw;
  struct QuadTree* se;
};
typedef struct QuadTree QuadTree;

QuadTree* QuadTree_init(AABB boundary);
bool QuadTree_insert(QuadTree* const qt, const LinePg* const pg);
bool QuadTree_remove(QuadTree* const qt, const LinePg* const pg);

#endif  // QUADTREE_H_
