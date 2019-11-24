#ifndef RENDERER_H
#define RENDERER_H

#include <string>

#include "Ray.h"
#include "SceneParser.h"
#include "ArgParser.h"

class Hit;

class Vector3f;

class Ray;

class Renderer {
public:
    // Instantiates a renderer for the given scene.
    Renderer(const ArgParser &args);

    void Render();

private:
    Vector3f traceRay(const Ray &ray, float tmin, int bounces,
                      Hit &hit, float refIndex) const;

    Vector3f estimatePixel(const Ray &ray, float tmin, int length, int iters);

    std::vector<Ray> choosePath(const Ray &ray, Object3D *light, float tmin, int length) const;

    std::vector<Ray> tracePath(Ray ray, float tmin, int length) const;

    Vector3f colorPath(const std::vector<Ray> &path, float tmin);

    float probPath(const std::vector<Ray> &path);

    ArgParser _args;
    SceneParser _scene;
};

#endif // RENDERER_H
