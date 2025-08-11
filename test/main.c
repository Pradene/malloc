#include "malloc.h"

int main() {
  show_alloc_mem();

  void *p1 = ft_malloc(2048);
  void *p2 = ft_malloc(2048);
  void *p3 = ft_malloc(2048);
  void *p4 = ft_malloc(2000);
  void *p5 = ft_malloc(80);
  void *p6 = ft_malloc(104);
  void *p7 = ft_malloc(79);

  show_alloc_mem();

  ft_free(p1);
  ft_free(p2);
  ft_free(p3);
  ft_free(p4);
  ft_free(p5);
  ft_free(p6);
  ft_free(p7);

  return (0);
}
