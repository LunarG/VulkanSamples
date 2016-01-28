#ifndef SIMULATION_H
#define SIMULATION_H

#include <random>
#include <vector>

#include <glm/glm.hpp>

#include "Path.h"

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

class Simulation {
public:
    Simulation(int object_count);

    struct Object {
        int mesh;
        Animation animation;
        Path path;

        uint32_t frame_data_offset;

        glm::mat4 model;
    };

    const std::vector<Object> &objects() const { return objects_; }

    unsigned int rng_seed() { return random_dev_(); }

    void set_frame_data_size(uint32_t size);
    void update(float obj_time, int begin, int end);

private:
    std::random_device random_dev_;
    std::vector<Object> objects_;
};

#endif // SIMULATION_H
