import pygame

from ui.renderer import Renderer
from ui.input_handler import InputHandler
from envs.snake_env import SnakeEnv
from playing.session import Session
from agents.human_agent import HumanAgent

from agents.human_agent import HumanAgent
from agents.genetic_agent import GeneticAgent
from agents.neat_agent import NEATAgent
from agents.qlearning_agent import QLearningAgent

FRAME_RATE = 10
SCALE_RATIO = 4
SCREEN_WIDTH = 200
SCREEN_HEIGHT = 200

ALL_AGENTS = [
    HumanAgent,
    GeneticAgent,
    NEATAgent,
    QLearningAgent
]

def initialize_system():
    pygame.init()
    screen = pygame.display.set_mode((SCREEN_WIDTH * SCALE_RATIO, SCREEN_HEIGHT * SCALE_RATIO))
    pygame.display.set_caption("SnAIke")
    clock = pygame.time.Clock()

    input_handler = InputHandler()
    renderer = Renderer(screen, clock, SCREEN_HEIGHT, SCREEN_WIDTH, SCALE_RATIO, input_handler, ALL_AGENTS)
    return screen, clock, renderer, input_handler

def build_agent(agent_class, input_handler: InputHandler):
    if not getattr(agent_class, "trainable", True):
        return agent_class(input_handler)
    return agent_class()

def shutdown():
    pygame.quit()

def main_loop(renderer, clock, input_handler):
    running = True
    while(running):
        config = renderer.render_menu()

        if config.get("QUIT", False):
            running = False
            continue

        mode = config.get("mode")

        if mode == "play":
            controller = config.get("controller")
            if controller is None:
                continue

            agent = build_agent(controller, input_handler)
            env = SnakeEnv()

            session = Session(
                agent=agent,
                env=env,
                input_handler=input_handler,
                clock=clock,
                frame_rate=FRAME_RATE,
                render_fn=renderer.render_game
            )

            result = session.run()
            print(f"Game over - score: {result['score']} steps: {result['steps']}")
        elif mode == "train":
            agent_class = config.get("controller")
            limit       = config.get("limit", 100)
            headless    = config.get("headless", True)
            print(f"Training {agent_class.display_name} for {limit} generations\n(headless={headless}) - Trainer not yet implemented.")

def main():
    screen, clock, renderer, input_handler = initialize_system()
    main_loop(renderer, clock, input_handler)
    shutdown()

if __name__ == "__main__":
    main()