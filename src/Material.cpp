#include "Material.h"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Vector3f Material::shade(const Ray &ray,
                         const Hit &hit,
                         const Vector3f &dirToLight) {

    // Store the hit normal and incoming ray.
    Vector3f surfNormal = hit.getNormal();
    Vector3f eyeToSurf = ray.getDirection();

    // Calculate the diffuse component.
    float dot = Vector3f::dot(dirToLight, surfNormal);
    Vector3f diffuseLight = dot * _diffuseColor;

    // Calculate the specular component.
    Vector3f halfway = (dirToLight - eyeToSurf).normalized();
    float specularClamp = Vector3f::dot(surfNormal, halfway);
    specularClamp = fmax(0, specularClamp);
    Vector3f specularLight = pow(specularClamp, _shininess) * _specularColor;

    return diffuseLight + specularLight;
}
