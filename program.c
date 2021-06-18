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

static pthread_mutex_t *mutex_buffer_tarefas;
static pthread_mutex_t *mutex_buffer_display;

static pthread_cond_t *condicao_buffer_display;

void * thread_callback(void * args)
{
    while (1) 
    {
        pthread_mutex_lock(mutex_buffer_tarefas);

        if (buffer_tarefas->esta_vazia) {
            pthread_mutex_unlock(mutex_buffer_tarefas);
            break;
        }

        ArgsCalculo *args = malloc(sizeof(ArgsCalculo));
        fila_pop(buffer_tarefas, args);
        pthread_mutex_unlock(mutex_buffer_tarefas);

        result_data *result = malloc(sizeof(result_data));
        result->xi = args->xInicial;
        result->xf = args->xFinal;
        result->yi = 0;
        result->yf = 1000;

        display_double(args);

        pthread_mutex_lock(mutex_buffer_display);
        fila_push(buffer_display, result);
        pthread_mutex_unlock(mutex_buffer_display);
        pthread_cond_signal(condicao_buffer_display);
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

        pthread_mutex_lock(mutex_buffer_display);
        if (buffer_display->esta_vazia) {            
            pthread_cond_wait(condicao_buffer_display, mutex_buffer_display);
            pthread_mutex_unlock(mutex_buffer_display);
            continue;
        }

        result_data *result = malloc(sizeof(result_data));        
        fila_pop(buffer_display, result);
        result->yf = 1000;

        adicionar_imagem_x11(
            result->xi, result->yi, result->xi, result->yi, (result->xf - result->xi + 1), (result->yf - result->yi + 1)
        );

        pthread_mutex_unlock(mutex_buffer_display);
        
        tarefasRealizas++;
        free(result);
    }
}

int main(void)
{
    int tamanhoImagem = 1000;
    int quantidadeTarefas = 50;
    int qtdThreads = 100;

    inicializar_x11(tamanhoImagem);

    inicializar_cores();

    buffer_tarefas = inicializar_fila(quantidadeTarefas, sizeof(ArgsCalculo));
    buffer_display = inicializar_fila(quantidadeTarefas, sizeof(ArgsDisplay));

    mutex_buffer_tarefas = (pthread_mutex_t *) malloc(sizeof (pthread_mutex_t));
    mutex_buffer_display = (pthread_mutex_t *) malloc(sizeof (pthread_mutex_t));

    condicao_buffer_display = (pthread_cond_t *) malloc(sizeof (pthread_cond_t));
    pthread_cond_init(condicao_buffer_display, NULL);

    pthread_mutex_init(mutex_buffer_tarefas, NULL);

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