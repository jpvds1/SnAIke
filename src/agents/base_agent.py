from abc import ABC, abstractmethod
import numpy as np
from core.snake import Direction


# ---------------------------------------------------------------------------
# Genome
# ---------------------------------------------------------------------------

class Genome(ABC):
    # Single member of a population

    @abstractmethod
    def get_action(self, state: dict) -> Direction | None:
        raise NotImplementedError

# ---------------------------------------------------------------------------
# Base Agent
# ---------------------------------------------------------------------------

class Agent(ABC):
    trainable: bool = True
    display_name: str = ""
    description: str = ""

    @abstractmethod
    def get_action(self, observation: np.ndarray) -> Direction | None:
        raise NotImplementedError

    @classmethod
    def load_stats(cls) -> dict:
        return {
            "highest_score": 0,
            "current_generation": 0,
            "avg_score": 0.0
        }

# ---------------------------------------------------------------------------
# Population Agent
# ---------------------------------------------------------------------------

class PopulationAgent(Agent, ABC):
    # Population-based agents (GA, NEAT...)

    @abstractmethod
    def get_population(self) -> list[Genome]:
        # Export current generation
        raise NotImplementedError

    @abstractmethod
    def evolve(self, results: list[dict]) -> None:
        raise NotImplementedError