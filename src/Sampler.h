#ifndef METROCASTER_SAMPLER_H
#define METROCASTER_SAMPLER_H

#include "Ray.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


class Sampler {
public:
    Sampler() {};
    virtual ~Sampler() {};

    virtual Vector3f sample(const Ray &ray, const Vector3f &normal) const {
        return Vector3f(0);
    };

    virtual float pdf(const Vector3f &dir, const Vector3f &normal) const {
        return 1 / (4 * M_PI);
    }
};

class cosineWeightedHemisphere : public Sampler {
public:
    virtual Vector3f sample(const Ray &ray, const Vector3f &normal) const override;
    virtual float pdf(const Vector3f &dir, const Vector3f &normal) const override;
};

class pureReflectance : public Sampler {
public:
    virtual Vector3f sample(const Ray &ray, const Vector3f &normal) const override;
    virtual float pdf(const Vector3f &dir, const Vector3f &normal) const override;
};


#endif //METROCASTER_SAMPLER_H
