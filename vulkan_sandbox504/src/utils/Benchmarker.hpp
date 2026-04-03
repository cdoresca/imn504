#pragma once
#include <cmath> // For pow and log
#include <functional>
#include <iostream>
#include <string>
#include <variant>
#include <vector>


class ParameterCombinator {
public:
    enum class SteppingType {
        Linear,
        Power,
        Logarithmic
    };

    void addParameter(const std::string &name, float start, float stop, float step, float &ref, SteppingType stepType = SteppingType::Linear) {
        if (step == 0 && start != stop) {
            std::cerr << "Error: When step is 0, start must be equal to stop for parameter '" << name << "'.\n";
            return;
        }
        parameters.push_back({name, start, stop, step, stepType});
        paramRefs.push_back(std::ref(ref));
    }

    void addParameter(const std::string &name, int start, int stop, int step, int &ref, SteppingType stepType = SteppingType::Linear) {
        if (step == 0 && start != stop) {
            std::cerr << "Error: When step is 0, start must be equal to stop for parameter '" << name << "'.\n";
            return;
        }
        parameters.push_back({name, float(start), float(stop), float(step), stepType});
        paramRefs.push_back(std::ref(ref));
    }

    void addParameter(const std::string &name, uint start, uint stop, uint step, uint &ref, SteppingType stepType = SteppingType::Linear) {
        if (step == 0 && start != stop) {
            std::cerr << "Error: When step is 0, start must be equal to stop for parameter '" << name << "'.\n";
            return;
        }
        parameters.push_back({name, float(start), float(stop), float(step), stepType});
        paramRefs.push_back(std::ref(ref));
    }

    void generateCombinations() {
        int N = parameters.size();
        if (N == 0) {
            std::cerr << "No parameters added.\n";
            return;
        }

        std::vector<float> current(N); // Vector to store the current combination

        // Initialize the current combination with the start values
        for (int i = 0; i < N; ++i) {
            current[i] = parameters[i].start;
        }

        // Generate all combinations
        while (true) {
            // Store the current combination in the 2D vector
            combinations.push_back(current);

            // Find the rightmost element that can be incremented
            int i = N - 1;
            while (i >= 0 && current[i] >= parameters[i].stop) {
                i--;
            }

            // If no such element exists, we have generated all combinations
            if (i < 0) {
                break;
            }

            // Increment this element based on the stepping type
            if (parameters[i].step != 0) {
                current[i] = getNextValue(current[i], parameters[i]);
            }

            // Reset all subsequent elements to their start values
            for (int j = i + 1; j < N; ++j) {
                current[j] = parameters[j].start;
            }
        }
    }

    void executeCombination(size_t index) {
        if (index >= combinations.size()) {
            std::cerr << "Invalid index.\n";
            return;
        }

        setParameterValues(combinations[index]);
    }

    uint getCombinationsCount() const {
        return combinations.size();
    }

    void printCombination(uint index) const {
        for (float value : combinations[index]) {
            std::cout << value << " ";
        }
        std::cout << std::endl;
    }

    
private:
    struct Parameter {
        std::string name;
        float start;
        float stop;
        float step;
        SteppingType stepType;
    };

    public:
    std::vector<Parameter> parameters;
    private:

    std::vector<std::vector<float>> combinations;
    std::vector<std::variant<std::reference_wrapper<int>, std::reference_wrapper<uint>, std::reference_wrapper<float>>> paramRefs;

    // Set the values of the variables according to the current combination
    void setParameterValues(const std::vector<float> &combination) {
        for (size_t i = 0; i < combination.size(); ++i) {
            if (std::holds_alternative<std::reference_wrapper<int>>(paramRefs[i])) {
                std::get<std::reference_wrapper<int>>(paramRefs[i]).get() = static_cast<int>(combination[i]);
            } else if (std::holds_alternative<std::reference_wrapper<float>>(paramRefs[i])) {
                std::get<std::reference_wrapper<float>>(paramRefs[i]).get() = combination[i];
            } else if (std::holds_alternative<std::reference_wrapper<uint>>(paramRefs[i])) {
                std::get<std::reference_wrapper<uint>>(paramRefs[i]).get() = static_cast<uint>(combination[i]);
            }
        }
    }

    // Get the next value based on the stepping type
    inline float getNextValue(float current, const Parameter &param) {
        switch (param.stepType) {
        case SteppingType::Linear:
            return current + param.step;
        case SteppingType::Power:
            return current * param.step; // step is treated as a multiplier for power stepping
        case SteppingType::Logarithmic:
            return std::exp(std::log(current) + std::log(param.step)); // step is treated as a logarithmic multiplier
        default:
            return current + param.step; // Default to linear if undefined
        }
    }
};


struct RunningStats {
    double mean = 0.0;
    double M2 = 0.0;
    int n = 0;

    void update(double value) {
        ++n;
        double delta = value - mean;
        mean += delta / n;
        double delta2 = value - mean;
        M2 += delta * delta2;
    }

    double variance() const { return n > 1 ? M2 / (n - 1) : std::numeric_limits<double>::infinity(); }

    double stddev() const { return sqrt(variance()); }
};

// Function to check statistical significance
bool is_statistically_significant(double mean, double stddev, int n, double threshold) {
    if (n < 2) return false;

    // Compute standard error
    double standard_error = stddev / sqrt(n);

    // Compute 95% confidence interval
    double margin_of_error = 1.96 * standard_error; // Approximation for 95% CI
    return (2 * margin_of_error) < threshold * mean;
}