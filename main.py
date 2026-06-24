import sys
import io
import time
import zstandard as zstd

unidades = {"B": 0, "KB": 1, "MB": 2, "GB": 3}


def get_tamanho_bytes(message):
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


def main():
    # TODO: Remove hardcoded example file path

    # if len(sys.argv) != 2:
    #     print(f"Uso: {sys.argv[0]} arquivo.zst")
    #     sys.exit(1)

    arquivo = "data/acessos-Demo0.txt.zst"  # sys.argv[1]

    print("Formato: TAMANHO [B|KB|MB|GB]. Exemplo: 1024 MB.")

    # Solicita ao usuário o tamanho da memória e da página e converte para bytes
    tam_memoria = get_tamanho_bytes("Informe o tamanho da memória: ")
    tam_pagina = get_tamanho_bytes("Informe o tamanho da página: ")

    # Garante que o tamanho da página seja menos ou igual ao tamanho da mem´roia
    while tam_pagina > tam_memoria:
        print("O tamanho da página deve ser menor ou igual ao tamanho da memória.")
        tam_pagina = get_tamanho_bytes("Informe o tamanho da página: ")

    print(tam_memoria, tam_pagina)

    inicio = time.perf_counter()

    total_linhas = 0
    unicas = {}

    with open(arquivo, "rb") as fh:
        dctx = zstd.ZstdDecompressor(max_window_size=2147483648)

        with dctx.stream_reader(fh) as reader:
            text_stream = io.TextIOWrapper(reader, encoding="utf-8")

            for linha in text_stream:
                linha = linha.strip()

                total_linhas += 1

                unicas[linha] = unicas.get(linha, 0) + 1

    fim = time.perf_counter()

    tempo = fim - inicio

    print(f"Arquivo            : {arquivo}")
    print(f"Total de linhas    : {total_linhas}")
    print(f"Linhas únicas      : {len(unicas)}")
    print(f"Tempo              : {tempo:.2f} s")

    if tempo > 0:
        print(f"Taxa               : " f"{total_linhas/tempo:,.0f} linhas/s")


if __name__ == "__main__":
    main()
