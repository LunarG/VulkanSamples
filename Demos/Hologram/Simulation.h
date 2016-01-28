#ifndef SIMULATION_H
#define SIMULATION_H

#include <random>
#include <vector>

#include <glm/glm.hpp>

#include "Animation.h"
#include "Path.h"

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
