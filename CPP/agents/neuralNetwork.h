// neuralNetwork.h

#pragma once

#include <vector>
#include <random>
#include <cmath>
#include <span>

#include "helpers.h"

class NeuralNetwork {
public:
    NeuralNetwork(std::vector<int> layerSizes);

    std::vector<double> forward(std::span<const double> x) const;
    std::vector<double> getFlat() const;
    void setFlat(const std::vector<double>& flat); 
    const std::vector<int>& getLayerSizes() const { return layerSizes; }
    void randomize(std::mt19937& gen);
private:
    std::vector<int> layerSizes;
    std::vector<Matrix> weights;
    std::vector<std::vector<double>> biases;
};