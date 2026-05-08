from abc import ABC, abstractmethod

class Agent(ABC):
    @abstractmethod
    def get_action(self):
        raise NotImplementedError