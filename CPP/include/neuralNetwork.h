// neuralNetwork.h

#pragma once

#include<vector>

class NeuralNetwork {
public:
    NeuralNetwork(std::vector<int> layerSizes);

    std::vector<float> forward(std::vector<float> x) const;
    std::vector<float> getFlat() const;
    void setFlat(std::vector<float>); 
private:
    std::vector<int> layerSizer;
    std::vector<float> weights;
    std::vector<float> biases;
};