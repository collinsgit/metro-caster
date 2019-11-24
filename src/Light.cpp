#include "Light.h"
#include "Ray.h"

#include <random>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void DirectionalLight::getIllumination(const Vector3f &p,
                                       Vector3f &tolight,
                                       Vector3f &intensity,
                                       float &distToLight) const {
    // the direction to the light is the opposite of the
    // direction of the directional light source

    // BEGIN STARTER
    tolight = -_direction;
    intensity = _color;
    distToLight = std::numeric_limits<float>::max();
    // END STARTER
}

Ray DirectionalLight::emit() const {
    Ray ray(Vector3f(0., 0., 0.), _direction);
    return ray;
}

void PointLight::getIllumination(const Vector3f &p,
                                 Vector3f &tolight,
                                 Vector3f &intensity,
                                 float &distToLight) const {
    // TODO Implement point light source
    // tolight, intensity, distToLight are outputs

    tolight = _position - p;
    distToLight = tolight.abs();
    tolight.normalize();
    intensity = _color / (_falloff * (float) pow(distToLight, 2));
}

Ray PointLight::emit() const {
    std::default_random_engine generator;
    std::uniform_real_distribution<float> theta_dist(0., 2 * M_PI);
    std::uniform_real_distribution<float> cosphi_dist(-1., 1.);

    float theta = theta_dist(generator);
    float cosphi = cosphi_dist(generator);

    Vector3f dir((1 - cosphi * cosphi) * cos(theta),
                 (1 - cosphi * cosphi) * sin(theta),
                 cosphi);

    Ray ray(_position, dir);
    return ray;
}
