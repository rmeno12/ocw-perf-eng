#ifndef LINEPG_H_
#define LINEPG_H_

#include "line.h"

// Parallelogram formed by line at current time and line at the next time step
struct LinePg {
  // This LinePg owns the next line but not the now line, which belongs to the
  // CollisionWorld.
  Line next;
  Line* now;
};
typedef struct LinePg LinePg;

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

#endif
