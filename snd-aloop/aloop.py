import sys


def main():
    efeitos = {'normal': '0', 'robo': '1', 'alien': '2'}

    try:
        efeito = efeitos[sys.argv[1]]

    except:
        print(" Passe um dos parametros abaixo,\n para selecionar um efeito:\n")
        print("- normal;")
        print("- robo;")
        print("- alien.")
        sys.exit()

    with open("/sys/module/aloop_robot/parameters/efeito", "w") as efeito_file:
        efeito_file.write(efeito)

if __name__ == "__main__":
    main()
