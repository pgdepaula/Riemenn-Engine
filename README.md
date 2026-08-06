# Riemenn Engine

Motor gráfico e framework de simulação científica de alta performance, desenvolvido em **C11**.

Projetado com arquitetura modular limpa — separação clara entre **engine** (genérica, reutilizável) e **aplicação** (simulação astrofísica). O primeiro aplicativo construído sobre a engine é um simulador orbital do Sistema Solar com precisão científica.

---

## Arquitetura

```
engine/
├── foundation/     Logging, assertions — zero dependências
├── math/           Vetores 4D, matrizes 4x4, tensores, métricas de Kerr/Schwarzschild
├── platform/       Abstração de SO (Wayland, Win32)
├── rhi/            Render Hardware Interface (Vulkan, D3D12)
├── ecs/            Entity Component System + sistema de eventos
├── physics/        Integradores numéricos (RK4, Leapfrog, Yoshida), geodésicas
├── assets/         Loaders de imagem (PNG) e SVG
├── geometry/       Geração procedural de meshes
├── render/         Camera 3D, controller, shader utilities
├── scene/          Scene graph genérico
├── ui/             Framework de UI imediato (widgets, layout, temas, render 2D)
└── engine.h        API pública unificada

game/
├── simulation/     Cenários, presets (Solar System J2000, Kerr), fábricas de entidades
├── render/         Passes de renderização (black hole lensing, planetas, espaço-tempo)
├── screens/        HUD, tela inicial, viewport
├── input/          Mapeamento de entrada do jogador
├── config/         Configurações persistentes
└── debug/          Telemetria de física
```

### Diagrama de Dependências

```
Camada 0   foundation          (log, assert)
Camada 1   math                (vec4, mat4, tensor, spacetime)
Camada 2   platform + rhi      (Wayland/Win32 + Vulkan/D3D12)
Camada 3   ecs, physics, assets, geometry, render, scene
Camada 4   ui                  (widgets, layout, render 2D)
Camada 5   game/               (aplicação — simulação astrofísica)
```

Cada camada depende **apenas** das camadas abaixo. Zero dependências circulares.

---

## Precisão Científica

A simulação não utiliza dados pré-programados. Resultados como período orbital, periélio e afélio são **derivados dos cálculos físicos** em tempo real.

Fundamentos:

- Mecânica orbital e gravitação N-body
- Relatividade geral (métricas de Schwarzschild e Kerr)
- Integradores simpléticos (Leapfrog, Yoshida 4ª ordem)
- Correções pós-Newtonianas (1PN)
- Constantes astronômicas IAU 2015

---

## Plataformas e APIs Gráficas (Troca em Runtime)

Todos os backends suportados pela plataforma são compilados juntos no binário. O usuário pode alternar a API gráfica nas configurações do jogo em runtime.

| Plataforma      | Windowing | GPU Backends Suportados (Troca em Runtime) |
|-----------------|-----------|-------------------------------------------|
| Linux           | Wayland   | Vulkan (primário), OpenGL (fallback)      |
| Windows         | Win32     | Direct3D 12 (primário), Vulkan, Direct3D 11, OpenGL |

---

## Scripting

Game scripting via **LuaJIT** para extensibilidade de cenários e lógica de jogo.

---

## Compilação

### Requisitos

- **Compilador**: Clang (recomendado) — C11
- **Build system**: CMake 3.20+
- **Vulkan SDK** (Linux)
- **Wayland dev libs** (Linux): `wayland-client`, `wayland-protocols`
- **FreeType2** + **Fontconfig** (Linux, para UI)

### Build rápido

```bash
# Linux (Wayland + Vulkan)
make linux

# Windows (Win32 + D3D12)
make windows

# Release otimizado
make release
```

### CMake direto

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel $(nproc)
```

### Testes

```bash
make test
```

---

## Estrutura de Módulos (CMake)

| Target              | Alias             | Tipo    | Dependências                    |
|---------------------|-------------------|---------|---------------------------------|
| `ri_foundation`     | Riemenn::Foundation | STATIC | pthread                         |
| `ri_math`           | Riemenn::Math     | STATIC  | ri_foundation, m                |
| `ri_platform`       | Riemenn::Platform | STATIC  | ri_foundation, wayland/win32    |
| `ri_rhi`            | Riemenn::RHI      | STATIC  | ri_foundation, ri_platform, Vulkan |
| `ri_ecs`            | Riemenn::ECS      | STATIC  | ri_foundation, ri_math          |
| `ri_physics`        | Riemenn::Physics  | STATIC  | ri_foundation, ri_math          |
| `ri_assets`         | Riemenn::Assets   | STATIC  | ri_foundation                   |
| `ri_geometry`       | Riemenn::Geometry | STATIC  | ri_math                         |
| `ri_render`         | Riemenn::Render   | STATIC  | ri_rhi, ri_math                 |
| `ri_scene`          | Riemenn::Scene    | STATIC  | ri_ecs                          |
| `ri_ui`             | Riemenn::UI       | STATIC  | ri_rhi, ri_platform, Fontconfig, FreeType2 |
| `ri_engine`         | Riemenn::Engine   | INTERFACE | todos os módulos acima        |
| `blackhole_sim`     | —                 | EXE     | ri_engine                       |

---

## Licença

MIT — veja [LICENSE](LICENSE).
