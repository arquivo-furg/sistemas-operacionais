#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>

const int N = 64; // Tamanho máximo para o nome do binário

// Variável global para comunicação entre a thread de timeout e o main
pthread_mutex_t mutex;
pthread_cond_t cond;

volatile int timeout_occurred = 0; // indica que o tempo acabou
pid_t pid_filho = 0; // PID do filho para que a thread possa matá-lo

// Estrutura para passar o timeout à thread de monitoramento
struct timeout_info
{
    unsigned int segundos;
};

// Função executada pela thread de monitoramento
void *thread_timeout(void *arg)
{
    struct timeout_info *info = (struct timeout_info *)arg;
    unsigned int seg = info->segundos;

    // Dorme pelo tempo do timeout
    sleep(seg);

    // Ao acordar, sinaliza timeout à thread principal
    pthread_mutex_lock(&mutex);
    timeout_occurred = 1;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    // Mata o processo filho imediatamente
    if (pid_filho > 0)
    {
        kill(pid_filho, SIGKILL);
    }

    pthread_exit(0);
}

int main(int argc, char *argv[])
{
    // Verifica se o usuário passou o nome do binário como argumento
    if (argc < 2)
    {
        printf("Erro, use: %s <nome do binario>\n", argv[0]);
        return 1;
    }

    // Copia o argumento para um buffer local
    char nome[N];
    strncpy(nome, argv[1], N - 1);
    nome[N - 1] = '\0'; // Caractere nulo para finalizar a string

    // Consulta do timeout em tempo de relógio
    unsigned int timeout;
    printf("Informe o timeout em segundos (0 = sem timeout): ");
    scanf("%u", &timeout);

    int pid = fork(); // Cria um novo processo

    // Processos pai e filho executam a linha porém com PIDs diferentes
    printf("PID do processo: <%d>\n", getpid());

    if (pid < 0) // Erro ao criar o processo
    {
        printf("Erro ao criar o processo!\n");
        return 1;
    }
    else if (pid == 0) // Processo filho
    {
        printf("Processo filho iniciado com PID %d\n", getpid());

        // Substitui a imagem do processo pelo binário solicitado
        // O argv[0] é o nome do FMS, o binário receberá os mesmos argumentos
        // É útil se for um programa que usa argc/argv
        execve(nome, argv, 0);

        // Se execve retornar, houve erro
        printf("Erro ao executar o binario '%s'\n", nome);
        return 1;
    }
    else // Processo pai (FMS)
    {
        printf("FMS >> processo filho criado com PID %d\n", pid);
        pid_filho = pid; // Guarda PID do filho para uso na thread

        if (timeout > 0)
        {
            // Com timeout usa thread e mutex/cond para monitorar o processo filho e o tempo
            pthread_t tid;
            struct timeout_info tinfo;
            tinfo.segundos = timeout;

            // Inicializa mutex e condição
            pthread_mutex_init(&mutex, NULL);
            pthread_cond_init(&cond, NULL);

            // Cria a thread de monitoramento
            pthread_create(&tid, NULL, thread_timeout, (void *)&tinfo);

            // Thread principal: espera até que o filho termine ou timeout ocorra
            int status;
            int filho_terminou = 0;

            while (1)
            {
                // Verifica se o filho terminou
                int w = waitpid(pid, &status, WNOHANG);
                if (w == pid)
                {
                    filho_terminou = 1;
                    break;
                }

                // Bloqueia no mutex e verifica timeout
                pthread_mutex_lock(&mutex);

                if (timeout_occurred)
                {
                    pthread_mutex_unlock(&mutex);
                    break;
                }

                // Aguarda um sinal (timeout) ou um breve período para evitar busy wait
                // Usamos tempo limite curto para poder reavaliar waitpid
                struct timespec ts;

                clock_gettime(CLOCK_REALTIME, &ts);

                ts.tv_nsec += 100000000; // 100 ms

                if (ts.tv_nsec >= 1000000000)
                {
                    ts.tv_nsec -= 1000000000;
                    ts.tv_sec += 1;
                }

                pthread_cond_timedwait(&cond, &mutex, &ts);

                pthread_mutex_unlock(&mutex);
            }

            // Se o filho ainda estiver rodando (timeout ocorreu) encerra
            if (timeout_occurred && !filho_terminou)
            {
                kill(pid, SIGKILL);

                waitpid(pid, &status, 0); // recolhe o status

                printf("FMS >> TEMPO ESGOTADO! Processo filho %d foi morto.\n", pid);
            }
            else
            {
                // Cancela a thread de timeout caso o filho tenha terminado antes
                pthread_cancel(tid);

                // Espera a thread encerrar
                pthread_join(tid, NULL);

                // Relata o que aconteceu
                if (WIFEXITED(status))
                    printf("FMS >> processo filho %d finalizou normalmente com código %d.\n", pid, WEXITSTATUS(status));
                else if (WIFSIGNALED(status))
                    printf("FMS >> processo filho %d terminou pelo sinal %d.\n", pid, WTERMSIG(status));
                else
                    printf("FMS >> processo filho %d terminou de forma desconhecida.\n", pid);
            }

            pthread_mutex_destroy(&mutex);
            pthread_cond_destroy(&cond);
        }
        else
        {
            // Sem timeout: espera simples
            int status;

            waitpid(pid, &status, 0); // Aguarda o término do processo filho

            // Relata o que aconteceu
            if (WIFEXITED(status))
                printf("FMS >> processo filho %d finalizou normalmente com código %d.\n", pid, WEXITSTATUS(status));
            else if (WIFSIGNALED(status))
                printf("FMS >> processo filho %d terminou pelo sinal %d.\n", pid, WTERMSIG(status));
            else
                printf("FMS >> processo filho %d terminou de forma desconhecida.\n", pid);
        }
    }

    return 0;
}
