Compile com cmake (recomendado) ou rode o wrapper Make (Menos configuravel.)


## Setup 



```bash
# 1. Configurar (Auto-Detect)
cmake -S . -B build

# 2. Compilar
cmake --build build --parallel $(nproc)
```

### Overrides Manuais

exemplo: Forçar Wayland + Vulkan:
```bash
cmake -S . -B build -DPLATFORM=LINUX_WAYLAND -DGPU_API=vulkan
```

Exemplo: Forçar Windows + DX11:
```bash
cmake -S . -B build -DPLATFORM=WINDOWS -DGPU_API=dx11
```


CMake detecta automaticamente a plataforma e a API grafica recomendada, se você quiser usar outra API, rode o cmake (ou wrapper make) com a flag especifica (Tipo, quando optar usar opengl ao invés do padrão vulkan. )


# Plataformas e APIs graficas:

  linux-x11       **(Default: vulkan | Opções: opengl**)

  linux-wayland   **(Default: vulkan | Opções: opengl**)

  fbsd-x11        **(Default: vulkan | Opções: opengl**)

  fbsd-wayland    **(Default: vulkan | Opções: opengl**)

  windows         **(Default: dx11   | Opções: vulkan, opengl**)

  mac             **(Default: metal  | Apenas Apple Silicon**)

  mac_x86         **(Default: metal  | Intel - Suporta, porém deprecated: opengl**)

  navigator       **(Default: webgpu | Nenhuma outra opção**)




Recomendado usar clang-llvm ou apple-clang (o melhor em apple). Não foi testado em outros compiladores e pode dar problemas com flags!

# Riemenn-Engine
