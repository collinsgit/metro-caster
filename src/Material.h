#ifndef MATERIAL_H
#define MATERIAL_H

#include <cassert>
#include <vecmath.h>

#include "Ray.h"
#include "Image.h"
#include "Vector3f.h"

#include <string>

class Material {
public:
    Material(const Vector3f &diffuseColor,
             const Vector3f &specularColor = Vector3f::ZERO,
             const Vector3f &transColor = Vector3f::ZERO,
             const Vector3f &light = Vector3f::ZERO,
             float shininess = 1,
             float refIndex = 0
    ) :
            _diffuseColor(diffuseColor),
            _specularColor(specularColor),
            _shininess(shininess),
            _light(light),
            _transColor(transColor),
            _refIndex(refIndex) {}

    const Vector3f &getDiffuseColor() const {
        return _diffuseColor;
    }

    const Vector3f &getSpecularColor() const {
        return _specularColor;
    }

    const Vector3f &getTransColor() const {
        return _transColor;
    }

    const Vector3f &getLight() const {
        return _light;
    }

    const float &getShininess() const {
        return _shininess;
    }

    float getRefIndex() const {
        return _refIndex;
    }

    Vector3f shade(const Ray &ray,
                   const Hit &hit,
                   const Vector3f &dirToLight);

protected:
    Vector3f _diffuseColor;
    Vector3f _specularColor;
    Vector3f _transColor;
    Vector3f _light;
    float _shininess;
    float _refIndex;
};

#endif // MATERIAL_H
