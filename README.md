# Simulador RISC-V (RV32I)

Simulador didático de um processador **RISC-V RV32I de ciclo múltiplo**, executado inteiramente no navegador (React + Vite). O projeto reúne um montador de Assembly, uma memória endereçada por byte (little-endian), o banco de 32 registradores e uma interface que mostra, estágio por estágio, o que acontece no caminho de dados a cada pulso de clock.

> O projeto nasceu como simulador do MAC-1 (o MIC-1 de Tanenbaum) e foi migrado para RV32I. Parte dos arquivos do MAC-1 continua no repositório; veja [Código legado](#código-legado-mac-1).

---

## Funcionalidades

- **Unidade de controle como FSM de 5 estágios**: Fetch, Decode, Execute, Memory e Write-back, um estágio por pulso de clock.
- **ISA RV32I** nos formatos R, I, S, B, U e J (`FENCE` não é suportada; `ECALL`/`EBREAK` são decodificadas, mas não fazem nada).
- **Montador de duas passagens** com rótulos, comentários, imediatos em decimal/hex/binário e 22 pseudoinstruções (`li`, `mv`, `j`, `ret`, `beqz`...).
- **Importação de binários reais** (`.bin` gerado com `objcopy`) em um endereço base configurável.
- **Visualização do caminho de dados**: registradores x0–x31 (HEX/DEC/BIN), PC, MAR, MBR/MDR, operandos, saída da ALU com flags Z/N, resultado do desvio e etapa atual.
- **Janelas de memória** (até 3 simultâneas) com troca de base e busca por endereço.
- **IPS real**: instruções por segundo medidas em tempo real.
- **Log de eventos** (instruções executadas e escritas na RAM).
- **Três processadores selecionáveis** na interface (`RV32I`, `RV32I (Cache D)` e `RV32I (Cache A)`). Hoje só o primeiro está completo; veja [Estado atual](#estado-atual-e-limitações-conhecidas).

---

## Como executar

Requisitos: **Node.js 20.19+ ou 22.12+** (exigência do Vite 8) e npm.

```bash
npm install
npm run dev       # http://localhost:5173
```

Outros comandos:

```bash
npm run build     # gera a build de produção em dist/
npm run preview   # serve a build localmente
npm run lint      # ESLint (ainda com pendências)
```

### Fluxo de uso

1. Abra o painel **Instructions** (botão flutuante) e escreva o código Assembly. Já há um programa de exemplo carregado.
2. Clique em **Montar Programa RV32I**. O código é montado e gravado na RAM a partir do endereço `0x0`.
3. Abra uma janela de memória com **+** para acompanhar as escritas.
4. Clique em **►** para executar e em **||** para pausar. O campo **Tempo (ms)** com o botão **Set Clock** muda o intervalo entre estágios (padrão: 1000 ms).
5. **Clear** limpa a RAM e reseta registradores e clock.

Para usar um programa compilado, use **Upload .bin** no painel de memória (veja [Binários externos](#binários-externos-bin)).

---

## Estrutura do projeto

```
.
├── mac1/                      # Processador RV32I base
│   ├── controlUnit.js         # Unidade de controle: decodificador + FSM de 5 estágios
│   ├── componentes/
│   │   ├── alu.js             # ALU e avaliação de condições de desvio
│   │   ├── registers.js       # x0–x31, PC, nomes ABI, helpers toHex32/toBinary32
│   │   ├── memory.js          # RAM byte a byte, little-endian (instância única)
│   │   ├── clock.js           # Clock de 5 subciclos
│   │   └── amux, mir, mpc...  # legado do MAC-1 (não usados)
│   └── manual/                # Referência do MAC-1 (legado)
├── mac2/                      # Variante "Cache D" (estende o mac1)
├── mac3/                      # Variante "Cache A" (estende o mac1)
├── montador/
│   ├── montador.js            # Assembler de duas passagens
│   └── macroinstrucoes.js     # Codificadores R/I/S/B/U/J e tabela de opcodes
├── src/                       # Interface React
│   ├── App.jsx                # Alterna entre "Simulador" e "Comparacao"
│   ├── context/MacContext.jsx # Processador ativo (mac1 | mac2 | mac3)
│   ├── hooks/useIPS.js        # Medição de instruções por segundo
│   ├── services/              # simulationLog.js e performanceTracker.js
│   ├── components/            # memory, SImulation, instructions, simulationLog
│   └── pages/RaceComparison/  # Página de comparação entre processadores
└── testes/                    # Scripts de teste em Node
```

> `testes/teste3.js` não é um teste: é o módulo que instancia o `Clock` e as três unidades de controle usadas pela interface.

---

## Arquitetura

### Unidade de controle (`mac1/controlUnit.js`)

A `ControlUnit` executa uma instrução em **5 subciclos**, chamados por `rodarCiclo(subciclo, ciclos)`:

| Subciclo | Estágio | O que acontece |
|:---:|---|---|
| 1 | **FETCH** (IF) | Lê a palavra de 32 bits em `RAM[PC]` para o IR/MBR, calcula `PC + 4` e registra a instrução no log |
| 2 | **DECODE** (ID) | Separa `opcode`, `rd`, `rs1`, `rs2`, `funct3`, `funct7` e o imediato (com extensão de sinal); lê `rs1`/`rs2` para os latches A e B |
| 3 | **EXECUTE** (EX) | A ALU opera sobre registradores/imediato; calcula o endereço de loads/stores, a condição e o alvo de desvios, e o link de `JAL`/`JALR` |
| 4 | **MEMORY** (MEM) | Loads leem a RAM (`LB`/`LH`/`LW`/`LBU`/`LHU`) para o MDR; stores escrevem (`SB`/`SH`/`SW`) |
| 5 | **WRITE-BACK** (WB) | Grava `rd` (x0 é ignorado) com o MDR ou a saída da ALU, atualiza o PC (`PC + 4`, desvio ou salto) e incrementa o contador de instruções concluídas |

Outros métodos úteis: `getEstado()` (snapshot completo para a interface), `setCallback()` (notifica a UI a cada subciclo), `reset()` e `hardReset(baseAddress)`.

### Clock (`mac1/componentes/clock.js`)

```js
const clock = new Clock();
clock.setVelocidade(250);  // ms entre subciclos (padrão: 1000)
clock.iniciar(uc);         // execução contínua, um estágio por tick
clock.pausar();            // pausa sem perder estado
clock.pulso(uc);           // executa uma instrução completa (5 subciclos)
clock.resetClock();        // zera subciclo e contador de ciclos
```

O contador de ciclos aumenta a cada 5 subciclos, ou seja, uma vez por instrução.

### Registradores (`mac1/componentes/registers.js`)

32 registradores de 32 bits (`x0`–`x31`) mais o PC. `x0` é sempre zero: escritas são ignoradas. O montador aceita tanto `x0`–`x31` quanto os nomes ABI (`zero`, `ra`, `sp`, `gp`, `tp`, `t0`–`t6`, `s0`/`fp`–`s11`, `a0`–`a7`).

### Memória (`mac1/componentes/memory.js`)

- Endereçamento por byte, **little-endian**, com leitura e escrita de byte, meia-palavra e palavra.
- Existe uma única instância (`memoria`), compartilhada por toda a aplicação.
- **Tamanho padrão: 128 bytes (32 palavras).** O tamanho deve ser potência de 2, pois os endereços são mascarados por `size - 1` (acessos fora da faixa dão a volta). Para programas maiores, altere o valor padrão de `new Memory(sizeInBytes)`. A interface espelha até 4096 palavras.

### ALU (`mac1/componentes/alu.js`)

Operações: `ADD`, `SUB`, `SLL`, `SLT`, `SLTU`, `XOR`, `SRL`, `SRA`, `OR`, `AND`. Gera as flags `Z` (zero) e `N` (negativo). As condições de desvio (`BEQ`, `BNE`, `BLT`, `BGE`, `BLTU`, `BGEU`) são avaliadas por `testBranchCondition()`.

---

## O Montador (`montador/`)

### Fluxo

1. **Primeira passagem** (`primeira_passagem`): remove comentários, identifica rótulos, expande pseudoinstruções e atribui endereços (`PC = índice × 4`).
2. **Segunda passagem** (`segunda_passagem`): resolve rótulos em offsets relativos ao PC, extrai `rd`/`rs1`/`rs2`/`imm` e codifica cada instrução em 32 bits.
3. **`preencherMemoria()`** grava as palavras na RAM a partir do endereço `0` e registra as instruções no log.

```js
import Montador from "./montador/montador.js";

const montador = new Montador(codigo);
const erro = montador.main();           // false = sucesso; string = mensagem de erro
if (!erro) montador.preencherMemoria();
```

### Sintaxe

- Comentários com `#` ou `;`.
- Rótulos no formato `nome:`, sozinhos na linha ou antes da instrução (sem diferenciar maiúsculas de minúsculas).
- Imediatos em decimal, `0x...` ou `0b...`, inclusive negativos.
- Operandos de memória no formato `imm(reg)`, como `4(sp)`.
- Desvios e `JAL` aceitam rótulo; o offset relativo é calculado automaticamente.
- **Não há diretivas** (`.data`, `.word`, `.text`...): o código é composto apenas de instruções.

### Instruções suportadas

| Formato | Instruções |
|---|---|
| R | `ADD` `SUB` `SLL` `SLT` `SLTU` `XOR` `SRL` `SRA` `OR` `AND` |
| I (ALU) | `ADDI` `SLTI` `SLTIU` `XORI` `ORI` `ANDI` `SLLI` `SRLI` `SRAI` |
| I (load) | `LB` `LH` `LW` `LBU` `LHU` |
| I (salto/sistema) | `JALR` `ECALL` `EBREAK` |
| S | `SB` `SH` `SW` |
| B | `BEQ` `BNE` `BLT` `BGE` `BLTU` `BGEU` |
| U | `LUI` `AUIPC` |
| J | `JAL` |

**Pseudoinstruções:** `NOP`, `MV`, `NOT`, `NEG`, `SEQZ`, `SNEZ`, `SLTZ`, `SGTZ`, `LI` (1 ou 2 instruções, conforme o valor), `J`, `JR`, `RET`, `BEQZ`, `BNEZ`, `BLEZ`, `BGEZ`, `BLTZ`, `BGTZ`, `BGT`, `BLE`, `BGTU`, `BLEU`.

### Exemplo

```asm
        li   x1, 10
        li   x2, 20
        add  x3, x1, x2      # x3 = 30
        sw   x3, 64(x0)      # MEM[64] = 30
        lw   x4, 64(x0)      # x4 = 30
        beq  x3, x4, fim     # desvio tomado
        addi x5, x0, 99      # (pulada)
fim:    addi x5, x0, 42      # x5 = 42
```

Ao final da execução: `x3 = 30`, `x4 = 30`, `x5 = 42` e `MEM[64] = 30`.

### Binários externos (`.bin`)

O painel de memória importa um **binário bruto (flat binary, bare-metal)**, e não um `.elf`:

```bash
riscv64-unknown-elf-objcopy -O binary programa.elf programa.bin
```

Informe o endereço base (hex ou decimal) e use **Upload .bin**. A memória é limpa, os bytes são gravados em little-endian (`loadRawBinary`) e a unidade de controle recebe um `hardReset(base)`, que zera os registradores e aponta o PC para o endereço base. Lembre-se de que o binário precisa caber na RAM (128 bytes por padrão).

---

## Interface (`src/`)

`App.jsx` tem dois botões no topo, **Simulador** e **Comparacao**, e um painel de **Log** disponível nas duas telas. O `MacContext` guarda qual processador está ativo.

### Tela "Simulador"

Três painéis lado a lado:

```
┌──────────┐  ┌────────────┐  ┌──────────────┐
│  Memory  │  │ Simulation │  │ Instructions │
└──────────┘  └────────────┘  └──────────────┘
```

**Memory** (`src/components/memory/`) é o painel de controle:
- **Clear**, **►**, **||**, campo **Tempo (ms)** e **Set Clock**.
- **IPS Real**: instruções concluídas por segundo na máquina hospedeira, atualizado uma vez por segundo (`useIPS`).
- **Importar Binário Real (.bin)** com endereço base e ajuda de toolchain.
- Até **3 janelas** `MemoryDisplay`, abertas com `+` e fechadas com `×`. Cada uma atualiza a cada 100 ms, alterna entre **HEX / BIN / DEC**, carrega 100 linhas por vez e tem busca por endereço ou linha.

**Simulation** (`src/components/SImulation/`):
- Botões para escolher o processador: **RV32I (Datapath)** (`mac1`), **RV32I (Cache D)** (`mac2`) e **RV32I (Cache A)** (`mac3`), mais o campo de tamanho da cache.
- Banco de registradores x0–x31 com formato alternável (no modo BIN mostra os 16 bits menos significativos).
- Blocos do caminho de dados: Clock & FSM, etapa atual, mnemônico, formato/imediato, operação da FSM, PC, desvio (`TAKEN`/`NOT TAKEN`), próximo PC, MAR, MBR/MDR, RS1, RS2, operando A, ALU (com Z/N) e saída da ALU.
- Tabela **Conteúdo da Cache** (mostra "Modo Direto" quando não há cache).

**Instructions** (`src/components/instructions/`): painel colapsável com um `textarea` para o Assembly e o botão **Montar Programa RV32I**. Erros de montagem aparecem em `alert()`, com a instrução problemática.

**Log** (`src/components/simulationLog/`): lista até 500 eventos, os mais recentes primeiro: `[INSTRUCTION]`, `[MEMORY WRITE]` e, quando houver cache integrada, `[CACHE HIT]`/`[CACHE MISS]`.

### Tela "Comparacao"

Pensada para rodar o mesmo programa em três processadores lado a lado (cada um com sua própria cópia da memória) e comparar ciclos, CPI e acertos de cache. **No momento esta tela não abre**; veja o próximo tópico.

---

## Estado atual e limitações conhecidas

- **A tela "Comparacao" está quebrada.** Ela falha ao renderizar porque o `performanceTracker` ainda monitora `MAC-1`/`MAC-2`/`MAC-3`, enquanto a página usa `RV32I`/`RV32I-C1`/`RV32I-C2`. Além disso, o laço dela executa 4 subciclos e a unidade de controle RV32I tem 5, então o Write-back nunca rodaria.
- **`mac2` e `mac3` ainda não têm cache.** Ambos apenas estendem o `mac1` e expõem um `cacheSize` e um `getSnapshot()` vazio; por isso a tabela de cache aparece vazia. O protótipo em `mac2/componentes/cache.js` (write-through, blocos de 4 palavras, totalmente associativo, substituição FIFO, endereços de 16 bits) é herdado do MAC-1 e ainda não está conectado ao RV32I.
- **Não há instrução de parada.** `ECALL`/`EBREAK` não encerram a execução: ao acabar o programa, o PC continua avançando sobre palavras zeradas até você pausar.
- **A RAM padrão é pequena** (128 bytes); veja [Memória](#memória-mac1componentesmemoryjs).
- `Header` e `StackMemoryDisplay` existem em `src/components/`, mas não são exibidos na interface atual.

### Código legado (MAC-1)

Os arquivos abaixo pertencem à versão anterior do projeto e **não são usados** pelo RV32I:

- `mac1/componentes/`: `amux`, `cs`, `decoderA/B/C`, `increment`, `latchA/B`, `mar`, `mbr`, `mir`, `mmux`, `mpc`, `msl` e `shifter`.
- `mac2/componentes/cs.js`, `mac3/componentes/cs.js` e `mac3/componentes/decoderI.js`.
- `mac1/manual/`: tabela de macroinstruções e microprograma do MAC-1, mantida como referência histórica.

---

## Testes

Não há test runner configurado: os testes são scripts Node que lançam erro se uma asserção falhar. Execute-os na raiz do projeto:

```bash
node testes/testeRiscV.js
```

| Script | O que cobre | Situação |
|---|---|---|
| `testeRiscV.js` | Registradores, memória little-endian, ALU, montador e execução com `BEQ` | Passa |
| `testeBateriaCompleta.js` | Bateria da ISA: `LUI`/`AUIPC`, aritmética, shifts, loads/stores, desvios, `JAL`/`JALR` | Passa |
| `testeBinaryLoader.js` | Carregamento de `.bin` e endereço base arbitrário | Passa |
| `testeIPS.js` | `formatIPS` e contador de instruções (importa React; rode `npm install` antes) | Passa |
| `testeAlu.js` | Imprime resultados e flags da ALU (sem asserções) | Roda |
| `teste.js`, `teste2.js`, `teste.txt` | Programas do MAC-1 em binário de 16 bits | Legado |
| `regressaoMac2.js` | Regressão do MAC-2 | Legado (falha: depende do `mpc` do MAC-1) |

---

## Dependências

| Pacote | Versão | Papel |
|---|---|---|
| `react` / `react-dom` | ^19.2 | Interface |
| `vite` | ^8.0 | Bundler e servidor de desenvolvimento |
| `@vitejs/plugin-react` | ^6.0 | Suporte a JSX |
| `eslint` + `eslint-plugin-react-hooks` + `eslint-plugin-react-refresh` | ^10.3 / ^7.1 / ^0.5 | Lint |
