#include <cmath>
#include "Animation.h"

Animation::Animation(unsigned int rng_seed)
    : rng_(rng_seed), dir_(-1.0f, 1.0f), speed_(0.1f, 1.0f), scale_(0.005f, 0.02f)
{
    float x = dir_(rng_);
    float y = dir_(rng_);
    float z = std::sqrt(1 - x * x - y * y);
    z = std::copysign(z, dir_(rng_));
    current_.axis = glm::vec3(x, y, z);

    current_.speed = speed_(rng_);
    current_.scale = scale_(rng_);

    current_.time = 0;
    current_.matrix = glm::scale(glm::mat4(1.0f), glm::vec3(current_.scale));
}

glm::mat4 Animation::transformation(float t)
{
    current_.matrix = glm::rotate(current_.matrix, current_.speed * (t - current_.time), current_.axis);
    current_.time = t;

    return current_.matrix;
}
