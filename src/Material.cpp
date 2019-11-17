#include "Material.h"
Vector3f Material::shade(const Ray &ray,
    const Hit &hit,
    const Vector3f &dirToLight,
    const Vector3f &lightIntensity)
{
    // TODO implement Diffuse and Specular phong terms

    // diffuse
    Vector3f surfNormal = hit.getNormal();
    float diffuseClamp = Vector3f::dot(dirToLight, surfNormal);
    diffuseClamp = diffuseClamp > 0 ? diffuseClamp : 0;

    Vector3f diffuseLight = diffuseClamp * lightIntensity * _diffuseColor;

    // specular
    Vector3f eyeToSurf = (ray.getDirection() * hit.getT()).normalized();
    Vector3f reflectedEye = (eyeToSurf - 2 * Vector3f::dot(eyeToSurf, surfNormal) * surfNormal).normalized();
    float specularClamp = Vector3f::dot(dirToLight, reflectedEye);
    specularClamp = specularClamp > 0 ? specularClamp : 0;

    Vector3f specularLight = pow(specularClamp, _shininess) * lightIntensity * _specularColor;

    return diffuseLight + specularLight;
}
