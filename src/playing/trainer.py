import os
import time
import threading
import multiprocessing as mp
from concurrent.futures import ProcessPoolExecutor
from dataclasses import dataclass, field
from itertools import repeat
from typing import Callable

from agents.base_agent import Agent, PopulationAgent
from playing.evaluator import EnvConfig, evaluate_genome
from playing.session import Session
from envs.snake_env import SnakeEnv

# ---------------------------------------------------------------------------
# Stubs
# ---------------------------------------------------------------------------

_NULL_ACTIONS: dict = {
    "QUIT":        False,
    "BACK":        False,
    "MOUSE_CLICK": False,
    "MOUSE_POS":   (0, 0)
}

# Headless input stub
class _NullInputHandler:
    def process_events(self) -> dict:
        return _NULL_ACTIONS

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
    n_workers: int | None = None

    @property
    def resolved_workers(self) -> int:
        if self.n_workers is not None:
            return max(1, self.n_workers)
        return max(1, (os.cpu_count() - 2) or 1)

    @classmethod
    def from_ui(
        cls,
        limit_mode: str,
        limit: int | float,
        *,
        env_width: int = 20,
        env_height: int = 20,
        n_workers: int | None = None
    ) -> "TrainerConfig":
        if limit_mode == "time":
            return cls(
                time_limit_minutes=float(limit),
                env_width=env_width,
                env_height=env_height,
                n_workers=n_workers
            )
        return cls(
            max_generations=int(limit),
            env_width=env_width,
            env_height=env_height,
            n_workers=n_workers
        )

    def as_env_config(self) -> EnvConfig:
        return EnvConfig(width=self.env_width, height=self.env_height)

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
        self._generation: int = 0

        self._executor: ProcessPoolExecutor | None = None

    # ---------------------------------------------------------------------------
    # Public API
    # ---------------------------------------------------------------------------

    def run(self) -> list[GenerationStats]:
        self._stop_event.clear()
        self._history.clear()
        self._generation = 0

        use_parallel = (
            isinstance(self.agent, PopulationAgent)
            and self.config.resolved_workers > 1
        )

        if use_parallel:
            self._run_with_pool()
        else:
            self._run_loop()

        run_start = time.monotonic()

        self._call(self.agent, "on_training_end", self._history)
        return self._history

    def start(self) -> threading.Thread:
        t = threading.Thread(target=self.run, daemon=True, name="TrainerThread")
        t.start()
        return t

    def stop(self) -> None:
        self._stop_event.set()
        if self._executor is not None:
            self._executor.shutdown(wait=False, cancel_futures=True)

    @property
    def current_generation(self) -> int:
        return self._generation

    @property
    def history(self) -> list[GenerationStats]:
        return list(self._history)

    # ---------------------------------------------------------------------------
    # Pool lifecycle
    # ---------------------------------------------------------------------------

    def _run_with_pool(self) -> None:
        n = self.config.resolved_workers
        ctx = mp.get_context("spawn")

        with ProcessPoolExecutor(max_workers=n, mp_context=ctx) as executor:
            self._executor = executor
            try:
                self._run_loop()
            finally:
                self._executor = None

    def _run_loop(self) -> None:
        run_start = time.monotonic()
        while not self._should_stop(run_start):
            self._generation += 1
            stats = self._run_generation(self._generation, run_start)
            self._history.append(stats)
            if self._on_generation_end:
                self._on_generation_end(stats)

    # ---------------------------------------------------------------------------
    # Dispatch generation
    # ---------------------------------------------------------------------------

    def _run_generation(self, gen_num: int, run_start: float) -> GenerationStats:
        if isinstance(self.agent, PopulationAgent):
            return self._run_generation_population(gen_num, run_start)
        raise NotImplementedError

    # ---------------------------------------------------------------------------
    # Population-based path
    # ---------------------------------------------------------------------------
        
    def _run_generation_population(self, gen_num: int, run_start: float) -> GenerationStats:
        population = self.agent.get_population()
        env_cfg = self.config.as_env_config()
        gen_start = time.monotonic()

        results = self._evaluate_population(population, env_cfg)

        if results:
            self.agent.evolve(results)

        return GenerationStats(
            generation=gen_num,
            scores=[r["score"] for r in results],
            total_rewards=[r["total_reward"] for r in results],
            steps=[r["steps"] for r in results],
            duration_seconds=time.monotonic() - gen_start,
        )

    def _evaluate_population(self, population: list, env_cfg: EnvConfig) -> list[dict]:
        if self._executor is None:
            return [evaluate_genome(g, env_cfg) for g in population]

        n         = self.config.resolved_workers
        chunksize = max(1, len(population) // (n * 4))

        try:
            results = list(
                self._executor.map(
                    evaluate_genome,
                    population,
                    repeat(env_cfg),
                    chunksize=chunksize,
                )
            )
        except Exception as e:
            print(f"[Trainer] Worker exception during evaluation: {e}")
            print("[Trainer] Falling back to sequential evalutaion")
            results = [evaluate_genome(g, env_cfg) for g in population]
        
        return results

    # ---------------------------------------------------------------------------
    # Helpers
    # ---------------------------------------------------------------------------

    def _should_stop(self, run_start: float) -> bool:
        if self._stop_event.is_set():
            return True
        if self.config.max_generations is not None and self._generation >= self.config.max_generations:
            return True
        if self.config.time_limit_minutes is not None:
            elapsed = (time.monotonic() - run_start) / 60.0
            if elapsed >= self.config.time_limit_minutes:
                return True
        return False

    @staticmethod
    def _call(obj, method: str, *args) -> None:
        fn = getattr(obj, method, None)
        if callable(fn):
            fn(*args)