from enum import Enum, auto
import pygame

BUTTON_COLOR = (209, 0, 0)
BUTTON_HOVER_COLOR = (255, 0, 0)
SELECTOR_COLOR = (0, 0, 150)

class MenuState(Enum):
    MAIN             = auto()
    PLAY_SELECT      = auto()
    ALGORITHM_SELECT = auto()
    TRAIN_CONFIG     = auto()
    QUIT             = auto()

class Renderer:
    def __init__(self, screen, clock, u_height, u_width, scale_ratio, input_handler):
        self.screen = screen
        self.clock = clock
        self.height = u_height * scale_ratio
        self.width = u_width * scale_ratio
        self.input_handler = input_handler

        self.menu_state = MenuState.MAIN

        self.font = pygame.font.Font(None, 10 * scale_ratio)

        self.controllers = ["Human", "Genetic Algorithm", "NEAT", "Q-Learning"]
        self.play_selector = Selector(self.controllers)

    # -------------------------------------------
    # Rendering Helpers
    # -------------------------------------------

    def draw_button(self, text, x, y, width, height, mouse_click, mouse_pos):
        if x < mouse_pos[0] < x + width and y < mouse_pos[1] < y + height:
            pygame.draw.rect(self.screen, BUTTON_HOVER_COLOR, (x, y, width, height))
            if mouse_click:
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

    def draw_selector(self, selector, x, y, width, height, mouse_click, mouse_pos):
        arrow_w = height
        option_w = width - 2 * arrow_w
        left_x = x - width // 2
        opt_x = left_x + arrow_w
        right_x = opt_x + option_w
        top_y = y - height // 2

        # Left Arrow
        left_rect = pygame.Rect(left_x, top_y, arrow_w, height)
        left_hovered = left_rect.collidepoint(mouse_pos)
        pygame.draw.rect(self.screen, BUTTON_HOVER_COLOR if left_hovered else BUTTON_COLOR, left_rect)
        left_surf = self.font.render("<", True, "black")
        self.screen.blit(left_surf, left_surf.get_rect(center=left_rect.center))
        if left_hovered and mouse_click:
            selector.prev()

        # Label
        opt_rect = pygame.Rect(opt_x, top_y, option_w, height)
        pygame.draw.rect(self.screen, SELECTOR_COLOR, opt_rect)
        opt_surf = self.font.render(selector.selected, True, "black")
        self.screen.blit(opt_surf, opt_surf.get_rect(center=opt_rect.center))
        
        # Right Arrow
        right_rect = pygame.Rect(right_x, top_y, arrow_w, height)
        right_hovered = right_rect.collidepoint(mouse_pos)
        pygame.draw.rect(self.screen, BUTTON_HOVER_COLOR if right_hovered else BUTTON_COLOR, right_rect)
        right_surf = self.font.render(">", True, "black")
        self.screen.blit(right_surf, right_surf.get_rect(center=right_rect.center))
        if right_hovered and mouse_click:
            selector.next()


    # -------------------------------------------
    # Menu Rendering
    # -------------------------------------------

    def render_menu_main(self, actions):
        self.screen.fill("white")

        self.draw_text("SnAIke", self.width * 0.5, self.height * 0.3)

        if self.draw_button("Play", self.width * 0.25, self.height * 0.35, self.width * 0.5, self.height * 0.12, actions["MOUSE_CLICK"], actions["MOUSE_POS"]):
            self.menu_state = MenuState.PLAY_SELECT
            return

        if self.draw_button("Train", self.width * 0.25, self.height * 0.5, self.width * 0.5, self.height * 0.12, actions["MOUSE_CLICK"], actions["MOUSE_POS"]):
            self.menu_state = MenuState.ALGORITHM_SELECT
            return

        if self.draw_button("Quit", self.width * 0.25, self.height * 0.65, self.width * 0.5, self.height * 0.12, actions["MOUSE_CLICK"], actions["MOUSE_POS"]):
            self.menu_state = MenuState.QUIT

    def render_menu_play_sel(self, actions):
        self.screen.fill("white")

        self.draw_selector(
            self.play_selector,
            self.width * 0.5, self.height * 0.2,   # center
            self.width * 0.5, self.height * 0.1,   # width, height
            actions["MOUSE_CLICK"], actions["MOUSE_POS"]
        )

        if self.draw_button("Play", self.width * 0.25, self.height * 0.35, self.width * 0.5, self.height * 0.12, actions["MOUSE_CLICK"], actions["MOUSE_POS"]):
            return {"controller": self.play_selector.selected}

        if self.draw_button("Back", self.width * 0.25, self.height * 0.5, self.width * 0.5, self.height * 0.12, actions["MOUSE_CLICK"], actions["MOUSE_POS"]):
            self.menu_state = MenuState.MAIN
            return

    def render_menu_algo_sel(self):
        pass

    def render_menu_train_conf(self):
        pass

    def render_menu(self) -> dict:
        choosing = True
        while(choosing):
            actions = self.input_handler.get_actions()
            if actions["QUIT"] or self.menu_state == MenuState.QUIT:
                return {"QUIT": True}

            if actions["BACK"]:
                if self.menu_state == MenuState.MAIN:
                    return {"QUIT": True}
                else:
                    self.menu_state = MenuState.MAIN
            
            elif self.menu_state == MenuState.MAIN:
                self.render_menu_main(actions)
            elif self.menu_state == MenuState.PLAY_SELECT:
                result = self.render_menu_play_sel(actions)
                if result:
                    print(result)
                    return result
            elif self.menu_state == MenuState.ALGORITHM_SELECT:
                self.render_menu_algo_sel(actions)
            elif self.menu_state == MenuState.TRAIN_CONFIG:
                self.render_menu_train_conf(actions)

            pygame.display.flip()
            self.clock.tick(60)


class Selector:
    def __init__(self, options: list):
        self.options = options
        self.index = 0

    @property
    def selected(self):
        return self.options[self.index]

    def prev(self):
        self.index = (self.index - 1) % len(self.options)

    def next(self):
        self.index = (self.index + 1) % len(self.options)