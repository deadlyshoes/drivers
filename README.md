# Efeitos de voz em kernel  
  
Neste projeto, foram modificados drivers de áudio do linux com objetivo de aplicar efeitos de voz no microphone do usuário.  
Dessa forma, durante uma chamada, os outros participantes irão ouvir sua voz com alguma distorção. Mais especificamente, os  
efeitos elaborados, podem deixam a voz do usuário semelhante a de um robô ou alienígena de ficção científica.  

Nos tópicos abaixo, serão explicados, como os efeitos de voz foram implementados no kernel, quais rotinas foram 
modificadas em cada driver e como substituir os módulos já carregados na sua máquina pelos módulos modificados 
neste repositório.

### Criação dos efeitos
Como os dados do áudio já estão digitalizados no momento em que são acessados pelas rotinas que serão comentadas nos tópicos
abaixo, é necessário, para aplicação dos efeitos, convertê-los para o domínio da frequência utilizando uma DFT (Discrete Fourier Tansform, ou 
Transformada de Fourier). Depois que o efeito é aplicado, basta aplicar uma IDFT (Inverse Discrete Fourier Transform, ou 
Transformada de Fourier Inversa) para retornar os dados ao formato original.

### snd-aloop  
A função `copy_play_buf()` é responsável por copiar os dados da saída de áudio deste módulo para saída do microfone. Para instalação,
seguem os comandos abaixo, que devem ser executados pelo terminal na pasta com o módulo modificado e o makefile:

```
$ make -C /lib/modules/$(uname -r)/build M=$(pwd) modules
```
```
$ sudo insmod snd-aloop.ko
```

### snd-usb-audio
A modificação foi feita dentro da função `retire_capture_urb()` responsável pela cópia dos dados do áudio para a memória para
serem utilizados por aplicações no espaço do usuário. Para instalação, basta executar os comandos no terminal, dentro da pasta
onde foi baixado junto com o makefile:

```
$ make -C /lib/modules/$(uname -r)/build M=$(pwd) modules
```
```
$ sudo insmod snd-usb-audio.ko
```

### Parametrização
Com um dos módulos acima carregados na máquina, para alternar entre modo de voz com efeito e voz normal. Basta executar o programa
`robot.py` com permissão de adiministrador, passando como argumento uma das 3 opções de voz ('normal', 'robo' ou 'alien'):

```
$ sudo python3 robot.py [opção]
```
