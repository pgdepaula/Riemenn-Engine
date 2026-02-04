/**
 * @file cocoa.mm
 * @brief Implementação Cocoa/AppKit para macOS
 *
 * Olha, eu não queria estar aqui. Ninguém quer lidar com Objective-C.
 * Mas a Apple, na sua infinita sabedoria, decidiu que todos devemos
 * sofrer juntos. Então aqui estamos, misturando C++ com colchetes
 * que mais parecem syntax error.
 *
 * Backend de plataforma usando Objective-C++ "moderno" (aspas porque
 * nada que usa @ pra tudo pode ser considerado moderno). Usa ARC
 * porque ninguém merece fazer retain/release manual em 2026.
 *
 * Estrutura (prepare-se para a bagunça):
 * - BHSAppDelegate: Gerencia ciclo de vida (aka: faz NSApp funcionar)
 * - BHSWindow: Subclasse porque NSWindow é teimoso demais
 * - BHSView: Onde a mágica acontece (e os bugs também)
 * - bhs_platform_impl: Estado global, sim, eu sei, cala a boca
 * - bhs_window_impl: Estado por janela (esse é ok)
 *
 * Invariantes:
 * - ARC gerencia ObjC, C é na mão (óbvio, né?)
 * - UI só na main thread ou sofra as consequências
 * - Se crashar, provavelmente é culpa da Apple (mentira, é minha)
 *
 * Resumindo: Eu não sou o melhor programador dessa linguagem bosta
 * E eu também não tenho um mac, apenas muito tempo livre kkkkkkkk
 */

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <cstdlib>
#include <cstring>
#include <deque>

/* Nossa interface pública - a única coisa limpa nesse arquivo */
extern "C" {
#include "ux/platform/platform.h"
}

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================
 */

@class BHSAppDelegate;
@class BHSWindow;
@class BHSView;
@class BHSWindowDelegate;

/* ============================================================================
 * ESTRUTURAS INTERNAS
 * ============================================================================
 */

/**
 * Estado global da plataforma Cocoa.
 *
 * "Estado global é evil" - Todo mundo
 * "Foda-se, NSApplication.sharedApplication existe" - Apple
 *
 * Se você está lendo isso e pensando "mas por quê?", bem-vindo
 * ao desenvolvimento macOS. O singleton é obrigatório. Reclame
 * com Tim Cook, não comigo.
 */
struct bhs_platform_impl {
  BHSAppDelegate *app_delegate; /* O chefão que manda em tudo */
  bool initialized;
  bool should_quit;
};

/**
 * Estado por janela.
 *
 * Frankenstein de ObjC com C. Não me julga. Você faria
 * a mesma coisa se tivesse que fazer FFI com Cocoa.
 *
 * Pelo menos funciona. (eu ashokk).
 */
struct bhs_window_impl {
  BHSWindow *native_window; /* A janela de verdade */
  BHSView *content_view;
  BHSWindowDelegate *delegate;
  CAMetalLayer *metal_layer;

  /* Estado da janela */
  bool should_close;
  int32_t width;
  int32_t height;
  int32_t fb_width; /* Framebuffer (pode ser diferente em Retina) */
  int32_t fb_height;

  /* Fila de eventos (estilo poll) */
  std::deque<struct bhs_event> event_queue;

  /* Callback (pra quem gosta de inversão de controle) */
  bhs_event_callback_fn event_callback;
  void *callback_userdata; /* Ponteiro mágico do usuário */
}; /* fim dessa struct gigante, ufa */

/* ============================================================================
 * HELPERS - Vulgo: gambiarras necessárias pra sobreviver
 * ============================================================================
 */

/**
 * Obtém timestamp em nanosegundos.
 * Usa mach_absolute_time porque a Apple decidiu que APIs POSIX são muito
 * fáceis.
 */
static uint64_t bhs_cocoa_timestamp_ns(void) {
  /*
   * Timebase info é cached porque chamar mach_timebase_info
   * toda hora seria burrice. Não que a Apple ligue pra performance...
   */
  static mach_timebase_info_data_t timebase = {0};
  if (timebase.denom == 0) {
    mach_timebase_info(&timebase);
  }
  uint64_t ticks = mach_absolute_time();
  return ticks * timebase.numer / timebase.denom;
}

/**
 * Converte modificadores do NSEvent para nosso formato.
 */
