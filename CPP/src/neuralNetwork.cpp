#include "../include/neuralNetwork.h"
#include <cmath>
#include <cstddef>
#include <random>

NeuralNetwork::NeuralNetwork(std::vector<int> layerSizes) : layerSizes(layerSizes) {
    for (int i = 0; i < (layerSizes.size() - 1); i++) {
        int rows = layerSizes[i];
        int cols = layerSizes[i + 1];
        weights.emplace_back(rows, cols);
        biases.push_back(std::vector<double>(cols, 0.0)); 
    }
}

void NeuralNetwork::randomize(std::mt19937& gen) {
    std::normal_distribution<double> standard_normal(0.0, 1.0);

    for (int i = 0; i < weights.size(); i++) {
        int rows = layerSizes[i];
        double scalingFactor = std::sqrt(2.0 / rows);

        for (auto& val : weights[i].data) {
            val = standard_normal(gen) * scalingFactor;
        }
    }
}

std::vector<double> NeuralNetwork::forward(std::span<const double> x) const {
    thread_local std::vector<double> tl_a, tl_b;

    tl_a.assign(x.begin(), x.end());

    for (int i = 0; i < static_cast<int>(weights.size()); ++i) {
        const Matrix& w = weights[i];
        const int rows = w.rows;
        const int cols = w.cols;

        tl_b.assign(cols, 0.0);

        for (int r = 0; r < rows; ++r) {
            const double ar = tl_a[static_cast<size_t>(r)];
            for (int c = 0; c < cols; ++c)
                tl_b[static_cast<size_t>(c)] += ar * w(r, c);
        }

        for (int j = 0; j < cols; ++j)
            tl_b[static_cast<size_t>(j)] += biases[i][static_cast<size_t>(j)];

        if (i < static_cast<int>(weights.size()) - 1) {
            for (auto& v : tl_b)
                v = std::tanh(v);
        }

        std::swap(tl_a, tl_b);
    }

    return {tl_a.begin(), tl_a.end()};
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