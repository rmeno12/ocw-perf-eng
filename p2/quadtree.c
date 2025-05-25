#include "quadtree.h"

#include <assert.h>
#include <stdlib.h>

#include "vec.h"

static inline bool AABB_contains_point(const AABB* const aabb, const Vec v) {
  const vec_dimension left = aabb->center.x - aabb->half_dim.x;
  const vec_dimension right = aabb->center.x + aabb->half_dim.x;
  const vec_dimension bottom = aabb->center.y - aabb->half_dim.y;
  const vec_dimension top = aabb->center.y + aabb->half_dim.y;

  // TODO: see if this short circuiting is good or bad
  return v.x > left && v.x < right && v.y > bottom && v.y < top;
}

bool AABB_contains(const AABB* const aabb, const LinePg* const pg) {
  Line now = *pg->now;
  bool nowin1 = AABB_contains_point(aabb, now.p1);
  bool nowin2 = AABB_contains_point(aabb, now.p2);

  Line next = pg->next;
  bool nextin1 = AABB_contains_point(aabb, next.p1);
  bool nextin2 = AABB_contains_point(aabb, next.p2);

  // TODO: see if this not short circuiting is good or bad
  return nowin1 && nowin2 && nextin1 && nextin2;
}

QuadTree* QuadTree_init(AABB boundary) {
  QuadTree* qt = malloc(sizeof(QuadTree));
  *qt = (QuadTree){
      .boundary = boundary,
      .contained = {.head = NULL, .tail = NULL},
      .contained_sz = 0,
      .nw = NULL,
      .ne = NULL,
      .sw = NULL,
      .se = NULL,
  };
  return qt;
}

void QuadTree_free(QuadTree* qt) {
  if (qt == NULL) return;

  if (qt->contained.head != NULL) {
    assert(qt->contained.tail != NULL);
    LinePgListNode_free(qt->contained.head);
  } else {
    assert(qt->contained.tail == NULL);
  }

  QuadTree_free(qt->nw);
  QuadTree_free(qt->ne);
  QuadTree_free(qt->sw);
  QuadTree_free(qt->se);

  free(qt);
}

bool QuadTree_isleaf(const QuadTree* const qt) {
  if (qt == NULL) return false;
  if (qt->nw == NULL) {
    assert(qt->ne == NULL);
    assert(qt->sw == NULL);
    assert(qt->se == NULL);
    return true;
  }
  return false;
}

static bool QuadTree_haslocal(const QuadTree* const qt) {
  if (qt == NULL) return false;
  if (qt->contained_sz == 0) {
    assert(qt->contained.head == NULL);
    assert(qt->contained.tail == NULL);
    return false;
  } else {
    assert(qt->contained.head != NULL);
    assert(qt->contained.tail != NULL);
    return true;
  }
}

static void QuadTree_subdivide(QuadTree* const qt) {
  assert(QuadTree_isleaf(qt));

  AABB boundary = qt->boundary;
  Vec new_half = Vec_multiply(boundary.half_dim, 0.5);

  AABB nw_boundary = {
      .center =
          Vec_add(boundary.center, (Vec){.x = -new_half.x, .y = new_half.y}),
      .half_dim = new_half,
  };
  qt->nw = QuadTree_init(nw_boundary);

  AABB ne_boundary = {
      .center =
          Vec_add(boundary.center, (Vec){.x = new_half.x, .y = new_half.y}),
      .half_dim = new_half,
  };
  qt->ne = QuadTree_init(ne_boundary);

  AABB sw_boundary = {
      .center =
          Vec_add(boundary.center, (Vec){.x = -new_half.x, .y = -new_half.y}),
      .half_dim = new_half,
  };
  qt->sw = QuadTree_init(sw_boundary);

  AABB se_boundary = {
      .center =
          Vec_add(boundary.center, (Vec){.x = new_half.x, .y = -new_half.y}),
      .half_dim = new_half,
  };
  qt->se = QuadTree_init(se_boundary);

  // TODO: attempt to insert contained nodes into children instead?
}

static void QuadTree_add(QuadTree* const qt, const LinePg* const pg) {
  LinePgList_append(&qt->contained, pg);
  qt->contained_sz++;
}

bool QuadTree_insert(QuadTree* const qt, const LinePg* const pg) {
  // If the pg isn't in the bounds of this qt node, we can't insert it here.
  if (!AABB_contains(&qt->boundary, pg)) {
    return false;
  }

  if (QuadTree_isleaf(qt)) {
    if (qt->contained_sz < QT_SOFT_CAPACITY) {
      // Store locally if we're below the soft limit on items stored in this
      // node.
      QuadTree_add(qt, pg);
      return true;
    } else {
      // Otherwise, create children for this node to add into.
      QuadTree_subdivide(qt);
    }
  }

  // Try to insert into one of the child nodes
  if (QuadTree_insert(qt->nw, pg)) return true;
  if (QuadTree_insert(qt->ne, pg)) return true;
  if (QuadTree_insert(qt->sw, pg)) return true;
  if (QuadTree_insert(qt->se, pg)) return true;

  // If we couldn't insert into any of the child nodes, we just store here.
  QuadTree_add(qt, pg);
  return true;
}

// TODO: idea: instead of doing this search, add a link back from a linepg to
// the quadtree node parent?
const QuadTree* QuadTree_query(const QuadTree* const qt,
                               const LinePg* const pg) {
  if (!AABB_contains(&qt->boundary, pg)) {
    return NULL;
  }

  if (!QuadTree_isleaf(qt)) {
    if (QuadTree_query(qt->nw, pg)) return qt->nw;
    if (QuadTree_query(qt->ne, pg)) return qt->ne;
    if (QuadTree_query(qt->sw, pg)) return qt->sw;
    if (QuadTree_query(qt->se, pg)) return qt->se;
  }

  if (qt->contained_sz > 0 && LinePgList_contains(&qt->contained, pg)) {
    assert(QuadTree_haslocal(qt));
    return qt;
  }

  return NULL;
}
