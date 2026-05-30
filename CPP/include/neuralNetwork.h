// neuralNetwork.h

#pragma once

#include <vector>
#include <random>

#include "helpers.h"

class NeuralNetwork {
public:
    NeuralNetwork(std::vector<int> layerSizes);

    std::vector<double> forward(std::vector<double> x) const;
    std::vector<double> getFlat() const;
    void setFlat(const std::vector<double>& flat); 
private:
    std::vector<int> layerSizer;
    std::vector<std::vector<std::vector<double>>> weights;
    std::vector<std::vector<double>> biases;
};