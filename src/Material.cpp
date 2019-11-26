#include "Material.h"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Vector3f Material::shade(const Ray &ray,
                         const Hit &hit,
                         const Vector3f &dirToLight,
                         const Vector3f &lightIntensity) {

    Vector3f surfNormal = hit.getNormal();
    Vector3f eyeToSurf = ray.getDirection();
    Vector3f reflectedEye = (eyeToSurf - 2 * Vector3f::dot(eyeToSurf, surfNormal) * surfNormal).normalized();
    float specularClamp = Vector3f::dot(dirToLight, reflectedEye);
    specularClamp = specularClamp > 0 ? specularClamp : 0;

    Vector3f specularLight = pow(specularClamp, _shininess) * _specularColor;

    return _diffuseColor / M_PI + (_shininess + 2) * specularLight / (2 * M_PI);
}
