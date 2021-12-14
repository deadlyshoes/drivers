import sys


def main():
    efeitos = {'normal': '0', 'robo': '1', 'alien': '2'}

    try:    
        efeito = sys.argv[1]
        print("Efeito aplicado")
    except:
        print(" Passe um dos parametros abaixo,\n para selecionar um efeito:\n")
        print("- normal;")
        print("- robo;")
        print("- alien.")


if __name__ == "__main__":
    main()