static uint32_t bhs_cocoa_mods_from_nsevent(NSEvent *event) {
  NSEventModifierFlags flags = [event modifierFlags];
  uint32_t mods = BHS_MOD_NONE;

  if (flags & NSEventModifierFlagShift)
    mods |= BHS_MOD_SHIFT;
  if (flags & NSEventModifierFlagControl)
    mods |= BHS_MOD_CTRL;
  if (flags & NSEventModifierFlagOption)
    mods |= BHS_MOD_ALT;
  if (flags & NSEventModifierFlagCommand)
    mods |= BHS_MOD_SUPER;
  if (flags & NSEventModifierFlagCapsLock)
    mods |= BHS_MOD_CAPS;

  return mods;
}

/**
 * Converte botão do mouse do NSEvent para nosso enum.
 */
static enum bhs_mouse_button bhs_cocoa_mouse_button(NSEvent *event) {
  NSInteger button = [event buttonNumber];
  switch (button) {
  case 0:
    return BHS_MOUSE_LEFT;
  case 1:
    return BHS_MOUSE_RIGHT;
  case 2:
    return BHS_MOUSE_MIDDLE;
  case 3:
    return BHS_MOUSE_EXTRA1;
  case 4:
    return BHS_MOUSE_EXTRA2;
  default:
    return BHS_MOUSE_LEFT;
  }
}

/**
 * Enfileira evento na janela.
 *
 * Dupla personalidade: chama callback E enfileira. Porque
 * ninguém consegue decidir qual estilo de API prefere.
 */
static void bhs_cocoa_push_event(struct bhs_window_impl *win,
                                 const struct bhs_event *event) {
  if (!win)
    return;

  /* Callback primeiro, porque aparentemente precisa */
  if (win->event_callback) {
    win->event_callback((bhs_window_t)win, event, win->callback_userdata);
  }

  /*
   * Joga na fila também, vai que alguém usa poll().
   * Sim, é redundante. Não, não me importo.
   */
  win->event_queue.push_back(*event);

  /*
   * Limite de sanidade: se o programa não consome eventos,
   * problema dele. Não vou estourar memória por preguiça alheia.
   */
  while (win->event_queue.size() > 1024) {
    win->event_queue.pop_front();
  }
}

/* ============================================================================
 * BHSVIEW - A view que faz tudo porque NSView é preguiçosa
 *
 * Sério, você precisa subclassear pra qualquer coisa. Quer Metal?
 * Subclasse. Quer eventos de teclado? Subclasse. Quer respirar? Subclasse.
 * ============================================================================
 */

@interface BHSView : NSView
@property(nonatomic, assign) struct bhs_window_impl *impl;
@end

@implementation BHSView

/**
 * "Ei AppKit, queremos Metal, não aquele OpenGL jurássico."
 * Sem isso, você ganha um CALayer genérico. Inútil.
 */
+ (Class)layerClass {
  return [CAMetalLayer class];
}

- (CALayer *)makeBackingLayer {
  /*
   * Cria o layer Metal. Se MTLCreateSystemDefaultDevice retornar
   * nil, você está num Mac muito antigo. Compra um novo.
   */
  CAMetalLayer *layer = [CAMetalLayer layer];
  layer.device = MTLCreateSystemDefaultDevice();
  layer.pixelFormat =
      MTLPixelFormatBGRA8Unorm_sRGB; /* sRGB porque somos civilizados */
  layer.framebufferOnly = YES;       /* Performance > flexibilidade */
  return layer;
}

- (BOOL)wantsLayer {
  return YES;
}

- (BOOL)wantsUpdateLayer {
  return YES;
}

/**
 * Aceita first responder pra receber eventos de teclado.
 */
- (BOOL)acceptsFirstResponder {
  return YES;
}

/**
 * Aceita clique mesmo sem foco. Porque ter que clicar duas
 * vezes (uma pra focar, outra pra ação) é coisa de psicopata.
 */
- (BOOL)acceptsFirstMouse:(NSEvent *)event {
  return YES;
}

/* ----------------------------------------------------------------------------
 * Eventos de Mouse
 * ----------------------------------------------------------------------------
 */

