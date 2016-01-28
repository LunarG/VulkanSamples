#ifndef ANIMATION_H
#define ANIMATION_H

#include <random>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Animation {
public:
    Animation(unsigned rng_seed);

    glm::mat4 transformation(float t);

private:
    struct Data {
        glm::vec3 axis;
        float speed;
        float scale;

        float time;
        glm::mat4 matrix;
    };

    std::mt19937 rng_;
    std::uniform_real_distribution<float> dir_;
    std::uniform_real_distribution<float> speed_;
    std::uniform_real_distribution<float> scale_;

    Data current_;
};

#endif // ANIMATION_H
