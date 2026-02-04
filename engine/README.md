# Engine / Motor

Responsabilidade: núcleo determinístico da engine.
Contém física, simulação, sistemas, gerenciamento de recursos, execução de shaders (lógico, não API).
Define o que acontece, não como é apresentado.
Não contém UI, janelas, input direto, nem chamadas gráficas de baixo nível.
Não depende de plataforma nem de API gráfica concreta.