- (void)mouseDown:(NSEvent *)event {
  if (!_impl)
    return;

  NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
  struct bhs_event ev = {};
  ev.type = BHS_EVENT_MOUSE_DOWN;
  ev.mods = bhs_cocoa_mods_from_nsevent(event);
  ev.timestamp_ns = bhs_cocoa_timestamp_ns();
  ev.mouse_button.x = (int32_t)loc.x;
  ev.mouse_button.y = (int32_t)(_impl->height - loc.y); /* Flip Y */
  ev.mouse_button.button = BHS_MOUSE_LEFT;
  ev.mouse_button.click_count = (int)[event clickCount];

  bhs_cocoa_push_event(_impl, &ev);
}

- (void)mouseUp:(NSEvent *)event {
  if (!_impl)
    return;

  NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
  struct bhs_event ev = {};
  ev.type = BHS_EVENT_MOUSE_UP;
  ev.mods = bhs_cocoa_mods_from_nsevent(event);
  ev.timestamp_ns = bhs_cocoa_timestamp_ns();
  ev.mouse_button.x = (int32_t)loc.x;
  ev.mouse_button.y = (int32_t)(_impl->height - loc.y);
  ev.mouse_button.button = BHS_MOUSE_LEFT;
  ev.mouse_button.click_count = (int)[event clickCount];

  bhs_cocoa_push_event(_impl, &ev);
}

- (void)rightMouseDown:(NSEvent *)event {
  if (!_impl)
    return;

  NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
  struct bhs_event ev = {};
  ev.type = BHS_EVENT_MOUSE_DOWN;
  ev.mods = bhs_cocoa_mods_from_nsevent(event);
  ev.timestamp_ns = bhs_cocoa_timestamp_ns();
  ev.mouse_button.x = (int32_t)loc.x;
  ev.mouse_button.y = (int32_t)(_impl->height - loc.y);
  ev.mouse_button.button = BHS_MOUSE_RIGHT;
  ev.mouse_button.click_count = (int)[event clickCount];

  bhs_cocoa_push_event(_impl, &ev);
}

- (void)rightMouseUp:(NSEvent *)event {
  if (!_impl)
    return;

  NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
  struct bhs_event ev = {};
  ev.type = BHS_EVENT_MOUSE_UP;
  ev.mods = bhs_cocoa_mods_from_nsevent(event);
  ev.timestamp_ns = bhs_cocoa_timestamp_ns();
  ev.mouse_button.x = (int32_t)loc.x;
  ev.mouse_button.y = (int32_t)(_impl->height - loc.y);
  ev.mouse_button.button = BHS_MOUSE_RIGHT;
  ev.mouse_button.click_count = (int)[event clickCount];

  bhs_cocoa_push_event(_impl, &ev);
}

- (void)otherMouseDown:(NSEvent *)event {
  if (!_impl)
    return;

  NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
  struct bhs_event ev = {};
  ev.type = BHS_EVENT_MOUSE_DOWN;
  ev.mods = bhs_cocoa_mods_from_nsevent(event);
  ev.timestamp_ns = bhs_cocoa_timestamp_ns();
  ev.mouse_button.x = (int32_t)loc.x;
  ev.mouse_button.y = (int32_t)(_impl->height - loc.y);
  ev.mouse_button.button = bhs_cocoa_mouse_button(event);
  ev.mouse_button.click_count = (int)[event clickCount];

  bhs_cocoa_push_event(_impl, &ev);
}

- (void)otherMouseUp:(NSEvent *)event {
  if (!_impl)
    return;

  NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
  struct bhs_event ev = {};
  ev.type = BHS_EVENT_MOUSE_UP;
  ev.mods = bhs_cocoa_mods_from_nsevent(event);
  ev.timestamp_ns = bhs_cocoa_timestamp_ns();
  ev.mouse_button.x = (int32_t)loc.x;
  ev.mouse_button.y = (int32_t)(_impl->height - loc.y);
  ev.mouse_button.button = bhs_cocoa_mouse_button(event);
  ev.mouse_button.click_count = (int)[event clickCount];

  bhs_cocoa_push_event(_impl, &ev);
}

- (void)mouseMoved:(NSEvent *)event {
  if (!_impl)
    return;

  NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
  struct bhs_event ev = {};
  ev.type = BHS_EVENT_MOUSE_MOVE;
  ev.mods = bhs_cocoa_mods_from_nsevent(event);
  ev.timestamp_ns = bhs_cocoa_timestamp_ns();
  ev.mouse_move.x = (int32_t)loc.x;
  ev.mouse_move.y = (int32_t)(_impl->height - loc.y);
  ev.mouse_move.dx = (int32_t)[event deltaX];
  ev.mouse_move.dy = (int32_t)[event deltaY];

  bhs_cocoa_push_event(_impl, &ev);
}

