#include "Sampler.h"
#include "Material.h"

#include <random>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Vector3f cosineWeightedHemisphere::sample(const Ray &ray, Hit &h) const {
    std::default_random_engine generator(rand());
    std::uniform_real_distribution<float> uniform(0.f, 1.f);

    float r = sqrt(uniform(generator));
    float v = 2.f * (float)M_PI * uniform(generator);
    Vector3f normal = h.getNormal();

    // TODO: think of a better way to do this
    Vector3f x = Vector3f::cross(Vector3f(1., 2., 3.), normal).normalized();
    Vector3f y = Vector3f::cross(x, normal).normalized();

    float factor = sqrt(1-r*r);

    return r * normal + factor * (sin(v) * x + cos(v) * y);
}

float cosineWeightedHemisphere::pdf(const Vector3f &dir, Hit &h) const {
    float dot = Vector3f::dot(dir, h.getNormal());
    dot = dot < 0 ? 0 : dot;
    return dot / M_PI;
}

Vector3f pureReflectance::sample(const Ray &ray, Hit &h) const {
    return (ray.getDirection() - 2 * Vector3f::dot(ray.getDirection(), h.getNormal()) * h.getNormal()).normalized();
}

float pureReflectance::pdf(const Vector3f &dir, Hit &h) const {
    return 1;
}

Vector3f blinnPhong::sample(const Ray &ray, Hit &h) const {
    float shininess = h.getMaterial()->getShininess();
    float prob_spec = (2 + shininess) / (5 + shininess);
    int max_iters = 10;

    std::default_random_engine generator(rand());
    std::uniform_real_distribution<float> uniform(0.f, 1.f);

    float u = uniform(generator);
    float v = 2.f * (float)M_PI * uniform(generator);
    float sin_v = sin(v);
    float cos_v = cos(v);

    float r;
    float factor;
    Vector3f normal;
    Vector3f x;
    Vector3f y;
    Vector3f output = -h.getNormal();

    if (uniform(generator) > prob_spec) {
        // do diffuse distribution
        r = sqrt(u);
        factor = sqrt(1-r*r);
        normal = h.getNormal();

        x = Vector3f::cross(Vector3f(1., 2., 3.), normal).normalized();
        y = Vector3f::cross(x, normal).normalized();
        output = r * normal + factor * (sin_v * x + cos_v * y);
    } else {
        // do specular distribution
        normal = (ray.getDirection() -
                  2 * Vector3f::dot(ray.getDirection(), h.getNormal()) * h.getNormal()).normalized();
        x = Vector3f::cross(Vector3f(1., 2., 3.), normal).normalized();
        y = Vector3f::cross(x, normal).normalized();

        for (int i=0; i<max_iters; i++) {
            r = pow(u, 1.f / (1.f + shininess));
            factor = sqrt(1 - r * r);
            output = r * normal + factor * (sin_v * x + cos_v * y);

            if (Vector3f::dot(output, h.getNormal()) > 0) {
                return output;
            }

            u = uniform(generator);
            v = fmod(v + 0.4, 2 * M_PI);
            sin_v = sin(v); cos_v = cos(v);
        }

        r = sqrt(u);
        factor = sqrt(1-r*r);
        normal = h.getNormal();
        x = Vector3f::cross(Vector3f(1., 2., 3.), normal).normalized();
        y = Vector3f::cross(x, normal).normalized();
        output = r * normal + factor * (sin_v * x + cos_v * y);
    }

    return output;
}

float blinnPhong::pdf(const Vector3f &dir, Hit &h) const {
    return 1;
}
