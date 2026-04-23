

class Renderer:
    def __init__(self, screen, clock):
        self.screen = screen
        self.clock = clock

    def render_menu(self) -> dict:
        choices = {
            "type": "PLAYING",
            "model": "HUMAN"
        }

        '''
        choices = {
            "type": "TRAINING",
            "model": "NEAT",
            "limit_type": "GENERATIONS",
            "limit": 1000,
            "visualize": False
        }
        '''
        return choices