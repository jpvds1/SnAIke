import pygame
from core.snake import Direction

_KEY_TO_DIRECTION = {
    pygame.K_UP: Direction.UP,
    pygame.K_w:  Direction.UP,
    pygame.K_DOWN: Direction.DOWN,
    pygame.K_s:    Direction.DOWN,
    pygame.K_LEFT: Direction.LEFT,
    pygame.K_a:    Direction.LEFT,
    pygame.K_RIGHT: Direction.RIGHT,
    pygame.K_d:     Direction.RIGHT
}

class InputHandler:

    def __init__(self):
        self._state = {
            "QUIT": False,
            "BACK": False,
            "MOUSE_CLICK": None,
            "MOUSE_POS": (0, 0),
            "DIRECTION": None
        }

    def process_events(self) -> dict:
        self._state["QUIT"] = False
        self._state["BACK"] = False
        self._state["MOUSE_CLICK"] = None
        self._state["MOUSE_POS"] = pygame.mouse.get_pos()
        self._state["DIRECTION"] = None
        
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                self._state["QUIT"] = True
            
            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    self._state["BACK"] = True
                elif event.key in _KEY_TO_DIRECTION:
                    self._state["DIRECTION"] = _KEY_TO_DIRECTION[event.key]
            
            if event.type == pygame.MOUSEBUTTONDOWN:
                if event.button == 1:
                    self._state["MOUSE_CLICK"] = pygame.mouse.get_pos()
                    
        return self._state

    @property
    def direction(self) -> Direction | None:
        return self._state["DIRECTION"]

    @property
    def quit(self) -> bool:
        return self._state["QUIT"]

    @property
    def back(self) -> bool:
        return self._state["BACK"]