#include "malloc.h"

int main() {
  struct rlimit limit = {0};
  getrlimit(RLIMIT_AS, &limit);

  void *p1 = malloc(2048);
  void *p2 = malloc(2048);
  void *p3 = malloc(2048);
  void *p4 = malloc(2000);
  void *p5 = malloc(80);
  void *p6 = malloc(104);

  show_alloc_mem();

  free(p1);
  free(p2);
  free(p3);
  free(p4);
  free(p5);
  free(p6);

  Zone *zone = NULL;
  while (base) {
    zone = base->next;
    munmap(base, base->size);
    base = zone;
  }
  return (0);
}
