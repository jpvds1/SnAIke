import time
import threading
from dataclasses import dataclass, field
from typing import Callable

from agents.base_agent import Agent
from playing.session import Session
from envs.snake_env import SnakeEnv

# Headless input stub
class _NullInputHandler:
    def process_events(self) -> dict:
        return {
            "QUIT": False,
            "BACK": False,
            "MOUSE_CLICK": False,
            "MOUSE_POS": (0, 0)
        }


# ---------------------------------------------------------------------------
# Per-generation statistics
# ---------------------------------------------------------------------------

@dataclass
class GenerationStats:
    generation: int
    scores: list[int] = field(default_factory=list)
    total_rewards: list[float] = field(default_factory=list)
    steps: list[int] = field(default_factory=list)
    duration_seconds: float = 0.0

    @property
    def avg_score(self) -> float:
        return sum(self.scores) / len(self.scores) if self.scores else 0.0

    @property
    def max_score(self) -> int:
        return max(self.scores, default=0)

    @property
    def avg_reward(self) -> float:
        return sum(self.total_rewards) / len(self.total_rewards) if self.total_rewards else 0.0

    @property
    def avg_steps(self) -> float:
        return sum(self.steps) / len(self.steps) if self.steps else 0.0

    def __repr__(self) -> str:
        return (
            f"Gen {self.generation:>4} | "
            f"avg_score={self.avg_score:.2f} max={self.max_score}  "
            f"avg_reward={self.avg_reward:.3f}  "
            f"episodes={len(self.scores)}  "
            f"elapsed={self.duration_seconds:.1f}s"
        ) 

# ---------------------------------------------------------------------------
# Trainer config
# ---------------------------------------------------------------------------

@dataclass
class TrainerConfig:
    max_generations: int | None = None
    time_limit_minutes: float | None = None
    env_width: int = 20
    env_height: int = 20

    @classmethod
    def from_ui(
        cls,
        limit_mode: str,
        limit: int | float,
        *,
        env_width: int = 20,
        env_height: int = 20
    ) -> "TrainerConfig":
        if limit_mode == "time":
            return cls(
                time_limit_minutes=float(limit),
                env_width=env_width,
                env_height=env_height
            )
        return cls(
            max_generations=int(limit),
            env_width=env_width,
            env_height=env_height
        )

# ---------------------------------------------------------------------------
# Trainer
# ---------------------------------------------------------------------------

class Trainer:

    def __init__(
        self, 
        agent: Agent, 
        config: TrainerConfig, 
        *,
        on_generation_end: Callable[[GenerationStats], None] | None = None    
    ) -> None:
        if not getattr(agent, "trainable", False):
            raise ValueError(f"Agent '{type(agent).__name__} is not trainable.")

        self.agent = agent
        self.config = config
        self._on_generation_end = on_generation_end

        self._stop_event = threading.Event()
        self._history: list[GenerationStats] = []
        self._best_score: int = 0
        self._generation: int = 0

    # ---------------------------------------------------------------------------
    # Public API
    # ---------------------------------------------------------------------------

    def run(self) -> list[GenerationStats]:
        self._stop_event.clear()
        self._history.clear()
        self._generation = 0

        run_start = time.monotonic()

        while not self._should_stop(run_start):
            self._generation += 1
            stats = self._run_generation(self._generation, run_start)
            self._history.append(stats)

            self._call(self.agent, "on_generation_end", stats)
            if self._on_generation_end:
                self._on_generation_end(stats)

        self._call(self.agent, "on_training_end", self._history)
        return self._history

    def start(self) -> threading.Thread:
        t = threading.Thread(target=self.run, daemon=True, name="TrainerThread")
        t.start()
        return t

    def stop(self) -> None:
        self._stop_event.set()

    @property
    def current_generation(self) -> int:
        return self._generation

    @property
    def history(self) -> list[GenerationStats]:
        return list(self._history)

    # ---------------------------------------------------------------------------
    # Interval
    # ---------------------------------------------------------------------------

    def _should_stop(self, run_start: float) -> bool:
        if self._stop_event.is_set():
            return True

        if self.config.max_generations is not None and self._generation > self.config.max_generations:
            return True

        if self.config.time_limit_minutes is not None:
            elapsed = (time.monotonic() - run_start) / 60.0
            if elapsed >= self.config.time_limit_minutes:
                return True

        return False

    def _run_generation(self, gen_nun: int, run_start: float) -> GenerationStats:
        n_episodes = getattr(self.agent, "episodes_per_generation", 1)
        scores, rewards, steps = [], [], []
        gen_start = time.monotonic()

        for _ in range(n_episodes):
            if self._should_stop(run_start):
                break
            
            result = self._run_episode()
            if result["aborted"]:
                self.stop()
                break

            self._call(self.agent, "on_episode_end", result)
            
            scores.append(result["score"])
            rewards.append(result["total_reward"])
            steps.append(result["steps"])
        
        return GenerationStats(
            generation=gen_nun,
            scores=scores,
            total_rewards=rewards,
            steps=steps,
            duration_seconds=time.monotonic() - gen_start
        )

    def _run_episode(self) -> dict:
        env = SnakeEnv(width=self.config.env_width, height=self.config.env_height)
        session = Session(
            agent = self.agent,
            env=env,
            input_handler=_NullInputHandler(),
            clock=None,
            render_fn=None
        )
        return session.run()

    @staticmethod
    def _call(obj, method: str, *args) -> None:
        fn = getattr(obj, method, None)
        if callable(fn):
            fn(*args)