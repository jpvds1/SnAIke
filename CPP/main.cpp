#include <iostream>
#include <cassert>
#include "include/types.h" // Assuming you grouped types here
#include "include/engine.h"

// Helper function to print the current board layout to the console
void printBoard(const State& state) {
    std::cout << "--- Step: " << state.steps 
              << " | Score: " << state.score 
              << " | Hunger: " << state.hunger << " ---\n";

    for (int y = 0; y < state.boardSize.y; ++y) {
        for (int x = 0; x < state.boardSize.x; ++x) {
            Position currentPos{x, y};

            if (currentPos == state.head) {
                std::cout << "H "; // Snake Head
            } else if (currentPos == state.apple) {
                std::cout << "A "; // Apple
            } else {
                bool isBody = false;
                // Skip the head (index 0) to print body units distinctly
                for (size_t i = 1; i < state.snakePositions.size(); ++i) {
                    if (state.snakePositions[i] == currentPos) {
                        isBody = true;
                        break;
                    }
                }
                std::cout << (isBody ? "o " : ". "); // Body or Empty space
            }
        }
        std::cout << "\n";
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "Initializing Engine Test..." << std::endl;
    Engine engine;

    // Test 1: Reset Environment
    State state = engine.reset();
    std::cout << "Environment Reset Successful!" << std::endl;
    std::cout << "Initial Snake Head: (" << state.head.x << ", " << state.head.y << ")" << std::endl;
    std::cout << "Initial Apple Position: (" << state.apple.x << ", " << state.apple.y << ")\n" << std::endl;
    
    printBoard(state);

    // Test 2: Simulating a few steps
    // Let's force the snake to move UP a few times
    std::cout << "Moving UP..." << std::endl;
    GymState stepResult = engine.update(Direction::UP);
    printBoard(stepResult.state);

    std::cout << "Moving LEFT..." << std::endl;
    stepResult = engine.update(Direction::LEFT);
    printBoard(stepResult.state);

    // Test 3: Basic Assertion Checks
    assert(stepResult.state.steps == 2);
    assert(stepResult.state.hunger == 2);
    std::cout << "Step and Hunger tracking assertions passed!" << std::endl;

    std::cout << "\n🎉 All basic engine tests passed successfully!" << std::endl;
    return 0;
}