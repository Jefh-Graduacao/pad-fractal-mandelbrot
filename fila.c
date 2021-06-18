#include <string.h>

typedef struct {
  int size;
  void** data;
  long item_size;
  int head, tail;
  int is_full, is_empty;
} fila;

fila *inicializar_fila (unsigned tamanho, long item_size) {
  fila *q = (fila *)malloc(sizeof (fila));
  q->data = malloc(sizeof(item_size) * tamanho);

  q->size = tamanho;
  q->item_size = item_size;
  q->is_empty = 1;
  q->is_full = 0;
  q->head = 0;
  q->tail = 0;
  
  return q;
}

void fila_push(fila *fila, void *item) {
  fila->data[fila->tail] = item;
  fila->tail++;
  if (fila->tail == fila->size) {
    fila->tail = 0;
  }
  if (fila->tail == fila->head) {
    fila->is_full = 1;
  }
  fila->is_empty = 0;
}

void fila_pop(fila *q, void *item) {
  memcpy(item, q->data[q->head], q->item_size);
  q->head++;
  if (q->head == q->size) {
    q->head = 0;
  }
  if (q->head == q->tail) {
    q->is_empty = 1;
  }
  q->is_full = 0;
}