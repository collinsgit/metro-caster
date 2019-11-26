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
    Vector3f estimatePixel(const Ray &ray, float tmin, float length, int iters);

    void choosePath(const Ray &r, Object3D *light, float tmin, float length, float &prob_path,
            std::vector<Ray> &eye_path, std::vector<Hit> &eye_hits, std::vector<Ray> &light_path, std::vector<Hit> &light_hits) const;

    void tracePath(const Ray &r, float tmin, int length, float &prob_path, std::vector<Ray> &path, std::vector<Hit> &hits) const;

    Vector3f colorPath(float tmin, Object3D *light, const std::vector<Ray> &eye_path, std::vector<Hit> &eye_hits, const std::vector<Ray> &light_path, std::vector<Hit> &light_hits);

    ArgParser _args;
    SceneParser _scene;
};

#endif // RENDERER_H