- (void)mouseDragged:(NSEvent *)event {
  /* Dragging é basicamente um move durante o clique */
  [self mouseMoved:event];
}

- (void)rightMouseDragged:(NSEvent *)event {
  [self mouseMoved:event];
}

- (void)otherMouseDragged:(NSEvent *)event {
  [self mouseMoved:event];
}

- (void)scrollWheel:(NSEvent *)event {
  if (!_impl)
    return;

  NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
  struct bhs_event ev = {};
  ev.type = BHS_EVENT_MOUSE_SCROLL;
  ev.mods = bhs_cocoa_mods_from_nsevent(event);
  ev.timestamp_ns = bhs_cocoa_timestamp_ns();
  ev.scroll.x = (int32_t)loc.x;
  ev.scroll.y = (int32_t)(_impl->height - loc.y);
  ev.scroll.dx = (float)[event scrollingDeltaX];
  ev.scroll.dy = (float)[event scrollingDeltaY];
  ev.scroll.is_precise = [event hasPreciseScrollingDeltas];

  bhs_cocoa_push_event(_impl, &ev);
}

/* ----------------------------------------------------------------------------
 * Eventos de Teclado
 * ----------------------------------------------------------------------------
 */

- (void)keyDown:(NSEvent *)event {
  if (!_impl)
    return;

  struct bhs_event ev = {};
  ev.type = [event isARepeat] ? BHS_EVENT_KEY_REPEAT : BHS_EVENT_KEY_DOWN;
  ev.mods = bhs_cocoa_mods_from_nsevent(event);
  ev.timestamp_ns = bhs_cocoa_timestamp_ns();
  ev.key.keycode = [event keyCode];
  ev.key.scancode = [event keyCode];

  /* Pega o texto (se for caractere imprimível) */
  NSString *chars = [event characters];
  if (chars && [chars length] > 0) {
    const char *utf8 = [chars UTF8String];
    if (utf8) {
      size_t len = strlen(utf8);
      if (len < sizeof(ev.key.text)) {
        memcpy(ev.key.text, utf8, len);
        ev.key.text[len] = '\0';
      }
    }
  }

  bhs_cocoa_push_event(_impl, &ev);
}

- (void)keyUp:(NSEvent *)event {
  if (!_impl)
    return;

  struct bhs_event ev = {};
  ev.type = BHS_EVENT_KEY_UP;
  ev.mods = bhs_cocoa_mods_from_nsevent(event);
  ev.timestamp_ns = bhs_cocoa_timestamp_ns();
  ev.key.keycode = [event keyCode];
  ev.key.scancode = [event keyCode];

  bhs_cocoa_push_event(_impl, &ev);
}

- (void)flagsChanged:(NSEvent *)event {
  /*
   * A Apple, na sua infinita sabedoria, decidiu que Shift, Ctrl,
   * etc não merecem keyDown/keyUp como teclas normais. Não, elas
   * ganham flagsChanged, que te dá apenas o estado atual.
   *
   * "Qual tecla mudou?" Boa pergunta. Descobre aí comparando bits.
   *
   * Nota: Caps Lock e outros modificadores requerem tracking de estado
   * comparando com as flags anteriores para gerar eventos precisos.
   * System Constraint: Confiamos no gerenciamento de estado global do macOS.
   */
  (void)event; /* shut up, compiler */
}

@end

/* ============================================================================
 * BHSWINDOWDELEGATE - Porque a janela precisa de babá
 *
 * No Cocoa, quase tudo funciona via delegate. A janela acontece
 * algo? Pergunta pro delegate. Vai fechar? Delegate. Redimensionou?
 * Delegate. É o padrão de projeto favorito da Apple.
 * ============================================================================
 */

@interface BHSWindowDelegate : NSObject <NSWindowDelegate>
@property(nonatomic, assign) struct bhs_window_impl *impl;
@end

@implementation BHSWindowDelegate

