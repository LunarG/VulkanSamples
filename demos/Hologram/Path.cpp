#include <cassert>
#include <cmath>
#include "Path.h"

class Curve {
public:
    virtual ~Curve() {}
    virtual glm::vec3 evaluate(float t) = 0;
};

namespace {

enum CurveType {
    CURVE_RANDOM,
    CURVE_CIRCLE,
    CURVE_COUNT,
};

class RandomCurve : public Curve {
public:
    RandomCurve(unsigned int rng_seed)
        : rng_(rng_seed), direction_(-0.5f, 0.5f), duration_(0.5f, 5.0f),
          segment_start_(0.0f), segment_direction_(0.0f),
          time_start_(0.0f), time_duration_(0.0f)
    {
    }

    glm::vec3 evaluate(float t)
    {
        if (t >= time_start_ + time_duration_)
            new_segment(t);

        pos_ += unit_dir_ * (t - last_);
        last_ = t;

        return pos_;
    }

private:
    void new_segment(float time_start)
    {
        segment_start_ += segment_direction_;
        segment_direction_ = glm::vec3(direction_(rng_),
                                       direction_(rng_),
                                       direction_(rng_));

        time_start_ = time_start;
        time_duration_ = duration_(rng_);

        unit_dir_ = segment_direction_ / time_duration_;
        pos_ = segment_start_;
        last_ = time_start_;
    }

    std::mt19937 rng_;
    std::uniform_real_distribution<float> direction_;
    std::uniform_real_distribution<float> duration_;

    glm::vec3 segment_start_;
    glm::vec3 segment_direction_;
    float time_start_;
    float time_duration_;

    glm::vec3 unit_dir_;
    glm::vec3 pos_;
    float last_;
};

class CircleCurve : public Curve {
public:
    CircleCurve(float radius, glm::vec3 axis)
        : r_(radius)
    {
        glm::vec3 a;

        if (axis.x != 0.0f) {
            a.x = -axis.z / axis.x;
            a.y = 0.0f;
            a.z = 1.0f;
        } else if (axis.y != 0.0f) {
            a.x = 1.0f;
            a.y = -axis.x / axis.y;
            a.z = 0.0f;
        } else {
            a.x = 1.0f;
            a.y = 0.0f;
            a.z = -axis.x / axis.z;
        }

        a_ = glm::normalize(a);
        b_ = glm::normalize(glm::cross(a_, axis));
    }

    glm::vec3 evaluate(float t)
    {
        return (a_ * (glm::vec3(std::cos(t)) - glm::vec3(1.0f)) + b_ * glm::vec3(std::sin(t))) *
            glm::vec3(r_);
    }

private:
    float r_;
    glm::vec3 a_;
    glm::vec3 b_;
};

} // namespace

Path::Path(unsigned int rng_seed)
    : rng_(rng_seed), type_(0, CURVE_COUNT - 1), duration_(5.0f, 20.0f)
{
    // trigger a subpath generation
    current_.end = -1.0f;
}

glm::vec3 Path::position(float t)
{
    while (t >= current_.end)
        generate_subpath(t);

    return current_.origin + current_.curve->evaluate(t - current_.start);
}

void Path::generate_subpath(float t)
{
    float duration = duration_(rng_);
    CurveType type = static_cast<CurveType>(type_(rng_));

    if (current_.curve) {
        current_.origin += current_.curve->evaluate(current_.end - current_.start);
        current_.start = current_.end;
    } else {
        std::uniform_real_distribution<float> origin(0.0f, 2.0f);
        current_.origin = glm::vec3(origin(rng_), origin(rng_), origin(rng_));
        current_.start = t;
    }

    current_.end = current_.start + duration;

    Curve *curve;

    switch (type) {
    case CURVE_RANDOM:
        curve = new RandomCurve(rng_());
        break;
    case CURVE_CIRCLE:
        {
            std::uniform_real_distribution<float> dir(-1.0f, 1.0f);
            glm::vec3 axis(dir(rng_), dir(rng_), dir(rng_));
            if (axis.x == 0.0f && axis.y == 0.0f && axis.z == 0.0f)
                axis.x = 1.0f;

            std::uniform_real_distribution<float> radius_(0.01f, 0.5f);
            curve = new CircleCurve(radius_(rng_), axis);
        }
        break;
    default:
        assert(!"unreachable");
        curve = nullptr;
        break;
    }

    current_.curve.reset(curve);
}
