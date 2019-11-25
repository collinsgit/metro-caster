#include "Sampler.h"

#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Vector3f cosineWeightedHemisphere::sample(const Ray &ray, const Vector3f &normal) const {
    std::default_random_engine generator(rand());
    std::uniform_real_distribution<float> uniform(0.f, 1.f);

    float r = sqrt(uniform(generator));
    float v = 2.f * (float)M_PI * uniform(generator);

    // TODO: think of a better way to do this
    Vector3f x = Vector3f::cross(Vector3f(1., 2., 3.), normal).normalized();
    Vector3f y = Vector3f::cross(x, normal).normalized();

    float factor = sqrt(1-r*r);

    return r * normal + factor * (sin(v) * x + cos(v) * y);
}

float cosineWeightedHemisphere::pdf(const Vector3f &dir, const Vector3f &normal) const {
    float dot = Vector3f::dot(dir, normal);
    dot = dot < 0 ? 0 : dot;
    return dot / M_PI;
}

Vector3f pureReflectance::sample(const Ray &ray, const Vector3f &normal) const {
    return (ray.getDirection() - 2 * Vector3f::dot(ray.getDirection(), normal) * normal).normalized();
}

float pureReflectance::pdf(const Vector3f &dir, const Vector3f &normal) const {
    return 1;
}
