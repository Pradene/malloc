#include "malloc.h"

int main() {
  struct rlimit limit = {0};
  getrlimit(RLIMIT_AS, &limit);
  if (DEBUG) {
    printf("Current %zu\n", limit.rlim_cur);
    printf("Max %zu\n", limit.rlim_max);
  }

  void *p1 = ft_malloc(2048);
  void *p2 = ft_malloc(2048);
  void *p3 = ft_malloc(2048);
  void *p4 = ft_malloc(2048);
  void *p5 = ft_malloc(64);
  void *p6 = ft_malloc(8192);

  print_mem();

  ft_free(p1);
  ft_free(p2);
  ft_free(p3);
  ft_free(p4);
  ft_free(p5);
  ft_free(p6);

  Page *page = NULL;
  while (base) {
    page = base->next;
    munmap(base, base->size);
    base = page;
  }
  return (0);
}
