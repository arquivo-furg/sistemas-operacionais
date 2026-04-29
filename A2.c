#define TRUE 1
2
3 while (TRUE) { /* repita para sempre */
4 type_prompt( ); /* mostra prompt na tela */
5 read_command(command, parameters); /* le entrada do terminal */
6
7 if (fork( ) != 0) { /* cria processo filho */
8 /* Codigo do processo pai. */
9 waitpid(-1, &status, 0); /* aguarda o processo filho
acabar */
10 } else {
11 /* Codigo do processo filho. */
12 execve(command, parameters, 0); /* executa o comando */
13 }
14 }