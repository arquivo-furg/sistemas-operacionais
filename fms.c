#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

const int N = 64; // Tamanho máximo para o nome do binário

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

        waitpid(pid, NULL, 0); // Aguarda o término do processo filho

        printf("FMS >> processo filho %d finalizado.\n", pid);
    }

    return 0;
}
