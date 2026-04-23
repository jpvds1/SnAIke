from enum import Enum, auto
import pygame

from ui.renderer import Renderer

FRAME_RATE = 20
FRAME_TIME = 1.0 / FRAME_RATE
SCALE_RATIO = 1
SCREEN_WIDTH = 500
SCREEN_HEIGHT = 500

class State(Enum):
    MENU     = auto()
    PLAYING  = auto() # Human or AI player
    TRAINING = auto()

def initialize_system():
    pygame.init()
    screen = pygame.display.set_mode((SCREEN_WIDTH * SCALE_RATIO, SCREEN_HEIGHT * SCALE_RATIO))
    pygame.display.set_caption("SnAIke")
    clock = pygame.time.Clock()

    renderer = Renderer(screen, clock)
    state = State.MENU
    return screen, clock, renderer, state

def shutdown():
    pygame.quit()

def state_machine(state, renderer):
    running = True
    while(running):
        config = renderer.render_menu()

        if config.get("QUIT", False):
            running = False
            continue

def main():
    screen, clock, renderer, state = initialize_system()
    state_machine(state, renderer)
    shutdown()

if __name__ == "__main__":
    main()