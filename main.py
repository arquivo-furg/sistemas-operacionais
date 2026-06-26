import sys
import io
import time
import zstandard as zstd


def main():
    # TODO: Remove hardcoded example file path

    # if len(sys.argv) != 2:
    #     print(f"Uso: {sys.argv[0]} arquivo.zst")
    #     sys.exit(1)

    arquivo = "data/acessos-A0.txt.zst"  # sys.argv[1]

    # Solicita ao usuário o tamanho da memória e da página e converte para bytes
    print("Formato: TAMANHO [B|KB|MB|GB]. Exemplo: 1024 MB.")
    tam_memoria = get_tamanho_bytes("Informe o tamanho da memória: ")
    tam_pagina = get_tamanho_bytes("Informe o tamanho da página: ")

    # Garante que o tamanho da página seja menos ou igual ao tamanho da memória
    while tam_pagina > tam_memoria:
        print("O tamanho da página deve ser menor ou igual ao tamanho da memória.")
        tam_pagina = get_tamanho_bytes("Informe o tamanho da página: ")

    # Calcula o número de páginas que cabem na memória
    num_paginas = tam_memoria // tam_pagina
    print(f"Limite de páginas  : {num_paginas}")

    # Mapeia os endereços de acesso para números de página
    acesso_paginas, total_paginas = get_acesso_paginas(arquivo, tam_pagina)

    faltas, tempo = FIFO(acesso_paginas)
    mostrar_resultado("FIFO", faltas, tempo, total_paginas)

    faltas, tempo = OPT(acesso_paginas)
    mostrar_resultado("OPT", faltas, tempo, total_paginas)


def get_tamanho_bytes(message):
    unidades = {"B": 0, "KB": 1, "MB": 2, "GB": 3, "TB": 4}

    while True:
        try:
            # Obtém o tamanho e a unidade do usuário, separados por espaço
            tamanho, unidade = input(message).split(" ")

            # Busca a potência correspondente à unidade fornecida pelo usuário
            potencia = unidades.get(unidade.upper(), -1)

            # Se a unidade não for válida, raise ValueError
            if potencia == -1:
                raise ValueError

            # Converte o tamanho para bytes usando a potência correspondente
            tamanho_bytes = int(tamanho) * 1024**potencia

            return tamanho_bytes
        except KeyboardInterrupt:
            # Interrompe o programa se o usuário pressionar Ctrl+C
            sys.exit(2)
        except:
            # Trata qualquer outro erro (como ValueError) e solicita novamente a entrada do usuário
            print("Informe o valor conforme o formato especificado.")


def get_acesso_paginas(arquivo, tam_pagina):
    enderecos = 0
    unicos = {}
    acesso_paginas = []
    unicas = {}

    with open(arquivo, "rb") as fh:
        print(f"Arquivo            : {arquivo}")
        dctx = zstd.ZstdDecompressor(max_window_size=2147483648)
        with dctx.stream_reader(fh) as reader:
            text_stream = io.TextIOWrapper(reader, encoding="utf-8")
            for linha in text_stream:
                # Convete a linha de endereço hexadecimal para os bytes correspondentes
                endereco = int(linha.strip(), 16)

                # Calcula o número da página a partir do endereço
                num_pagina = endereco // tam_pagina

                # Adiciona o número da página à lista de acessos
                acesso_paginas.append(num_pagina)

                # Incrementa o contador de endereços processados
                enderecos += 1

                # Conta o número de acessos únicos por endereço e por página
                unicos[endereco] = unicos.get(linha, 0) + 1
                unicas[num_pagina] = unicas.get(num_pagina, 0) + 1

    # Calcula o total de páginas acessadas
    total_paginas = len(acesso_paginas)

    print(f"Total de endereços : {enderecos}")
    print(f"Endereços únicos   : {len(unicos)}")
    print(f"Total de páginas   : {total_paginas}")
    print(f"Páginas únicas     : {len(unicas)}")

    return acesso_paginas, total_paginas


def FIFO(acesso_paginas, num_paginas):
    inicio = time.perf_counter()

    paginas_mem = []
    faltas = 0

    for pagina in acesso_paginas:
        # Se a página já estiver na memória, não faz nada
        if pagina in paginas_mem:
            continue

        # Se a memória estiver cheia, remove a página mais antiga (FIFO)
        if len(paginas_mem) >= num_paginas:
            paginas_mem.pop(0)

        # Adiciona a nova página à memória
        paginas_mem.append(pagina)

        # Incrementa uma falta de página uma vez que ela foi buscada na memória
        faltas += 1

    fim = time.perf_counter()
    tempo = fim - inicio

    return faltas, tempo


def OPT(acesso_paginas, num_paginas):
    inicio = time.perf_counter()

    paginas_mem = []
    faltas = 0

    for i, pagina in enumerate(acesso_paginas):
        # Se a página já estiver na memória, não faz nada
        if pagina in paginas_mem:
            continue

        # Se a memória estiver cheia, remove a página mais longe (OPT)
        if len(paginas_mem) >= num_paginas:
            # Cria uma sublista de acessos futuros a partir do próximo acesso
            acessos = acesso_paginas[i + 1 :]
            maior_distancia = -1
            remover = None

            # Itera sobre as páginas na memória para encontrar a que será acessada mais tarde
            for j, pag in enumerate(paginas_mem):
                try:
                    # Encontra o índice do próximo acesso da página atual na lista de acessos futuros
                    proximo_acesso = acessos.index(pag)
                except ValueError:
                    # Se a página não for mais aessada, pode ser removida imediatamente
                    remover = j
                    break

                # Se o próximo acesso for maior que a maior distância encontrada até agora
                # atualiza a maior distância e a página a ser removida
                if proximo_acesso > maior_distancia:
                    maior_distancia = proximo_acesso
                    remover = j

            # Remove a página que será acessada mais tarde
            paginas_mem.pop(remover)

        # Adiciona a nova página à memória
        paginas_mem.append(pagina)

        # Incrementa uma falta de página uma vez que ela foi buscada na memória
        faltas += 1

    fim = time.perf_counter()
    tempo = fim - inicio

    return faltas, tempo


def mostrar_resultado(algoritmo, faltas, tempo, total_paginas):
    print(f"Algoritmo          : {algoritmo}")
    print(f"Faltas de página   : {faltas}")
    print(f"Tempo              : {tempo:.2f} s")
    print(f"Taxa               : {total_paginas/tempo:,.0f} paginas/s")


if __name__ == "__main__":
    main()
