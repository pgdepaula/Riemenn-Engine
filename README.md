# Riemann Engine

Motor gráfico multiplataforma e simulador espacial de alta precisão, desenvolvido em C.

O projeto combina uma engine própria de renderização com uma aplicação de simulação astronômica baseada em modelos físicos e matemáticos utilizados na mecânica orbital moderna.

---

## Visão Geral

O objetivo da Riemann Engine é representar o Sistema Solar com fidelidade científica, calculando posições, distâncias e fenômenos orbitais em tempo real a partir de dados astronômicos reais.

Diferentemente de simuladores baseados em tabelas estáticas ou eventos pré-programados, o sistema prioriza cálculos físicos e matemáticos para obter resultados diretamente da simulação.

### Principais características

* Simulação do Sistema Solar baseada no padrão astronômico J2000.
* Cálculo de posições planetárias para datas passadas, atuais e futuras.
* Escala física real ou escala adaptada para visualização didática.
* Dados orbitais calculados em tempo real.
* Estatísticas derivadas da própria simulação.
* Renderização acelerada por GPU.
* Execução nativa em desktop e navegadores modernos.

---

## Precisão Científica

A engine não depende de conhecimento pré-programado sobre eventos astronômicos específicos.

Por exemplo:

* Não existe uma regra interna dizendo que um ano terrestre possui aproximadamente 365 dias.
* Não existe uma regra dizendo quando a Terra estará mais próxima ou mais distante do Sol.

Essas informações são obtidas a partir dos cálculos orbitais executados durante a simulação.

Isso permite que diversos dados sejam derivados diretamente dos modelos matemáticos utilizados pelo sistema, produzindo resultados compatíveis com observações astronômicas reais.

Os fundamentos incluem conceitos presentes em áreas como:

* Mecânica orbital
* Gravitação clássica
* Relatividade
* Dinâmica de sistemas

O objetivo é transformar modelos normalmente vistos em artigos científicos, livros e quadros de aula em uma representação visual e interativa.

---

# Arquitetura

A engine foi construída com foco em portabilidade, desempenho e baixo nível de abstração.

## Linguagem

* C (ISO C)

## Camadas principais

### HAL (Hardware Abstraction Layer)

Responsável por abstrair recursos específicos do hardware.

### PAL (Platform Abstraction Layer)

Responsável por abstrair sistemas operacionais, janelas, entrada e integração com plataformas.

---

## APIs Gráficas

A engine suporta múltiplos backends de renderização:

| API      | Status          |
| -------- | --------------- |
| Vulkan   | Principal       |
| Metal    | Principal       |
| OpenGL   | Compatibilidade |
| Direct3D | Compatibilidade |

Essas APIs são utilizadas diretamente através da camada de abstração da engine.

---

## Plataformas Suportadas

| Plataforma            | Backend padrão |
| --------------------- | -------------- |
| Linux (Wayland)       | Vulkan         |
| Linux (X11)           | Vulkan         |
| FreeBSD (Wayland)     | Vulkan         |
| FreeBSD (X11)         | Vulkan         |
| Windows               | Direct3D       |
| macOS (Apple Silicon) | Metal          |
| macOS (Intel)         | Metal          |
| WebAssembly/WebGPU    | WebGPU         |

---

## Paralelismo e Desempenho

A arquitetura foi projetada para aproveitar ao máximo os recursos disponíveis da máquina.

Dependendo da plataforma e configuração:

* Processamento físico pode ser executado na GPU.
* Simulações podem ser distribuídas entre múltiplos núcleos da CPU.
* Renderização e cálculos podem ocorrer de forma paralela.

O objetivo é manter alta precisão sem comprometer a taxa de atualização da aplicação.

---

## Dependências

O projeto busca minimizar dependências externas.

Além das APIs gráficas e componentes nativos de cada sistema operacional, a engine evita bibliotecas de terceiros sempre que possível.

---

## Web

A Riemann Engine também pode ser compilada para:

* WebAssembly (WASM)
* WebGPU

Permitindo execução diretamente em navegadores modernos.

---

# Compilação

## CMake (Recomendado)

### Configurar

```bash
cmake -S . -B build
```

### Compilar

```bash
cmake --build build --parallel $(nproc)
```

---

## Configurações Manuais

### Wayland + Vulkan

```bash
cmake -S . -B build \
    -DPLATFORM=LINUX_WAYLAND \
    -DGPU_API=vulkan
```

### Windows + Direct3D

```bash
cmake -S . -B build \
    -DPLATFORM=WINDOWS \
    -DGPU_API=dx11
```

O sistema detecta automaticamente a plataforma e o backend gráfico recomendado para cada ambiente.

As opções podem ser sobrescritas manualmente através das flags de configuração.

---

# Plataformas e APIs Disponíveis

| Plataforma    | Backend padrão | Alternativas        |
| ------------- | -------------- | ------------------- |
| linux-x11     | Vulkan         | OpenGL              |
| linux-wayland | Vulkan         | OpenGL              |
| fbsd-x11      | Vulkan         | OpenGL              |
| fbsd-wayland  | Vulkan         | OpenGL              |
| windows       | Direct3D       | Vulkan, OpenGL      |
| mac           | Metal          | —                   |
| mac_x86       | Metal          | OpenGL (deprecated) |
| navigator     | WebGPU         | —                   |

---

# Compiladores

Recomendados:

* Clang
* LLVM Clang
* Apple Clang

Outros compiladores não foram amplamente testados e podem apresentar diferenças de comportamento ou incompatibilidades com determinadas flags de compilação.
