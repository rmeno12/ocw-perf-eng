#include "linepg.h"

#include <assert.h>
#include <stdlib.h>

const LinePgListNode* LinePgListNode_contains(const LinePgListNode* const list,
                                              const LinePg* const pg) {
  if (list == NULL) {
    return NULL;
  }

  if (list->pg == pg) {
    return list;
  }

  return LinePgListNode_contains(list->next, pg);
}

void LinePgListNode_free(LinePgListNode* node) {
  if (node == NULL) return;

  free(node->pg);
  LinePgListNode* next = node->next;
  free(node);
  // TODO: maybe do this iteratively and not recursively, see if it's slow first
  LinePgListNode_free(next);
}

void LinePgList_append(LinePgList* list, LinePg* pg) {
  LinePgListNode* node = malloc(sizeof(LinePgListNode));
  node->pg = pg;
  node->next = NULL;
  if (list->head == NULL) {
    assert(list->tail == NULL);
    list->head = node;
    list->tail = node;
  } else {
    assert(list->tail != NULL);
    list->tail->next = node;
    list->tail = node;
  }
}

static size_t LinePgList_len(LinePgList* list) {
  size_t len = 0;
  if (list == NULL) return len;
  LinePgListNode* n = list->head;
  if (n == NULL) return len;

  len++;
  while (n != NULL) {
    n = n->next;
    len++;
  }

  return len;
}

void LinePgList_droplast(LinePgList* list, size_t n) {
  if (list == NULL || n == 0) return;
  LinePgListNode* head = list->head;
  if (head == NULL) return;
#ifndef NDEBUG
  size_t start_len = LinePgList_len(list);
#endif

  LinePgListNode* fast = head;
  for (int i = 0; i < n; i++) {
    if (fast == NULL) break;
    fast = fast->next;
  }
  if (fast == NULL) {
    // <= n elements in the list, free the whole thing
    LinePgListNode_free(list->head);
    list->head = NULL;
    list->tail = NULL;
    return;
  }

  LinePgListNode* slow = head;
  while (fast->next != NULL) {
    fast = fast->next;
    slow = slow->next;
  }
  // slow is now at the start of the last n elements
  LinePgListNode_free(slow->next);
  slow->next = NULL;
  list->tail = slow;
#ifndef NDEBUG
  size_t end_len = LinePgList_len(list);
  assert(end_len + n == start_len);
#endif
}

bool LinePgList_contains(const LinePgList* const list, const LinePg* const pg) {
  if (list == NULL || list->head == NULL) return false;
  return LinePgListNode_contains(list->head, pg) != NULL;
}

void LinePgList_free(LinePgList* list) {
  if (list == NULL) return;
  if (list->head == NULL) {
    assert(list->tail == NULL);
    free(list);
    return;
  }
  LinePgListNode_free(list->head);
  free(list);
  return;
}
