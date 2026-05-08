from enum import Enum, auto
import pygame

# Shared palette
COLOR_BG         = ( 18,  18,  18)
COLOR_SURFACE    = ( 30,  30,  30)
COLOR_BORDER     = ( 50,  50,  50)

COLOR_TEXT       = (220, 220, 220)
COLOR_TEXT_MUTED = (120, 120, 120)


COLOR_PRIMARY    = ( 30, 160,  60)
COLOR_PRIMARY_HO = ( 80, 255, 120)
COLOR_DANGER     = (160,  40,  40)
COLOR_DANGER_HO  = (220,  50,  50)
COLOR_NEUTRAL    = ( 50,  50,  50)
COLOR_NEUTRAL_HO = ( 75,  75,  75)

# Game palette

COLOR_GRID       = ( 38,  38,  38)
COLOR_SNAKE_HEAD = ( 80, 255, 120)
COLOR_SNAKE_BODY = ( 30, 160,  60)
COLOR_APPLE      = (220,  50,  50)
COLOR_HUD        = (220, 220, 220)
 
CELL_PAD   = 2
BTN_RADIUS = 8

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
        self.scale_ratio = scale_ratio
        self.input_handler = input_handler

        self.menu_state = MenuState.MAIN

        self.font       = pygame.font.Font(None, 10 * scale_ratio)
        self.small_font = pygame.font.Font(None, 6 * scale_ratio)

        self.controllers = ["Human", "Genetic Algorithm", "NEAT", "Q-Learning"]
        self.play_selector = Selector(self.controllers)

    # -------------------------------------------
    # Rendering Helpers
    # -------------------------------------------

    def draw_button(self, text, x, y, width, height, mouse_click, mouse_pos, variant = "primary"):
        hovered = x < mouse_pos[0] < x + width and y < mouse_pos[1] < y + height

        colors = {
            "primary": (COLOR_PRIMARY_HO if hovered else COLOR_PRIMARY),
            "secondary": (COLOR_NEUTRAL_HO if hovered else COLOR_NEUTRAL),
            "danger": (COLOR_DANGER_HO if hovered else COLOR_DANGER)
        }
        color = colors.get(variant, COLOR_PRIMARY)

        pygame.draw.rect(self.screen, color, (x, y, width, height), border_radius=BTN_RADIUS)
        surf = self.font.render(text, True, COLOR_TEXT)
        self.screen.blit(surf, surf.get_rect(center=(x + width // 2, y + height // 2)))

        return bool(hovered and mouse_click)

    def draw_text(self, text, x, y, font=None, color=COLOR_TEXT):
        f = font or self.font
        surf = f.render(text, True, color)
        self.screen.blit(surf, surf.get_rect(center=(x, y)))

    def draw_selector(self, selector, x, y, width, height, mouse_click, mouse_pos):
        arrow_w = height
        option_w = width - 2 * arrow_w
        left_x = x - width // 2
        opt_x = left_x + arrow_w
        right_x = opt_x + option_w
        top_y = y - height // 2

        # Left Arrow
        left_rect = pygame.Rect(left_x, top_y, arrow_w, height)
        left_rect_fill = pygame.Rect(left_x + arrow_w - BTN_RADIUS, top_y, BTN_RADIUS * 2, height)
        left_hovered = left_rect.collidepoint(mouse_pos)
        pygame.draw.rect(self.screen, COLOR_NEUTRAL_HO if left_hovered else COLOR_NEUTRAL, left_rect, border_radius=BTN_RADIUS)
        pygame.draw.rect(self.screen, COLOR_NEUTRAL_HO if left_hovered else COLOR_NEUTRAL, left_rect_fill)
        left_surf = self.font.render("<", True, COLOR_TEXT)
        self.screen.blit(left_surf, left_surf.get_rect(center=left_rect.center))
        if left_hovered and mouse_click:
            selector.prev()
        
        # Right Arrow
        right_rect = pygame.Rect(right_x, top_y, arrow_w, height)
        right_rect_fill = pygame.Rect(right_x - BTN_RADIUS, top_y, BTN_RADIUS * 2, height)
        right_hovered = right_rect.collidepoint(mouse_pos)
        pygame.draw.rect(self.screen, COLOR_NEUTRAL_HO if right_hovered else COLOR_NEUTRAL, right_rect, border_radius=BTN_RADIUS)
        pygame.draw.rect(self.screen, COLOR_NEUTRAL_HO if right_hovered else COLOR_NEUTRAL, right_rect_fill)
        right_surf = self.font.render(">", True, COLOR_TEXT)
        self.screen.blit(right_surf, right_surf.get_rect(center=right_rect.center))
        if right_hovered and mouse_click:
            selector.next()

        # Label
        opt_rect = pygame.Rect(opt_x, top_y, option_w, height)
        pygame.draw.rect(self.screen, COLOR_SURFACE, opt_rect, border_radius=BTN_RADIUS)
        pygame.draw.rect(self.screen, COLOR_BORDER, opt_rect, 1, border_radius=BTN_RADIUS)
        opt_surf = self.font.render(selector.selected, True, COLOR_TEXT)
        self.screen.blit(opt_surf, opt_surf.get_rect(center=opt_rect.center))

    # -------------------------------------------
    # Game Rendering
    # -------------------------------------------

    def render_game(self, state: dict) -> None:
        board_w, board_h = state["board_size"]
        cell_w = self.width // board_w
        cell_h = self.height // board_h

        self._draw_background(board_w, board_h, cell_w, cell_h)
        self._draw_apple(state["apple"], cell_w, cell_h)
        self._draw_snake(state["snake_positions"], cell_w, cell_h)
        self._draw_hud(state["score"], state["steps"])

        pygame.display.flip()

    def _draw_background(self, board_w, board_h, cell_w, cell_h):
        self.screen.fill(COLOR_BG)
        for gx in range(board_w):
            for gy in range(board_h):
                rect = pygame.Rect(gx * cell_w, gy * cell_h, cell_w, cell_h)
                pygame.draw.rect(self.screen, COLOR_GRID, rect, 1)

    def _draw_apple(self, apple, cell_w, cell_h):
        if apple is None:
            return
        ax, ay = apple
        rect = pygame.Rect(
            ax * cell_w + CELL_PAD,
            ay * cell_h + CELL_PAD,
            cell_w - CELL_PAD * 2,
            cell_h - CELL_PAD * 2
        )
        pygame.draw.ellipse(self.screen, COLOR_APPLE, rect)

    def _draw_snake(self, positions, cell_w, cell_h):
        for i, (gx, gy) in enumerate(positions):
            color = COLOR_SNAKE_HEAD if i == 0 else COLOR_SNAKE_BODY
            rect = pygame.Rect(
                gx * cell_w + CELL_PAD,
                gy * cell_h + CELL_PAD,
                cell_w - CELL_PAD * 2,
                cell_h - CELL_PAD * 2
            )
            pygame.draw.rect(self.screen, color, rect, border_radius=4)

    def _draw_hud(self, score: int, steps: int):
        surf = self.small_font.render(f"Score: {score} Steps: {steps}", True, COLOR_HUD)
        self.screen.blit(surf, (8,8))

    # -------------------------------------------
    # Menu Rendering
    # -------------------------------------------

    def render_menu_main(self, actions):
        self.screen.fill(COLOR_BG)

        self.draw_text("SnAIke", self.width * 0.5, self.height * 0.28, color=COLOR_PRIMARY_HO)
        self.draw_text("Use arrow keys or WASD to play", self.width * 0.5, self.height * 0.38, font=self.small_font, color=COLOR_TEXT_MUTED)

        cx = self.width * 0.25
        bw = self.width * 0.5
        bh = self.height * 0.1
        gap = self.height * 0.04

        y_play = self.height * 0.47
        y_train = y_play + bh + gap
        y_quit = y_train + bh + gap

        if self.draw_button("Play", cx, y_play, bw, bh, actions["MOUSE_CLICK"], actions["MOUSE_POS"]):
            self.menu_state = MenuState.PLAY_SELECT
            return

        if self.draw_button("Train", cx, y_train, bw, bh, actions["MOUSE_CLICK"], actions["MOUSE_POS"], variant="secondary"):
            self.menu_state = MenuState.ALGORITHM_SELECT
            return

        if self.draw_button("Quit", cx, y_quit, bw, bh, actions["MOUSE_CLICK"], actions["MOUSE_POS"], variant="danger"):
            self.menu_state = MenuState.QUIT

    def render_menu_play_sel(self, actions):
        self.screen.fill(COLOR_BG)

        self.draw_text("Select Controller", self.width * 0.5, self.height * 0.28, color=COLOR_TEXT)
        self.draw_text("choose who plays this session", self.width * 0.5, self.height * 0.36, font=self.small_font, color=COLOR_TEXT_MUTED)

        self.draw_selector(
            self.play_selector,
            self.width * 0.5, self.height * 0.50,
            self.width * 0.55, self.height * 0.1,
            actions["MOUSE_CLICK"], actions["MOUSE_POS"]
        )

        cx = self.width * 0.25
        bw = self.width * 0.5
        bh = self.height * 0.1
        gap = self.height * 0.04

        y_play = self.height * 0.64
        y_back = y_play + bh + gap


        if self.draw_button("Play", cx, y_play, bw, bh, actions["MOUSE_CLICK"], actions["MOUSE_POS"]):
            return {"controller": self.play_selector.selected}

        if self.draw_button("Back", cx, y_back, bw, bh, actions["MOUSE_CLICK"], actions["MOUSE_POS"], variant="secondary"):
            self.menu_state = MenuState.MAIN
            return

    def render_menu_algo_sel(self):
        pass

    def render_menu_train_conf(self):
        pass

    def render_menu(self) -> dict:
        choosing = True
        while(choosing):
            actions = self.input_handler.process_events()
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