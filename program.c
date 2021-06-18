#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <err.h>
#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <pthread.h>

#include "mandelbrot.c"
#include "fila.c"
#include "x11-helpers.c"

static fila *buffer_tarefas;
static fila *buffer_display;

static pthread_mutex_t *mutex;
static pthread_mutex_t *mutex_display;
static pthread_mutex_t *mutex_fila_result;

static pthread_cond_t *condicaoMtoLoca;

void * thread_callback(void * args)
{
    while (1) 
    {
        pthread_mutex_lock(mutex);

        if (buffer_tarefas->esta_vazia) {
            pthread_mutex_unlock(mutex);
            break;
        }

        ArgsCalculo *args = malloc(sizeof(ArgsCalculo));
        fila_pop(buffer_tarefas, args);
        pthread_mutex_unlock(mutex);

        result_data *result = malloc(sizeof(result_data));
        result->xi = args->xInicial;
        result->xf = args->xFinal;
        result->yi = 0;
        result->yf = 1000;

        display_double(args);

        pthread_mutex_lock(mutex_fila_result);
        fila_push(buffer_display, result);
        pthread_mutex_unlock(mutex_fila_result);
        pthread_cond_signal(condicaoMtoLoca);
    }
}

static void * consumer(void *data) 
{
    int quantidadeTarefas = 50;
    int tarefasRealizas = 0;

    while (1) 
    {
        if (tarefasRealizas == quantidadeTarefas) 
        {
            XFlush(display);
            return NULL;
        }

        pthread_mutex_lock(mutex_fila_result);
        if (buffer_display->esta_vazia) {            
            pthread_cond_wait(condicaoMtoLoca, mutex_fila_result);
            pthread_mutex_unlock(mutex_fila_result);
            continue;
        }

        result_data *result = malloc(sizeof(result_data));        
        fila_pop(buffer_display, result);
        result->yf = 1000;

        XPutImage(
            display, window, gc, imagem,
            result->xi, result->yi, result->xi, result->yi, (result->xf - result->xi + 1), (result->yf - result->yi + 1)
        );

        pthread_mutex_unlock(mutex_fila_result);
        
        tarefasRealizas++;
        free(result);
    }
}

int main(void)
{
    int tamanhoImagem = 1000;

    inicializar_x11(tamanhoImagem);

    inicializar_cores();

    buffer_display = inicializar_fila(100000, sizeof(ArgsDisplay));

    mutex = (pthread_mutex_t *) malloc(sizeof (pthread_mutex_t));
    mutex_display = (pthread_mutex_t *) malloc(sizeof (pthread_mutex_t));
    mutex_fila_result = (pthread_mutex_t *) malloc(sizeof (pthread_mutex_t));

    condicaoMtoLoca = (pthread_cond_t *) malloc(sizeof (pthread_cond_t));
    pthread_cond_init(condicaoMtoLoca, NULL);

    pthread_mutex_init(mutex, NULL);

    int quantidadeTarefas = 50;
    buffer_tarefas = inicializar_fila(quantidadeTarefas, sizeof(ArgsCalculo));

    int quantidadePartes = quantidadeTarefas;
    int tamanhoPorParte = tamanhoImagem / quantidadePartes;

    struct timeval tempoInicioExecucao, tempoFimExecucao;
    gettimeofday(&tempoInicioExecucao, 0);

    for (int i = 0; i < quantidadePartes; i++) 
    {
        struct ArgsCalculo *threadArgs = (struct ArgsCalculo *)malloc(sizeof(struct ArgsCalculo));
        
        int inicio = i * tamanhoPorParte;
        int fim = inicio + tamanhoPorParte;

        threadArgs->xInicial = inicio;
        threadArgs->xFinal = fim;
        threadArgs->tamanhoDivisao = quantidadePartes;
        threadArgs->imagem = imagem;
        threadArgs->tamanhoImagem = tamanhoImagem;

        fila_push(buffer_tarefas, threadArgs);
    }

    int qtdThreads = 10;
    pthread_t idsThreads[qtdThreads];
    for (int i = 0; i < qtdThreads; i++) {
        pthread_t idThread;
        pthread_create(&idThread, NULL, thread_callback, NULL);
        idsThreads[i] = idThread;

        printf("Thread %d criada\n", (int)idThread);        
    }
    printf("\n");
    
    pthread_t consumerThreadId;
    pthread_create(&consumerThreadId, NULL, consumer, NULL);

    for (int i = 0; i < qtdThreads; i++) {
        pthread_join(idsThreads[i], NULL);
        printf("\t\n*** Fim da thread %d***\n", (int)idsThreads[i]);
    }
    printf("\n");
    
    pthread_join(consumerThreadId, NULL);

    gettimeofday(&tempoFimExecucao, 0);
    long segundos = tempoFimExecucao.tv_sec - tempoInicioExecucao.tv_sec;
    long milissegundos = tempoFimExecucao.tv_usec - tempoInicioExecucao.tv_usec;
    double tempoDecorrido = segundos + milissegundos*1e-6;
    printf("\nLevou %f segundos para calcular\n", tempoDecorrido);

    loop_interface_grafica(tamanhoImagem);
    finalizar_x11();

    return 0;
}