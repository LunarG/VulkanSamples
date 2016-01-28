#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include "Simulation.h"

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

Simulation::Simulation(int object_count)
    : random_dev_()
{
    objects_.reserve(object_count);
    for (int i = 0; i < object_count; i++) {
        Object obj = { i, random_dev_(), random_dev_() };
        objects_.push_back(obj);
    }
}

void Simulation::set_frame_data_size(uint32_t size)
{
    uint32_t offset = 0;
    for (auto &obj : objects_) {
        obj.frame_data_offset = offset;
        offset += size;
    }
}

void Simulation::update(float obj_time, int begin, int end)
{
    for (int i = begin; i < end; i++) {
        auto &obj = objects_[i];

        glm::vec3 pos = obj.path.position(obj_time);
        glm::mat4 trans = obj.animation.transformation(obj_time);
        obj.model = glm::translate(glm::mat4(1.0f), pos) * trans;
    }
}
