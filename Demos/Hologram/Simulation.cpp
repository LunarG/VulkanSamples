#include <glm/gtc/matrix_transform.hpp>
#include "Simulation.h"

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
