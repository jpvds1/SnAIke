from dataclasses import dataclass
from agents.base_agent import Genome
from envs.snake_env import SnakeEnv

# ---------------------------------------------------------------------------
# Environment configuration
# ---------------------------------------------------------------------------

@dataclass(frozen=True)
class EnvConfig:
    width: int = 20
    height: int = 20

# ---------------------------------------------------------------------------
# Evaluation
# ---------------------------------------------------------------------------

def evaluate_genome(genome: Genome, env_config: EnvConfig) -> dict:
    # Run single episode for genome and return the result

    env = SnakeEnv(width=env_config.width, height=env_config.height)
    state = env.reset()
    done = False
    total_reward = 0.0

    while not done:
        action = genome.get_action(state)
        state, reward, done, _ = env.step(action)
        total_reward += reward

    return {
        "score":        state.get("score", 0),
        "steps":        state.get("steps", 0),
        "total_reward": total_reward,
    }