#include <string.h>

typedef struct {
  int capacidade;
  void** item;
  long item_tamanho;
  int esta_cheia, esta_vazia;
  int head, tail;
} fila;

fila *inicializar_fila (unsigned tamanho, long item_size)
{
    fila *q = (fila *)malloc(sizeof (fila));
    q->item = malloc(sizeof(item_size) * tamanho);

    q->capacidade = tamanho;
    q->item_tamanho = item_size;
    q->esta_vazia = 1;
    q->esta_cheia = 0;
    q->head = 0;
    q->tail = 0;
  
    return q;
}

void fila_push(fila *fila, void *item) 
{
    fila->item[fila->tail] = item;

    fila->tail++;
    if (fila->tail == fila->capacidade) {
      fila->tail = 0;
    }
    
    if (fila->tail == fila->head) {
      fila->esta_cheia = 1;
    }
    fila->esta_vazia = 0;
}

void fila_pop(fila *q, void *item) 
{
    memcpy(item, q->item[q->head], q->item_tamanho);
    q->head++;

    if (q->head == q->capacidade) {
      q->head = 0;
    }
    if (q->head == q->tail) {
      q->esta_vazia = 1;
    }

    q->esta_cheia = 0;
}