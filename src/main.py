import pygame

from ui.renderer import Renderer

FRAME_RATE = 20
FRAME_TIME = 1.0 / FRAME_RATE
SCALE_RATIO = 4
SCREEN_WIDTH = 200
SCREEN_HEIGHT = 200

def initialize_system():
    pygame.init()
    screen = pygame.display.set_mode((SCREEN_WIDTH * SCALE_RATIO, SCREEN_HEIGHT * SCALE_RATIO))
    pygame.display.set_caption("SnAIke")
    clock = pygame.time.Clock()

    renderer = Renderer(screen, clock, SCREEN_HEIGHT, SCREEN_WIDTH, SCALE_RATIO)    
    return screen, clock, renderer

def shutdown():
    pygame.quit()

def main_loop(renderer):
    running = True
    while(running):
        config = renderer.render_menu()

        if config.get("QUIT", False):
            running = False
            continue

def main():
    screen, clock, renderer = initialize_system()
    main_loop(renderer)
    shutdown()

if __name__ == "__main__":
    main()