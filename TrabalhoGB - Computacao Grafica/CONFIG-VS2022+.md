# Configurando o Visual Studio 
## Um mini-tutorial

Esse tutorial funciona para as versões mais atuais do Visual Studio (2019, 2022, ...)

Uma coisa muito importante, ao configurar um projeto no VS, é escolher para qual arquitetura que o projeto será configurado. Para isso, certifique-se se suas dependências que possuem bibliotecas estáticas (.lib) e/ou dinâmicas (.dll) estão compiladas em x86 ou x64 (arquiteturas 32 ou 64 bits). Considerando as dependências já existentes neste repositório, usaremos a arquitetura 64 bits. que é a padrão atual:

![image](https://github.com/user-attachments/assets/4e8a62f9-9cce-496c-b050-538c8d4d39df)

Se você quiser ou precisar alterar as dependências, você deve ir em Projeto -> Propriedades e alterar estes 3 lugares:

1) Diretório com os arquivos de cabeçalho (onde ficam os .h da biblioteca): 
![image](https://user-images.githubusercontent.com/2465857/128772404-13b3a1a4-fc71-4a93-9de1-1eb9cba05969.png)

2) Diretório com os arquivos de biblioteca estática pré-compilados (.lib ou .a):
![image](https://user-images.githubusercontent.com/2465857/128772735-d04bec8e-edcd-485f-a103-e66f1810cdad.png)

3) Incluir os nomes das bibliotecas estáticas:
![image](https://user-images.githubusercontent.com/2465857/128772961-04745bc6-ef1d-45b3-9aa7-a623d2af3065.png)

Ao rodar o programa Hello3D, você deverá obter este resultado:

![image](https://github.com/user-attachments/assets/45e2690e-48ac-45d8-a745-1c907002927d)
