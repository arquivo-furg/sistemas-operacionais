import sys
import io
import time
import json
import math
import zstandard as zstd

UNIDADES = {"B": 0, "KB": 1, "MB": 2, "GB": 3, "TB": 4}
MAX_QUADROS_BARRA = 50


def main():
    if len(sys.argv) != 2:
        print(f"Uso: {sys.argv[0]} arquivo.zst")
        sys.exit(1)

    arquivo = sys.argv[1]
    print(f"Arquivo            : {arquivo}")

    # Solicita ao usuário o tamanho da memória e da página e converte para bytes
    tam_memoria = get_tamanho_bytes("Tamanho da memória : ")
    tam_pagina = get_tamanho_bytes("Tamanho da página  : ")

    # Garante que o tamanho da página seja menos ou igual ao tamanho da memória
    while tam_pagina > tam_memoria:
        tam_pagina = get_tamanho_bytes("Tamanho da página  : ")

    # Calcula o número de páginas que cabem na memória
    num_quadros = tam_memoria // tam_pagina
    print(f"\nPáginas na memória : {num_quadros}")

    # Calcula a estimativa do tamanho da tabela de páginas
    tamanho_tabela(tam_pagina, num_quadros)

    # Mapeia os endereços de acesso para números de página
    paginas_acessadas, total_acessos, num_paginas_unicas = get_paginas_acessadas(
        arquivo, tam_pagina
    )

    faltas_opt, tempo, carregamentos = OPT(paginas_acessadas, num_quadros)
    mostrar_resultado("OPT", faltas_opt, tempo, total_acessos, num_paginas_unicas)
    salvar_carregamentos("OPT", carregamentos)

    faltas_fifo, tempo, carregamentos = FIFO(paginas_acessadas, num_quadros)
    mostrar_resultado("FIFO", faltas_fifo, tempo, total_acessos, num_paginas_unicas)
    salvar_carregamentos("FIFO", carregamentos)

    eficiencia_fifo = faltas_opt / faltas_fifo
    print(f"\nEficiência FIFOxOPT: {eficiencia_fifo:.2%}")


def get_tamanho_bytes(message):
    while True:
        try:
            # Obtém o tamanho e a unidade do usuário, separados por espaço
            tamanho, unidade = input(message).split(" ")

            # Busca a potência correspondente à unidade fornecida pelo usuário
            potencia = UNIDADES.get(unidade.upper(), -1)

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


def tamanho_tabela(tam_pagina, num_quadros):
    # Cada caractere do endereço está em hexadecimal, 16 valores de 0 a F -> 2^4
    # Os endereços possuem 12 caracteres, que combinados, dão o espaço de endereçamento total
    # Espaço de endereçamento = 16^12 -> (2^4)^12 -> 2^48 bytes
    num_entradas = 2**48 // tam_pagina

    # Bits necessários para indexar todos os quadros físicos
    bits_entrada = math.ceil(math.log2(max(num_quadros, 2)))

    # Arredonda para o menor byte que comporta esses bits
    bytes_entrada = 1
    while bytes_entrada * 8 < bits_entrada:
        bytes_entrada *= 2

    # Calcula o tamanho total da tabela de páginas em bytes
    # Cada página possível precisa de uma entrada na tabela,
    #   pois cada página pode ser mapeada para um quadro físico diferente
    tam_quadro = num_entradas * bytes_entrada

    print(f"Tabela de páginas  : {formatar_bytes(tam_quadro)}")


def formatar_bytes(valor):
    unidades = list(UNIDADES.keys())
    i = 0
    while valor >= 1024 and i < len(unidades) - 1:
        valor /= 1024
        i += 1
    return f"{valor:.2f} {unidades[i]}"


def get_paginas_acessadas(arquivo, tam_pagina):
    paginas_acessadas = []
    paginas_unicas = set()

    with open(arquivo, "rb") as fh:
        dctx = zstd.ZstdDecompressor(max_window_size=2147483648)

        with dctx.stream_reader(fh) as reader:
            text_stream = io.TextIOWrapper(reader, encoding="utf-8")

            for linha in text_stream:
                # Converte a linha de endereço hexadecimal para inteiro
                endereco = int(linha, 16)

                # Calcula o número da página a partir do endereço
                num_pagina = endereco // tam_pagina

                # Adiciona o número da página à lista de acessos
                paginas_acessadas.append(num_pagina)

                # Registra as páginas distintas acessadas
                paginas_unicas.add(num_pagina)

    # Calcula o total de páginas acessadas
    total_acessos = len(paginas_acessadas)

    print(f"\nTotal de endereços : {total_acessos}")
    print(f"Páginas únicas     : {len(paginas_unicas)}")

    return paginas_acessadas, total_acessos, len(paginas_unicas)


