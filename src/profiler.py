import cProfile
import pstats
import io
import time
import numpy as np
from collections import defaultdict

_timers: dict[str, float] = defaultdict(float)
_counts: dict[str, int] = defaultdict(int)

def _tick(label: str) -> float:
    return time.perf_counter()

def _tock(label: str, t0: float):
    _timers[label] += time.perf_counter() - t0
    _counts[label] += 1

def print_timings(n_episodes: int):
    print("\n" + "=" * 60)
    print(f"TIMINGS ({n_episodes} episodes)")
    print("=" * 60)
    total = sum(_timers.values()) or 1e-9
    rows = sorted(_timers.items(), key=lambda x: x[1], reverse=True)
    for label, elapsed in rows:
        n = _counts[label]
        pct = elapsed / total * 100
        per_call = elapsed / n * 1_000 if n else 0
        print(f"{label:<35} | {elapsed:7.3f}s | {pct:5.1f}% | n={n:>7,} | {per_call:.4f}ms/call")
    print("=" * 60)

from core.engine import Engine
from envs.snake_env import SnakeEnv
from agents.genetic_agent import GeneticAgent, ObservationWrapper, NeuralNetwork
from playing.session import Session
from playing.trainer import Trainer, TrainerConfig

# ObservationWrapper

_orig_obs = ObservationWrapper.observation_from_state.__func__

@classmethod
def _timed_obs(cls, state):
    t0 = _tick("obs_from_state")
    result = _orig_obs(cls, state)
    _tock("obs_from_state", t0)
    return result

ObservationWrapper.observation_from_state = _timed_obs

# NeuralNetwork

_orig_forward = NeuralNetwork.forward

def _timed_forward(self, x):
    t0 = _tick("nn_forward")
    result = _orig_forward(self, x)
    _tock("nn_forward", t0)
    return result

NeuralNetwork.forward = _timed_forward

# Engine

_orig_update = Engine.update

def _timed_update(self, direction=None):
    t0 = _tick("engine_update")
    result = _orig_update(self, direction)
    _tock("engine_update", t0)
    return result

Engine.update = _timed_update

_orig_spawn = Engine._spawn_apple

def _timed_spawn(self):
    t0 = _tick("engine_spawn_apple")
    result = _orig_spawn(self)
    _tock("engine_spawn_apple", t0)
    return result

Engine._spawn_apple = _timed_spawn

# Genetic Agent

_orig_get_action = GeneticAgent.get_action

def _timed_get_action(self, state):
    t0 = _tick("agent_get_action")
    result = _orig_get_action(self, state)
    _tock("agent_get_action", t0)
    return result

GeneticAgent.get_action = _timed_get_action

# Session

_orig_run = Session.run

def _timed_run(self):
    t0 = _tick("session_run")
    result = _orig_run(self)
    _tock("session_run", t0)
    return result

Session.run = _timed_run

# Helpers

N_EPISODES = 100

def run_episodes(n: int):
    agent = GeneticAgent(pop_size=n)
    config = TrainerConfig(max_generations=1, env_width=20, env_height=20)
    trainer = Trainer(agent, config)
    trainer.run()

# cProfile run

print("\nRunnin cProfile ...")
pr = cProfile.Profile()
pr.enable()
run_episodes(N_EPISODES)
pr.disable()

stream = io.StringIO()
ps = pstats.Stats(pr, stream=stream).sort_stats("cumulative")
ps.print_stats(25)

print("\n" + "=" * 60)
print("CPROFFILE - top 25 by cumulative time")
print("=" * 60)
print(stream.getvalue())

# Manual timings

print_timings(N_EPISODES)