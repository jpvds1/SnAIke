from enum import Enum, auto
import pygame

BUTTON_COLOR = (209, 0, 0)
BUTTON_HOVER_COLOR = (255, 0, 0)

class MenuState(Enum):
    MAIN             = auto()
    PLAY_SELECT      = auto()
    ALGORITHM_SELECT = auto()
    TRAIN_CONFIG     = auto()
    QUIT             = auto()

class Renderer:
    def __init__(self, screen, clock, u_height, u_width, scale_ratio):
        self.screen = screen
        self.clock = clock
        self.height = u_height * scale_ratio
        self.width = u_width * scale_ratio

        self.menu_state = MenuState.MAIN

        self.font = pygame.font.Font(None, 10 * scale_ratio)

    # -------------------------------------------
    # Rendering Helpers
    # -------------------------------------------

    def wait_for_mouse_release(self):
        waiting = True
        while waiting:
            for event in pygame.event.get():
                if event.type == pygame.MOUSEBUTTONUP:
                    waiting = False

    def draw_button(self, text, x, y, width, height):
        mouse_pos = pygame.mouse.get_pos()
        click = pygame.mouse.get_pressed()

        if x < mouse_pos[0] < x + width and y < mouse_pos[1] < y + height:
            pygame.draw.rect(self.screen, BUTTON_HOVER_COLOR, (x, y, width, height))
            if click[0] == 1:
                self.wait_for_mouse_release()
                print(f"clicked {text}")
                return True
        else:
            pygame.draw.rect(self.screen, BUTTON_COLOR, (x, y, width, height))

        text_surface = self.font.render(text, True, "black")
        text_rect = text_surface.get_rect(center=(x + width // 2, y + height // 2))
        self.screen.blit(text_surface, text_rect)

    def draw_text(self, text, x, y):
        text_surface = self.font.render(text, True, "black")
        text_rect = text_surface.get_rect(center=(x, y))
        self.screen.blit(text_surface, text_rect)


    # -------------------------------------------
    # Menu Rendering
    # -------------------------------------------

    def render_menu_main(self):
        self.screen.fill("white")

        self.draw_text("SnAIke", self.width * 0.5, self.height * 0.3)

        if self.draw_button("Play", self.width * 0.25, self.height * 0.35, self.width * 0.5, self.height * 0.12):
            self.menu_state = MenuState.PLAY_SELECT
            return

        if self.draw_button("Train", self.width * 0.25, self.height * 0.5, self.width * 0.5, self.height * 0.12):
            self.menu_state = MenuState.ALGORITHM_SELECT
            return

        if self.draw_button("Quit", self.width * 0.25, self.height * 0.65, self.width * 0.5, self.height * 0.12):
            self.menu_state = MenuState.QUIT

    def render_menu_play_sel(self):
        pass

    def render_menu_algo_sel(self):
        pass

    def render_menu_train_conf(self):
        pass

    def render_menu(self) -> dict:
        choosing = True
        while(choosing):
            if self.menu_state == MenuState.QUIT:
                return {"QUIT": True}
            elif self.menu_state == MenuState.MAIN:
                self.render_menu_main()
            elif self.menu_state == MenuState.PLAY_SELECT:
                self.render_menu_play_sel()
            elif self.menu_state == MenuState.ALGORITHM_SELECT:
                self.render_menu_algo_sel()
            elif self.menu_state == MenuState.TRAIN_CONFIG:
                self.render_menu_train_conf()

            pygame.display.flip()
            self.clock.tick(60)