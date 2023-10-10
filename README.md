# checkFile

## Introdução

O `checkFile` é uma aplicação desenvolvida em linguagem C que verifica o tipo de um ficheiro com base no seu conteúdo e compara-o com a extensão fornecida no nome do ficheiro. A motivação por trás deste projeto é que, embora um ficheiro possa ser nomeado como "a.png" ou "a.zip", não há garantia de que a extensão represente verdadeiramente o tipo de conteúdo do ficheiro.

### Funcionalidades:

- Utiliza o utilitário `file` do Linux para determinar o tipo de conteúdo do ficheiro.
- Verifica se a extensão do nome do ficheiro corresponde ao seu conteúdo real.
- Suporta vários tipos de ficheiros, como PDF, GIF, JPG, PNG, MP4, ZIP e HTML.
- Opções versáteis de linha de comando para analisar ficheiros individuais, em lote ou dentro de um diretório.

## Como usar:

### Sintaxe da linha de comandos:

checkFile [opções] [nome_do_ficheiro]

### Opções:
- `-f, --file <fich>`: Analisa o ficheiro especificado.
- `-b, --batch <fich_with_filenames>`: Analisa múltiplos ficheiros listados no ficheiro fornecido.
- `-d,--dir <directory>`: Analisa todos os ficheiros dentro do diretório especificado.
- `-h, --help`: Mostra a ajuda e informações adicionais.

## Tratamento de erros:

O programa validará os parâmetros da linha de comando, garantindo que os ficheiros e diretórios fornecidos existam. Além disso, trata os sinais SIGQUIT e SIGUSR1 de forma específica.