- (BOOL)windowShouldClose:(NSWindow *)sender {
  (void)sender; /* sim, eu sei qual janela é, obrigado */
  if (!_impl)
    return YES; /* vai embora logo */

  struct bhs_event ev = {};
  ev.type = BHS_EVENT_WINDOW_CLOSE;
  ev.timestamp_ns = bhs_cocoa_timestamp_ns();
  bhs_cocoa_push_event(_impl, &ev);

  /*
   * Retorna NO pra NÃO fechar. A aplicação decide quando morrer.
   * Se retornasse YES, a janela sumiria antes de salvar qualquer coisa.
   */
  _impl->should_close = true;
  return NO;
}

- (void)windowDidResize:(NSNotification *)notification {
  if (!_impl)
    return;

  NSWindow *window = [notification object];
  NSRect frame = [[window contentView] frame];
  NSRect backing = [[window contentView] convertRectToBacking:frame];

  _impl->width = (int32_t)frame.size.width;
  _impl->height = (int32_t)frame.size.height;
  _impl->fb_width = (int32_t)backing.size.width;
  _impl->fb_height = (int32_t)backing.size.height;

  /* Atualiza tamanho do Metal layer */
  if (_impl->metal_layer) {
    _impl->metal_layer.drawableSize =
        CGSizeMake(_impl->fb_width, _impl->fb_height);
  }

  struct bhs_event ev = {};
  ev.type = BHS_EVENT_WINDOW_RESIZE;
  ev.timestamp_ns = bhs_cocoa_timestamp_ns();
  ev.resize.width = _impl->width;
  ev.resize.height = _impl->height;
  bhs_cocoa_push_event(_impl, &ev);
}

- (void)windowDidBecomeKey:(NSNotification *)notification {
  if (!_impl)
    return;

  struct bhs_event ev = {};
  ev.type = BHS_EVENT_WINDOW_FOCUS;
  ev.timestamp_ns = bhs_cocoa_timestamp_ns();
  bhs_cocoa_push_event(_impl, &ev);
}

- (void)windowDidResignKey:(NSNotification *)notification {
  if (!_impl)
    return;

  struct bhs_event ev = {};
  ev.type = BHS_EVENT_WINDOW_BLUR;
  ev.timestamp_ns = bhs_cocoa_timestamp_ns();
  bhs_cocoa_push_event(_impl, &ev);
}

@end

/* ============================================================================
 * BHSWINDOW - Subclasse de NSWindow
 *
 * "Mas por que subclassear se não adiciona nada?"
 * Porque canBecomeKeyWindow retorna NO por padrão em certas
 * configurações. Obrigado Apple, muito intuitivo.
 * ============================================================================
 */

@interface BHSWindow : NSWindow
@end

@implementation BHSWindow
/* A implementação mais longa de "return YES" que você já viu */

/**
 * Sem isso, janelas borderless não recebem foco de teclado.
 * Genial, Apple. Simplesmente genial.
 */
- (BOOL)canBecomeKeyWindow {
  return YES;
}

- (BOOL)canBecomeMainWindow {
  return YES;
}

@end

/* ============================================================================
 * BHSAPPDELEGATE - O CEO de toda essa zona
 *
 * NSApp precisa de um delegate senão fica perdido. Tipo estagiário
 * no primeiro dia.
 * ============================================================================
 */

@interface BHSAppDelegate : NSObject <NSApplicationDelegate>
@property(nonatomic, assign) struct bhs_platform_impl *platform;
@end

