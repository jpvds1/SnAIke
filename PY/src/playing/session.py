import pygame
from typing import Callable

from envs.snake_env import SnakeEnv
from agents.base_agent import Agent
from ui.input_handler import InputHandler 

class Session:
    def __init__(
        self, 
        agent: Agent, 
        env: SnakeEnv, 
        input_handler: InputHandler, 
        clock: pygame.time.Clock | None, 
        frame_rate: int = 10,
        render_fn: Callable | None = None
    ):
        self.agent = agent
        self.env = env
        self.input_handler = input_handler
        self.clock = clock
        self.frame_rate = frame_rate
        self.render_fn = render_fn
        self._headless = render_fn is None and clock is None

    def run(self) -> dict:
        obs = self.env.reset()
        done = False
        total_reward = 0.0
        aborted = False

        # Render initial state before the first step
        if self.render_fn:
            self.render_fn(self.env._last_state)

        while not done:
            if not self._headless:
                actions = self.input_handler.process_events()
                if actions["QUIT"] or actions["BACK"]:
                    aborted = True
                    break

            action = self.agent.get_action(obs)
            obs, reward, done, info = self.env.step(action)
            total_reward += reward

            if self.render_fn:
                self.render_fn(info)

            if self.clock:
                self.clock.tick(self.frame_rate)

        final_state = self.env._last_state
        return {
            "score": final_state.get("score", 0),
            "steps": final_state.get("steps", 0),
            "total_reward": total_reward,
            "aborted": aborted
        }