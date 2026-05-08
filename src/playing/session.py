import pygame
from envs.snake_env import SnakeEnv
from agents.base_agent import Agent
from ui.input_handler import InputHandler 

class Session:
    def __init__(self, agent: Agent, env: SnakeEnv, input_handler: InputHandler, clock: pygame.time.Clock | None, frame_rate: int = 10):
        self.agent = agent
        self.env = env
        self.input_handler = input_handler
        self.clock = clock
        self.frame_rate = frame_rate

    def run(self) -> dict:
        obs = self.env.reset()
        done = False
        total_reward = 0.0
        aborted = False

        while not done:
            actions = self.input_handler.process_events()

            if actions["QUIT"] or actions["BACK"]:
                aborted = True
                break

            action = self.agent.get_action(obs)
            obs, reward, done, info = self.env.step(action)
            total_reward += reward

            if self.clock:
                self.clock.tick(self.frame_rate)

        final_state = self.env._last_state
        return {
            "score": final_state.get("score", 0),
            "steps": final_state.get("steps", 0),
            "total_reward": total_reward,
            "aborted": aborted
        }