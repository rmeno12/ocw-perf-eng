#ifndef QUADTREE_H_
#define QUADTREE_H_

#include <stddef.h>

#include "line.h"
#include "linepg.h"
#include "vec.h"

// We want a quadtree node to start putting things into its children once it has
// 4 elements, so that we have enough data to be worth using the children. Can
// look into tuning this value to help performance.
#define QT_SOFT_CAPACITY 4

// Axis aligned bounding box
struct AABB {
  Vec center;
  Vec half_dim;
};
typedef struct AABB AABB;

// Returns true if the LinePg is entirely contained within the bounding box
// defined by the AABB
bool AABB_contains(const AABB* const aabb, const LinePg* const pg);

struct QuadTree {
  AABB boundary;

  LinePgList contained;
  size_t contained_sz;

  struct QuadTree* nw;
  struct QuadTree* ne;
  struct QuadTree* sw;
  struct QuadTree* se;
};
typedef struct QuadTree QuadTree;

QuadTree* QuadTree_init(AABB boundary);
void QuadTree_free(QuadTree* qt);
bool QuadTree_isleaf(const QuadTree* const qt);
bool QuadTree_insert(QuadTree* const qt, const LinePg* const pg);
const QuadTree* QuadTree_query(const QuadTree* const qt,
                               const LinePg* const pg);
bool QuadTree_remove(QuadTree* const qt, const LinePg* const pg);

#endif  // QUADTREE_H_
