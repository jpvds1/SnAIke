// neuralNetwork.h

#pragma once

#include <vector>
#include <random>
#include <cmath>

#include "helpers.h"

struct Matrix {
    int rows;
    int cols;
    std::vector<double> data;

    Matric() : rows(0), cols(0) {}
    Matrix(int r, int c) : rows(r), cols(c), data(r * c, 0.0) {}
    double& operator()(int r, int c) { return data[r * cols + c]; }
    const double& operator()(int r, int c) const { return data[r * cols + c]; }
}

class NeuralNetwork {
public:
    NeuralNetwork(std::vector<int> layerSizes);

    std::vector<double> forward(std::vector<double> x) const;
    std::vector<double> getFlat() const;
    void setFlat(const std::vector<double>& flat); 
private:
    std::vector<int> layerSizes;
    std::vector<Matrix> weights;
    std::vector<std::vector<double>> biases;
};