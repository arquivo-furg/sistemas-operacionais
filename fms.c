#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>

const int N = 64; // Tamanho máximo para o nome do binário

// Variável global para comunicação entre o tratador do alarme e o main
volatile sig_atomic_t timeout_occurred = 0;
pid_t pid_filho = 0; // PID do filho para que o tratador possa matá-lo

// Tratador do sinal SIGALRM (disparado pelo alarme)
void tratador_alarme(int sig)
{
    timeout_occurred = 1;
    
    if (pid_filho > 0)
    {
        // Mata o processo filho imediatamente
        kill(pid_filho, SIGKILL);
    }
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
        pid_filho = pid; // Guarda PID do filho para uso no tratador

        // Configura o tratador para SIGALRM
        struct sigaction sa;
        sa.sa_handler = tratador_alarme;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGALRM, &sa, NULL);

        // Programa o alarme se timeout > 0
        if (timeout > 0)
        {
            alarm(timeout);
        }

        int status;
        waitpid(pid, &status, 0); // Aguarda o término do processo filho

        // Desliga o alarme caso o filho tenha terminado antes
        alarm(0);

        // Relata o que aconteceu
        if (timeout_occurred) // O filho foi morto por timeout
        {
            printf("FMS >> TEMPO ESGOTADO! Processo filho %d foi morto.\n", pid);
        }
        else if (WIFEXITED(status)) // O filho terminou normalmente
        {
            printf("FMS >> processo filho %d finalizou normalmente com código %d.\n", pid, WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status)) // O filho foi morto por um sinal (mas não necessariamente o alarme)
        {
            printf("FMS >> processo filho %d terminou pelo sinal %d.\n", pid, WTERMSIG(status));
        }
        else // Outro tipo de término
        {
            printf("FMS >> processo filho %d terminou de forma desconhecida.\n", pid);
        }
    }

    return 0;
}