@implementation BHSAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
  (void)notification; /* não preciso de você */
  /*
   * "Ei macOS, eu existo e quero atenção!"
   * Sem isso, a janela aparece atrás do Finder. Muito útil.
   */
  [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:
    (NSApplication *)sender {
  (void)sender;
  /*
   * "Não morra só porque a janela fechou!"
   * Comportamento padrão da Apple é matar o app. Que simpático.
   */
  return NO;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:
    (NSApplication *)sender {
  (void)sender;
  if (_platform) {
    _platform->should_quit = true;
  }
  /*
   * "Calma aí, deixa eu terminar o que tô fazendo!"
   * NSTerminateCancel = não mata agora, deixa o game loop sair bonito.
   */
  return NSTerminateCancel;
}

@end

/* ============================================================================
 * IMPLEMENTAÇÃO DA API C
 * ============================================================================
 */

extern "C" {

/* --------------------------------------------------------------------------
 * Plataforma
 * -------------------------------------------------------------------------- */

int bhs_platform_init(bhs_platform_t *platform) {
  if (!platform)
    return BHS_PLATFORM_ERR_INVALID;

  /* Aloca estrutura */
  struct bhs_platform_impl *p = new (std::nothrow) bhs_platform_impl;
  if (!p)
    return BHS_PLATFORM_ERR_NOMEM;

  memset(p, 0, sizeof(*p));

  @autoreleasepool {
    /*
     * Se ainda não existe NSApplication, cria uma.
     * Isso acontece quando rodamos sem bundle (linha de comando).
     */
    if (NSApp == nil) {
      [NSApplication sharedApplication];
    }

    /* Cria e configura o delegate */
    BHSAppDelegate *delegate = [[BHSAppDelegate alloc] init];
    delegate.platform = p;
    [NSApp setDelegate:delegate];
    p->app_delegate = delegate;

    /*
     * Define que somos uma aplicação de verdade.
     * Sem isso, não aparecemos no dock e temos problemas de foco.
     */
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    /*
     * Faz o setup inicial da aplicação.
     * finishLaunching inicializa menus e outras coisas.
     */
    [NSApp finishLaunching];
  }

  p->initialized = true;
  *platform = p;
  return BHS_PLATFORM_OK;
}

void bhs_platform_shutdown(bhs_platform_t platform) {
  if (!platform)
    return;

  struct bhs_platform_impl *p = platform;

  @autoreleasepool {
    if (p->app_delegate) {
      [NSApp setDelegate:nil];
      p->app_delegate = nil;
    }
  }

  delete p;
}

void bhs_platform_poll_events(bhs_platform_t platform) {
  if (!platform)
    return;

  @autoreleasepool {
    NSEvent *event;
    while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                       untilDate:nil
                                          inMode:NSDefaultRunLoopMode
                                         dequeue:YES])) {
      [NSApp sendEvent:event];
      [NSApp updateWindows];
    }
  }
}

void bhs_platform_wait_events(bhs_platform_t platform) {
  if (!platform)
    return;

  @autoreleasepool {
    /* Espera indefinidamente pelo próximo evento */
    NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                        untilDate:[NSDate distantFuture]
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:YES];
    if (event) {
      [NSApp sendEvent:event];
      [NSApp updateWindows];
    }
  }
}

/* --------------------------------------------------------------------------
 * Janela
 * -------------------------------------------------------------------------- */

int bhs_window_create(bhs_platform_t platform,
                      const struct bhs_window_config *config,
                      bhs_window_t *window) {
  if (!platform || !config || !window)
    return BHS_PLATFORM_ERR_INVALID;

  @autoreleasepool {
    /* Aloca nossa estrutura */
    struct bhs_window_impl *win = new (std::nothrow) bhs_window_impl;
    if (!win)
      return BHS_PLATFORM_ERR_NOMEM;

    memset(win, 0, sizeof(*win));

    /* Configura estilo da janela baseado nas flags */
    NSWindowStyleMask style = NSWindowStyleMaskTitled |
                              NSWindowStyleMaskClosable |
                              NSWindowStyleMaskMiniaturizable;

    if (config->flags & BHS_WINDOW_RESIZABLE) {
      style |= NSWindowStyleMaskResizable;
    }
    if (config->flags & BHS_WINDOW_BORDERLESS) {
      style = NSWindowStyleMaskBorderless;
    }

    /* Calcula posição */
    CGFloat x, y;
    if (config->x == BHS_WINDOW_POS_CENTERED ||
        config->x == BHS_WINDOW_POS_UNDEFINED) {
      /* Centraliza horizontalmente */
      NSScreen *screen = [NSScreen mainScreen];
      x = (screen.frame.size.width - config->width) / 2;
    } else {
      x = config->x;
    }

    if (config->y == BHS_WINDOW_POS_CENTERED ||
        config->y == BHS_WINDOW_POS_UNDEFINED) {
      /* Centraliza verticalmente */
      NSScreen *screen = [NSScreen mainScreen];
      y = (screen.frame.size.height - config->height) / 2;
    } else {
      y = config->y;
    }

    NSRect frame = NSMakeRect(x, y, config->width, config->height);

    /* Cria a janela */
    BHSWindow *native =
        [[BHSWindow alloc] initWithContentRect:frame
                                     styleMask:style
                                       backing:NSBackingStoreBuffered
                                         defer:NO];

    if (!native) {
      delete win;
      return BHS_PLATFORM_ERR_WINDOW;
    }

    /* Configura título */
    if (config->title) {
      [native setTitle:@(config->title)];
    }

    /* Cria nossa view customizada */
    BHSView *view = [[BHSView alloc] initWithFrame:frame];
    view.impl = win;
    [native setContentView:view];

    /* Configura delegate da janela */
    BHSWindowDelegate *delegate = [[BHSWindowDelegate alloc] init];
    delegate.impl = win;
    [native setDelegate:delegate];

    /* Aceita eventos de mouse mesmo fora da janela */
    [native setAcceptsMouseMovedEvents:YES];

    /* Habilita HiDPI se solicitado */
    if (config->flags & BHS_WINDOW_HIGH_DPI) {
      [view setWantsBestResolutionOpenGLSurface:YES];
    }

    /* Pega o Metal layer que foi criado pela view */
    CAMetalLayer *layer = (CAMetalLayer *)[view layer];

    /* Armazena tudo na nossa estrutura */
    win->native_window = native;
    win->content_view = view;
    win->delegate = delegate;
    win->metal_layer = layer;
    win->width = config->width;
    win->height = config->height;

    /* Calcula tamanho do framebuffer (importante pra Retina) */
    NSRect backing = [view convertRectToBacking:frame];
    win->fb_width = (int32_t)backing.size.width;
    win->fb_height = (int32_t)backing.size.height;

    if (layer) {
      layer.drawableSize = CGSizeMake(win->fb_width, win->fb_height);
    }

    /* Mostra a janela (a menos que BHS_WINDOW_HIDDEN) */
    if (!(config->flags & BHS_WINDOW_HIDDEN)) {
      [native makeKeyAndOrderFront:nil];
    }

    *window = win;
    return BHS_PLATFORM_OK;
  }
}

