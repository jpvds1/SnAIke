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
    def __init__(self, screen, clock, u_height, u_width, scale_ratio, input_handler, agent_classes=None):
        self.screen = screen
        self.clock = clock
        self.height = u_height * scale_ratio
        self.width = u_width * scale_ratio
        self.scale_ratio = scale_ratio
        self.input_handler = input_handler

        self.menu_state = MenuState.MAIN

        self.font       = pygame.font.Font(None, 10 * scale_ratio)
        self.small_font = pygame.font.Font(None, 6 * scale_ratio)

        agent_classes = agent_classes or []
        self.all_agents = agent_classes
        self.play_selector = Selector([a.display_name for a in agent_classes])

        self.trainable_agents = [a for a in agent_classes if getattr(a, "trainable", True)]
        self.train_selector = Selector([a.display_name for a in self.trainable_agents])

        self.train_gen_limit = 100
        self.train_time_limit = 10
        self.train_headless = True
        self.train_limit_mode_selector = Selector(["Generations", "Time (min)"])

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

    def draw_number_control(self, value, cx, cy, width, height, step, mouse_click, mouse_pos) -> int:
        btn_w = height
        num_w = width - 2 * btn_w
        left_x = int(cx - width // 2)
        num_x = left_x + btn_w
        right_x = num_x + num_w
        top_y = int(cy - height // 2)
        new_value = value

        # Minus button
        lr = pygame.Rect(left_x, top_y, btn_w, height)
        lf = pygame.Rect(left_x + btn_w - BTN_RADIUS, top_y, BTN_RADIUS * 2, height)
        lh = lr.collidepoint(mouse_pos)
        pygame.draw.rect(self.screen, COLOR_NEUTRAL_HO if lh else COLOR_NEUTRAL, lr, border_radius=BTN_RADIUS)
        pygame.draw.rect(self.screen, COLOR_NEUTRAL_HO if lh else COLOR_NEUTRAL, lf)
        ms = self.font.render("\u2212", True, COLOR_TEXT)
        self.screen.blit(ms, ms.get_rect(center=lr.center))
        if lh and mouse_click:
            new_value = max(step, value - step)

        # Plus button
        rr = pygame.Rect(right_x, top_y, btn_w, height)
        rf = pygame.Rect(right_x - BTN_RADIUS, top_y, BTN_RADIUS * 2, height)
        rh = rr.collidepoint(mouse_pos)
        pygame.draw.rect(self.screen, COLOR_NEUTRAL_HO if rh else COLOR_NEUTRAL, rr, border_radius=BTN_RADIUS)
        pygame.draw.rect(self.screen, COLOR_NEUTRAL_HO if rh else COLOR_NEUTRAL, rf)
        ps = self.font.render("+", True, COLOR_TEXT)
        self.screen.blit(ps, ps.get_rect(center=rr.center))
        if rh and mouse_click:
            new_value = value + step

        # Value display
        nr = pygame.Rect(num_x, top_y, num_w, height)
        pygame.draw.rect(self.screen, COLOR_SURFACE, nr)
        pygame.draw.rect(self.screen, COLOR_BORDER, nr, 1)
        vs = self.font.render(str(value), True, COLOR_TEXT)
        self.screen.blit(vs, vs.get_rect(center=nr.center))
 
        return new_value

    def _draw_toggle(self, cx, cy, value: bool, disabled: bool = False):
        cx, cy = int(cx), int(cy)
        h = int(self.height * 0.055)
        w = h * 2
        x = cx - w // 2
        y = cy - h // 2
 
        track_col = COLOR_PRIMARY if value else COLOR_NEUTRAL
        if disabled:
            track_col = tuple(max(0, c - 70) for c in track_col)
 
        pygame.draw.rect(self.screen, track_col, (x, y, w, h), border_radius=BTN_RADIUS)
 
        pad = 4
        knob_size = h - 2 * pad
        knob_x = (x + w - knob_size - pad) if value else (x + pad)
        knob_col = COLOR_TEXT_MUTED if disabled else COLOR_TEXT
        pygame.draw.rect(self.screen, knob_col,
                         (knob_x, y + pad, knob_size, knob_size),
                         border_radius=BTN_RADIUS // 2)


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
            return {"mode": "play", "controller": self.all_agents[self.play_selector.index]}

        if self.draw_button("Back", cx, y_back, bw, bh, actions["MOUSE_CLICK"], actions["MOUSE_POS"], variant="secondary"):
            self.menu_state = MenuState.MAIN
            return

    def render_menu_algo_sel(self, actions):
        self.screen.fill(COLOR_BG)

        self.draw_text("Select Algorithm", self.width * 0.5, self.height * 0.12, color=COLOR_TEXT)
        self.draw_text("choose an algorithm to train", self.width * 0.5, self.height * 0.20, font=self.small_font, color=COLOR_TEXT_MUTED)

        self.draw_selector(
            self.train_selector,
            self.width * 0.5, self.height * 0.30,
            self.width * 0.60, self.height * 0.09,
            actions["MOUSE_CLICK"], actions["MOUSE_POS"]
        )


        px = int(self.width * 0.08)
        py = int(self.height * 0.42)
        pw = int(self.width * 0.84)
        ph = int(self.height * 0.28)
        
        pygame.draw.rect(self.screen, COLOR_SURFACE, (px, py, pw, ph), border_radius=BTN_RADIUS)
        pygame.draw.rect(self.screen, COLOR_BORDER, (px, py, pw, ph), 1, border_radius=BTN_RADIUS)

        agent_cls = self.trainable_agents[self.train_selector.index]
        stats = agent_cls.load_stats()

        
        desc = getattr(agent_cls, "description", "")
        self.draw_text(desc, self.width * 0.5, py + int(ph * 0.28), font=self.small_font, color=COLOR_TEXT_MUTED)


        sep_y = py + int(ph * 0.52)
        pygame.draw.line(self.screen, COLOR_BORDER, (px + 16, sep_y), (px + pw - 16, sep_y))


        stats_data = [
            ("Best Score", str(stats.get("highest_score", 0))),
            ("Generation", str(stats.get("current_generation", 0))),
            ("Avg Score", f"{stats.get("avg_score", 0.0):.2f}")
        ]
        col_w = pw // len(stats_data)
        lbl_y = py + int(ph * 0.66)
        val_y = py + int(ph * 0.84)
        for i, (label, val) in enumerate(stats_data):
            col_cx = px + col_w * i + col_w // 2
            self.draw_text(label, col_cx, lbl_y, font=self.small_font, color=COLOR_TEXT_MUTED)
            self.draw_text(val, col_cx, val_y, font=self.small_font, color=COLOR_TEXT)

        
        cx = self.width * 0.25
        bw = self.width * 0.5
        bh = self.height * 0.09

        if self.draw_button("Train", cx, self.height * 0.76, bw, bh, actions["MOUSE_CLICK"], actions["MOUSE_POS"]):
            self.menu_state = MenuState.TRAIN_CONFIG

        if self.draw_button("Back", cx, self.height * 0.87, bw, bh, actions["MOUSE_CLICK"], actions["MOUSE_POS"]):
            self.menu_state = MenuState.MAIN

    def render_menu_train_conf(self, actions):
        self.screen.fill(COLOR_BG)
 
        agent_cls = self.trainable_agents[self.train_selector.index]
 
        self.draw_text("Training Config",      self.width * 0.5, self.height * 0.12, color=COLOR_TEXT)
        self.draw_text(agent_cls.display_name, self.width * 0.5, self.height * 0.20, font=self.small_font, color=COLOR_PRIMARY_HO)
 
        self.draw_text("Limit by", self.width * 0.5, self.height * 0.26, font=self.small_font, color=COLOR_TEXT_MUTED)
        self.draw_selector(
            self.train_limit_mode_selector,
            self.width * 0.5, self.height * 0.34,
            int(self.width * 0.50), int(self.height * 0.09),
            actions["MOUSE_CLICK"], actions["MOUSE_POS"],
        )
 
        by_time = self.train_limit_mode_selector.selected == "Time (min)"
 
        if by_time:
            self.draw_text("Duration (minutes)", self.width * 0.5, self.height * 0.45, font=self.small_font, color=COLOR_TEXT_MUTED)
            self.train_time_limit = self.draw_number_control(
                value       = self.train_time_limit,
                cx          = self.width  * 0.5,
                cy          = self.height * 0.53,
                width       = int(self.width  * 0.50),
                height      = int(self.height * 0.09),
                step        = 1,
                mouse_click = actions["MOUSE_CLICK"],
                mouse_pos   = actions["MOUSE_POS"],
            )
        else:
            self.draw_text("Generations", self.width * 0.5, self.height * 0.45, font=self.small_font, color=COLOR_TEXT_MUTED)
            self.train_gen_limit = self.draw_number_control(
                value       = self.train_gen_limit,
                cx          = self.width  * 0.5,
                cy          = self.height * 0.53,
                width       = int(self.width  * 0.50),
                height      = int(self.height * 0.09),
                step        = 10,
                mouse_click = actions["MOUSE_CLICK"],
                mouse_pos   = actions["MOUSE_POS"],
            )
 
        self.draw_text("Headless Mode", self.width * 0.5, self.height * 0.62, font=self.small_font, color=COLOR_TEXT_MUTED)
        self._draw_toggle(self.width * 0.5, self.height * 0.69, value=True, disabled=True)
 
        cx = self.width  * 0.25
        bw = self.width  * 0.50
        bh = self.height * 0.09
 
        if self.draw_button("Start Training", cx, self.height * 0.76, bw, bh, actions["MOUSE_CLICK"], actions["MOUSE_POS"]):
            return {
                "mode":       "train",
                "controller": agent_cls,
                "limit_mode": "time" if by_time else "generations",
                "limit":      self.train_time_limit if by_time else self.train_gen_limit,
                "headless":   self.train_headless,
            }
 
        if self.draw_button("Back", cx, self.height * 0.87, bw, bh, actions["MOUSE_CLICK"], actions["MOUSE_POS"], variant="secondary"):
            self.menu_state = MenuState.ALGORITHM_SELECT

    def render_menu(self) -> dict:
        choosing = True
        while(choosing):
            actions = self.input_handler.process_events()
            if actions["QUIT"] or self.menu_state == MenuState.QUIT:
                return {"QUIT": True}

            if actions["BACK"]:
                if self.menu_state == MenuState.MAIN:
                    return {"QUIT": True}
                elif self.menu_state in (MenuState.PLAY_SELECT, MenuState.ALGORITHM_SELECT):
                    self.menu_state = MenuState.MAIN
                elif self.menu_state == MenuState.TRAIN_CONFIG:
                    self.menu_state = MenuState.ALGORITHM_SELECT
            
            elif self.menu_state == MenuState.MAIN:
                self.render_menu_main(actions)
            elif self.menu_state == MenuState.PLAY_SELECT:
                result = self.render_menu_play_sel(actions)
                if result:
                    return result
            elif self.menu_state == MenuState.ALGORITHM_SELECT:
                self.render_menu_algo_sel(actions)
            elif self.menu_state == MenuState.TRAIN_CONFIG:
                result = self.render_menu_train_conf(actions)
                if result:
                    return result

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