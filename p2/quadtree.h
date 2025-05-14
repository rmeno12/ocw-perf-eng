#ifndef QUADTREE_H_
#define QUADTREE_H_

#include <stddef.h>

#include "line.h"
#include "vec.h"

// We want a quadtree node to start putting things into its children once it has
// 4 elements, so that we have enough data to be worth using the children. Can
// look into tuning this value to help performance.
#define QT_SOFT_CAPACITY 4

// Parallelogram formed by line at current time and line at the next time step
struct LinePg {
  // This LinePg owns the next line but not the now line, which belongs to the
  // CollisionWorld.
  Line next;
  Line* now;
};
typedef struct LinePg LinePg;

// Axis aligned bounding box
struct AABB {
  Vec center;
  Vec half_dim;
};
typedef struct AABB AABB;

// Returns true if the LinePg is entirely contained within the bounding box
// defined by the AABB
bool AABB_contains(const AABB* const aabb, const LinePg* const pg);

struct LinePgListNode {
  // The list owns the pg
  LinePg* pg;
  struct LinePgListNode* next;
};
typedef struct LinePgListNode LinePgListNode;

const LinePgListNode* LinePgListNode_contains(const LinePgListNode* const list,
                                              const LinePg* const pg);
void LinePgListNode_free(LinePgListNode* list);

struct LinePgList {
  LinePgListNode* head;
  LinePgListNode* tail;
};
typedef struct LinePgList LinePgList;

void LinePgList_append(LinePgList* list, LinePg* pg);
void LinePgList_droplast(LinePgList* list, size_t n);
bool LinePgList_contains(const LinePgList* const list, const LinePg* const pg);
void LinePgList_free(LinePgList* list);

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
