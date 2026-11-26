# Simulador RISC-V RV32I em C++

Aplicação desktop educacional que migra o simulador original em JavaScript para C++20. O núcleo mantém três pistas independentes: datapath sem cache, cache de mapeamento direto e cache associativa. A interface usa SDL2, OpenGL e Dear ImGui.

## Arquitetura

- `mac1/`: registradores, RAM little-endian, ALU, relógio e unidade de controle RV32I. Esta pista é a referência para paridade funcional.
- `mac2/` e `mac3/`: controladores que compartilham o núcleo e consultam caches de 32 bits (direta e associativa).
- `montador/`: codificadores, duas passagens, pseudoinstruções e carregamento de programas.
- `src/services/`: log limitado a 500 eventos e métricas de desempenho sincronizados.
- `src/simulador.*`: composição das pistas isoladas, execução temporizada ou turbo em worker, e publicação de snapshots imutáveis para a UI.
- `src/components/` e `src/pages/`: interface ImGui das abas Simulador e Comparação.
- `testes/`: suíte Catch2, paridade golden, carregador binário, fuzz diferencial e testes dos recursos da interface/núcleo.

## Requisitos

- CMake 3.24 ou mais recente e Ninja (os presets usam Ninja).
- Compilador C++20: GCC, Clang ou MSVC.
- Node.js para regenerar os traços JS de referência.
- Para a interface: SDL2, Dear ImGui e nativefiledialog-extended são baixados em versões fixadas pelo CMake; OpenGL deve estar disponível no sistema.
- Windows: toolchain com suporte a recursos `.rc` (MinGW ou MSVC). Os binários estáticos SDL/ImGui/NFD são empacotados junto com as dependências de runtime detectadas.

As dependências de desenvolvimento são obtidas com CMake `FetchContent` na primeira configuração. É necessária conexão de rede nesse passo.

## Configurar e compilar

Execute os comandos dentro de `cpp/`:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

O preset `debug` habilita GUI e testes. Para compilar a versão otimizada:

```sh
cmake --preset release
cmake --build --preset release
ctest --preset release --output-on-failure
```

Para compilação sem janela, por exemplo em CI headless, configure com `-DRV_BUILD_GUI=OFF`. Sanitizers podem ser ativados em toolchains GCC/Clang com `-DRV_ENABLE_SANITIZERS=ON`.

## Executar a interface

Depois do build, execute `build/debug/rv_sim` (Windows: `build/debug/rv_sim.exe`). A aba **Simulador** oferece execução por clock e Modo Turbo; **Comparação** executa as três pistas isoladamente. O programa de exemplo aparece no editor de instruções. Também é possível carregar arquivos `.bin` por seletor ou arrastar o arquivo para a janela.

## Testes e paridade

`ctest --preset debug --output-on-failure` executa os testes do núcleo, montador, cache, UI, concorrência do worker Turbo, carregador binário e comparação de traces. O corpus de referência está em `testes/golden/`.

O script `../testes/gerarTracos.js` executa os programas pelo simulador JavaScript original e atualiza os JSON golden. Rode-o a partir da raiz do repositório com Node.js; depois rode novamente `ctest` para comparar os estados subciclo a subciclo. O teste de fuzz usa uma semente determinística para comparar instruções RV32I válidas entre o núcleo JS e C++.

## Empacotamento

CPack instala o executável e este README. No Windows, a instalação também coleta as DLLs de runtime necessárias; no Linux, o pacote é `.tar.gz` e `.zip`; no macOS, `.dmg` e `.zip`.

```sh
cmake --install build/release --prefix stage
cpack --config build/release/CPackConfig.cmake
```

O gerador Windows produz `.zip` e adiciona instalador NSIS quando `makensis` está disponível. Os artefatos ficam no diretório de build usado pelo CPack. Pacotes para outros sistemas devem ser gerados nativamente nessas plataformas.

## Checklist final da migração (Seção 12)

- Núcleo e instruções: subciclos, RV32I inteiro, registradores, RAM desalinhada/wrap-around, montador, pseudoinstruções e `.bin` cobertos pelos testes de unidade e paridade.
- Interface: abas Simulador/Comparação, controles de memória, IPS, visualizações múltiplas, registradores, cache, montagem/modal de erro e log recente disponíveis.
- Qualidade: corpus golden, mensagens/codificações do montador, fuzz determinístico, caches e execução paralela com proteção de estado cobertos pela suíte CTest.
- Builds Debug/Release e empacotamento são verificações locais descritas acima. Builds nativos de Linux/macOS requerem seus respectivos sistemas.
