#include "neuralNetwork.h"

NeuralNetwork::NeuralNetwork(std::vector<int> layerSizes) : layerSizes(layerSizes) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::normal_distribution<double> standard_normal(0.0, 1.0);

    for (int i = 0; i < (layerSizes.size() - 1); i++) {
        int rows = layerSizes[i];
        int cols = layerSizes[i + 1];

        double scalingFactor = std::sqrt(2.0 / rows);

        Matrix w(rows, cols);
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                w(r, c) = standard_normal(gen) * scalingFactor;
            }
        }

        std::vector<double> b(cols, 0.0);

        weights.push_back(w);
        biases.push_back(b); 
    }
}

std::vector<double> NeuralNetwork::forward(std::vector<double> x) const {
    for (int i = 0; i < weights.size(); i++) {
        std::vector<double> next_x = vectorMatrixMultiplication(x, weights[i]);
        
        for (size_t j = 0; j < next_x.size(); j++) {
            next_x[j] += biases[i][j];
        }

        x = next_x;

        if (i < weights.size() - 1) {
            for (auto& val : x) {
                val = std::tanh(val);
            }
        }
    }
    return x;
}

std::vector<double> NeuralNetwork::getFlat() const {
    std::vector<double> flat;

    for (const auto& mat : weights) {
        flat.insert(flat.end(), mat.data.begin(), mat.data.end());
    }

    for (const auto& layer : biases) {
        flat.insert(flat.end(), layer.begin(), layer.end());
    }

    return flat;
}

void NeuralNetwork::setFlat(const std::vector<double>& flat) {
    size_t idx = 0;

    for (auto& mat : weights) {
        for (auto& val : mat.data) {
            val = flat[idx++];
        }
    }

    for (auto& layer : biases) {
        for (auto& val : layer) {
            val = flat[idx++];
        }
    }
}