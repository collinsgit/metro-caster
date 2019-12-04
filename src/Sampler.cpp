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

float cosineWeightedHemisphere::pdf(const Ray &ray, const Vector3f &dir, Hit &h) const {
    float dot = Vector3f::dot(dir, h.getNormal());
    dot = dot < 0 ? 0 : dot;
    return dot / M_PI;
}

Vector3f pureReflectance::sample(const Ray &ray, Hit &h) const {
    return (ray.getDirection() - 2 * Vector3f::dot(ray.getDirection(), h.getNormal()) * h.getNormal()).normalized();
}

float pureReflectance::pdf(const Ray &ray, const Vector3f &dir, Hit &h) const {
    return 1;
}

Vector3f blinnPhong::sample(const Ray &ray, Hit &h) const {
    float shininess = h.getMaterial()->getShininess();
    Vector3f diff = h.getMaterial()->getDiffuseColor();
    Vector3f spec = h.getMaterial()->getSpecularColor();
    float prob_spec = 1.f / (1.f + (diff[0] + diff[1] + diff[2]) / (spec[0] + spec[1] + spec[2]));
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

float blinnPhong::pdf(const Ray &ray, const Vector3f &dir, Hit &h) const {
    float shininess = h.getMaterial()->getShininess();
    Vector3f diff = h.getMaterial()->getDiffuseColor();
    Vector3f spec = h.getMaterial()->getSpecularColor();
    float prob_spec = 1.f / (1.f + (diff[0] + diff[1] + diff[2]) / (spec[0] + spec[1] + spec[2]));

    float diffuse_pdf = Vector3f::dot(dir, h.getNormal()) / M_PI;
    diffuse_pdf = diffuse_pdf > 0 ? diffuse_pdf : 0;


    Vector3f ref_dir = (ray.getDirection() - 2 * Vector3f::dot(ray.getDirection(), h.getNormal()) * h.getNormal()).normalized();
    float S = 0;
    float d = Vector3f::dot(ref_dir, h.getNormal());
    float c = sqrt(1-d*d);
    bool even = fmod(shininess, 2.) == 0.;
    float T = even ? 2.f*c : (float)M_PI;
    float A = even ? (float)2*(M_PI-acos(d)) : (float)M_PI;
    float i = even ? 1 : 0;

    while (i <= shininess-1) {
        S = S+T;
        T = T*c*c*(i+1.)/(i+2.);
        i = i+2;
    }

    float specular_norm = (A + d*(S)) / (shininess+1);
    float specular_pdf = pow(Vector3f::dot(ref_dir, dir), shininess) / specular_norm;
    specular_pdf = specular_pdf > 0 ? specular_pdf : 0;

    return prob_spec * specular_pdf + (1-prob_spec) * diffuse_pdf;

}

Vector3f experimental::sample(const Ray &ray, Hit &h) const {
    Vector3f diff = h.getMaterial()->getDiffuseColor();
    Vector3f spec = h.getMaterial()->getSpecularColor();
    float prob_spec = 1.f / (1.f + (diff[0] + diff[1] + diff[2]) / (spec[0] + spec[1] + spec[2]));

    std::default_random_engine generator(rand());
    std::uniform_real_distribution<float> uniform(0.f, 1.f);

    if (uniform(generator) > prob_spec) {
        Sampler* sampler = new cosineWeightedHemisphere;
        return sampler->sample(ray, h);
    } else {
        Sampler* sampler = new pureReflectance;
        return sampler->sample(ray, h);
    }
}

float experimental::pdf(const Ray &ray, const Vector3f &dir, Hit &h) const {
    Vector3f diff = h.getMaterial()->getDiffuseColor();
    Vector3f spec = h.getMaterial()->getSpecularColor();
    float prob_spec = 1.f / (1.f + (diff[0] + diff[1] + diff[2]) / (spec[0] + spec[1] + spec[2]));

    float pdf = 0;
    Sampler* sampler_diff = new cosineWeightedHemisphere;
    pdf += (1-prob_spec) * sampler_diff->pdf(ray, dir, h);
    Sampler* sampler_ref = new pureReflectance;
    pdf += prob_spec * sampler_ref->pdf(ray, dir, h);

    return pdf;
}