def FIFO(paginas_acessadas, num_quadros):
    inicio = time.perf_counter()

    paginas_mem = []
    faltas = 0
    carregamentos = {}

    mostrar_barra = num_quadros <= MAX_QUADROS_BARRA

    if mostrar_barra:
        print(f"\nFIFO: progresso da memória ({num_quadros} quadros)")

    for pagina in paginas_acessadas:
        # Se a página já estiver na memória, não faz nada
        if pagina in paginas_mem:
            continue

        # Se a memória estiver cheia, remove a página mais antiga (FIFO)
        if len(paginas_mem) >= num_quadros:
            paginas_mem.pop(0)

        # Adiciona a nova página à memória
        paginas_mem.append(pagina)

        # Incrementa uma falta de página uma vez que ela foi buscada na memória
        faltas += 1

        # Incrementa contador de carregamentos da página
        pagina = str(pagina)
        carregamentos[pagina] = carregamentos.get(pagina, 0) + 1

        # Exibe barra de memória
        if mostrar_barra:
            barra_memoria(paginas_mem, num_quadros)

    if mostrar_barra:
        print()

    fim = time.perf_counter()
    tempo = fim - inicio

    return faltas, tempo, carregamentos


def OPT(paginas_acessadas, num_quadros):
    inicio = time.perf_counter()

    paginas_mem = []
    faltas = 0
    carregamentos = {}

    mostrar_barra = num_quadros <= MAX_QUADROS_BARRA

    if mostrar_barra:
        print(f"\nOPT: progresso da memória ({num_quadros} quadros)")

    for i, pagina in enumerate(paginas_acessadas):
        # Se a página já estiver na memória, não faz nada
        if pagina in paginas_mem:
            continue

        # Se a memória estiver cheia, remove a página mais longe (OPT)
        if len(paginas_mem) >= num_quadros:
            # Cria uma sublista de acessos futuros a partir do próximo acesso
            maior_distancia = -1
            remover = None

            # Itera sobre as páginas na memória para encontrar a que será acessada mais tarde
            for j, pag in enumerate(paginas_mem):
                try:
                    # Encontra o índice do próximo acesso da página atual na lista de acessos futuros
                    proximo_acesso = paginas_acessadas.index(pag, i + 1)
                except ValueError:
                    # Se a página não for mais aessada, pode ser removida imediatamente
                    remover = j
                    break

                # Se o próximo acesso for maior que a maior distância encontrada até agora
                #   atualiza a maior distância e a página a ser removida
                if proximo_acesso > maior_distancia:
                    maior_distancia = proximo_acesso
                    remover = j

            # Remove a página que será acessada mais tarde
            paginas_mem.pop(remover)

        # Adiciona a nova página à memória
        paginas_mem.append(pagina)

        # Incrementa uma falta de página uma vez que ela foi buscada na memória
        faltas += 1

        # Incrementa contador de carregamentos da página
        pagina = str(pagina)
        carregamentos[pagina] = carregamentos.get(pagina, 0) + 1

        # Exibe barra de memória
        if mostrar_barra:
            barra_memoria(paginas_mem, num_quadros)

    if mostrar_barra:
        print()

    fim = time.perf_counter()
    tempo = fim - inicio

    return faltas, tempo, carregamentos


def barra_memoria(paginas_mem, num_quadros):
    ocupados = len(paginas_mem)
    livres = num_quadros - ocupados

    barra = "█" * ocupados + "░" * livres

    print(f"[{barra}] {ocupados}/{num_quadros}", end="\r", flush=True)


def mostrar_resultado(algoritmo, faltas, tempo, total_paginas, paginas_unicas):
    # Taxa de acessos que não causaram falta de página
    acertos = total_paginas - faltas
    taxa_acerto = acertos / total_paginas if total_paginas > 0 else 0

    # Taxa de eficiência do algoritmo em relação ao número de páginas únicas
    eficiencia = paginas_unicas / faltas if faltas > 0 else 0

    print(f"\nAlgoritmo          : {algoritmo}")
    print(f"Faltas de página   : {faltas}")
    print(f"Acertos            : {acertos}")
    print(f"Taxa de acerto     : {taxa_acerto:.2%}")  # Acessos sem falta
    print(f"Eficiência         : {eficiencia:.2%}")  # vs Mínimo de faltas
    print(f"Tempo              : {tempo:.6f}s")


def salvar_carregamentos(algoritmo, carregamentos):
    nome_arquivo = f"carregamentos_{algoritmo.lower()}.json"

    with open(nome_arquivo, "w", encoding="utf-8") as f:
        json.dump(carregamentos, f, indent=2)

    print(f"Carregamentos/pág  : {nome_arquivo}")


if __name__ == "__main__":
    main()
