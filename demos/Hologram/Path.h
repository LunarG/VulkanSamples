#ifndef PATH_H
#define PATH_H

#include <memory>
#include <random>
#include <glm/glm.hpp>

class Curve;

class Path {
public:
    Path(unsigned rng_seed);

    glm::vec3 position(float t);

private:
    struct Subpath {
        glm::vec3 origin;
        float start;
        float end;

        std::shared_ptr<Curve> curve;
    };

    void generate_subpath(float t);

    std::mt19937 rng_;
    std::uniform_int_distribution<> type_;
    std::uniform_real_distribution<float> duration_;

    Subpath current_;
};

#endif // PATH_H