void bhs_window_destroy(bhs_window_t window) {
  if (!window)
    return;

  struct bhs_window_impl *win = window;

  @autoreleasepool {
    if (win->native_window) {
      [win->native_window setDelegate:nil];
      [win->native_window close];
    }
    win->native_window = nil;
    win->content_view = nil;
    win->delegate = nil;
    win->metal_layer = nil;
  }

  delete win;
}

void bhs_window_show(bhs_window_t window) {
  if (!window)
    return;

  @autoreleasepool {
    [window->native_window makeKeyAndOrderFront:nil];
  }
}

void bhs_window_hide(bhs_window_t window) {
  if (!window)
    return;

  @autoreleasepool {
    [window->native_window orderOut:nil];
  }
}

void bhs_window_set_title(bhs_window_t window, const char *title) {
  if (!window || !title)
    return;

  @autoreleasepool {
    [window->native_window setTitle:@(title)];
  }
}

void bhs_window_get_size(bhs_window_t window, int32_t *width, int32_t *height) {
  if (!window)
    return;

  if (width)
    *width = window->width;
  if (height)
    *height = window->height;
}

void bhs_window_get_framebuffer_size(bhs_window_t window, int32_t *width,
                                     int32_t *height) {
  if (!window)
    return;

  if (width)
    *width = window->fb_width;
  if (height)
    *height = window->fb_height;
}

bool bhs_window_should_close(bhs_window_t window) {
  if (!window)
    return true;

  return window->should_close;
}

void bhs_window_set_should_close(bhs_window_t window, bool should_close) {
  if (!window)
    return;

  window->should_close = should_close;
}

void bhs_window_set_event_callback(bhs_window_t window,
                                   bhs_event_callback_fn callback,
                                   void *userdata) {
  if (!window)
    return;

  window->event_callback = callback;
  window->callback_userdata = userdata;
}

bool bhs_window_next_event(bhs_window_t window, struct bhs_event *event) {
  if (!window || !event)
    return false;

  if (window->event_queue.empty())
    return false;

  *event = window->event_queue.front();
  window->event_queue.pop_front();
  return true;
}

void *bhs_window_get_native_handle(bhs_window_t window) {
  if (!window)
    return nullptr;

  return (__bridge void *)window->native_window;
}

void *bhs_window_get_native_layer(bhs_window_t window) {
  if (!window)
    return nullptr;

  return (__bridge void *)window->metal_layer;
}

} /* extern "C" */

// A apple é aquela criança mimada que quer fazer tudo do jeito dela.