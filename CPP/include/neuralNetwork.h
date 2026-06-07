// neuralNetwork.h

#pragma once

#include <vector>
#include <random>
#include <cmath>

#include "helpers.h"

class NeuralNetwork {
public:
    NeuralNetwork(std::vector<int> layerSizes);

    std::vector<double> forward(std::vector<double> x) const;
    std::vector<double> getFlat() const;
    void setFlat(const std::vector<double>& flat); 
    const std::vector<int>& getLayerSizes() const { return layerSizes; }
private:
    std::vector<int> layerSizes;
    std::vector<Matrix> weights;
    std::vector<std::vector<double>> biases;
};