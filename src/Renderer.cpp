#include "Renderer.h"

#include "ArgParser.h"
#include "Camera.h"
#include "Image.h"
#include "Ray.h"
#include "iterator.h"
#include "VecUtils.h"

#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


Renderer::Renderer(const ArgParser &args) :
        _args(args),
        _scene(args.input_file) {
}

Vector3f Renderer::estimatePixel(const Ray &ray, float tmin, float length, int iters) {
    Vector3f color;

    // Average over multiple iterations
    for (int i = 0; i < iters; i++) {

        // 1. Choose a light
        std::default_random_engine generator;
        std::uniform_int_distribution<int> uniform(0, _scene.lights.size() - 1);
        Object3D *light = _scene.lights[uniform(generator)];

        std::vector<Hit> hits;
        float prob_path = 1;
        std::vector<Ray> path = choosePath(ray, light, tmin, length, prob_path, hits);
        Vector3f path_color = colorPath(path, tmin, hits);
        // TODO: Include probability factor
        // color += path_color / prob_path;
        color += path_color;
    }

    return color / (float) iters;
}

std::vector<Ray> Renderer::tracePath(Ray r,
                                     float tmin,
                                     int length,
                                     float &prob_path,
                                     std::vector<Hit> &hits) const {
    assert(length >= 1);

    std::vector<Ray> path;
    path.push_back(r);

    for (int i = 1; i < length; i++) {
        Hit h;
        if (_scene.getGroup()->intersect(r, tmin, h)) {
            Vector3f o = r.pointAtParameter(h.getT());

            Vector3f d = _scene.sampler->sample(r, h.getNormal());
            prob_path *= _scene.sampler->pdf(d, h.getNormal());

            r = Ray(o, d);
            path.push_back(r);
            hits.push_back(h);
        } else {
            break;
        }
    }

    return path;
}

std::vector<Ray> Renderer::choosePath(const Ray &r,
                                      Object3D *light,
                                      float tmin,
                                      float length,
                                      float &prob_path,
                                      std::vector<Hit> &hits) const {
    std::default_random_engine generator(rand());
    std::poisson_distribution<int> poisson(length);

    // 2. Draw light path
    int light_length = 1 + poisson(generator);
    std::vector<Hit> light_hits;
    float light_prob = 1;
    std::vector<Ray> light_path = tracePath(light->sample(), tmin, light_length, light_prob, light_hits);

    // 3. Draw eye path
    int eye_length = 1 + poisson(generator);
    float eye_prob = 1;
    std::vector<Ray> path = tracePath(r, tmin, eye_length, eye_prob, hits);
    prob_path *= eye_prob;

    // 4. Check for light path obstruction
    Ray last_eye = path[path.size() - 1];
    Ray last_light = light_path[light_path.size() - 1];

    Vector3f dir = last_light.getOrigin() - last_eye.getOrigin();
    Ray connector = Ray(last_eye.getOrigin(), dir.normalized());
    Hit connector_hit;
    _scene.getGroup()->intersect(connector, tmin, connector_hit);

    float eps = 0.01;
    if (connector_hit.getT() + eps < dir.abs()) {
        return path;
    }
    prob_path *= light_prob;

    // 5. Build Complete Path
    path[path.size() - 1] = connector;
    for (int i = (int) light_path.size() - 1; i > 0; i--) {
        path.emplace_back(light_path[i].getOrigin(),
                          -light_path[i - 1].getDirection());
    }

    // Update the hits accordingly.
   hits.emplace_back(connector_hit);
    for (int i = (int) light_hits.size() - 2; i >= 0; i--) {
        hits.emplace_back(light_hits[i]);
    }

    return path;
}

Vector3f Renderer::colorPath(const std::vector<Ray> &path, float tmin, std::vector<Hit> hits) {
    // Set up the intensity and light directions.
    Vector3f dirToLight(0);
    Vector3f lightIntensity = _scene.getAmbientLight();

    // Add the last hit to the light source in.
    Ray rayToLight = path[path.size() - 1];
    Hit lightSourceHit;
    _scene.getGroup()->intersect(rayToLight, tmin, lightSourceHit);
    hits.emplace_back(lightSourceHit);

    // Iterate through the paths and hits.
    for (int i = (int) path.size() - 1; i >= 0; i--) {
        // Fetch the necessary ray and hit, then update the light intensity.
        Ray r = path[i];
        Hit h = hits[i];
        if (i == path.size() - 1) {
            dirToLight = _scene.sampler->sample(r, h.getNormal());
        } else {
            dirToLight = path[i + 1].getDirection();
        }
        lightIntensity = h.getMaterial()->shade(r, h, dirToLight, lightIntensity);
    }

    return lightIntensity;
}

void Renderer::Render() {
    // Loop through all the pixels in the image
    // generate all the samples. Fetch necessary args.
    int w = _args.width;
    int h = _args.height;
    int iters = _args.iters;
    float length = _args.length;

    // This look generates camera rays and calls traceRay.
    // It also write to the color image.
    Image image(w, h);
    Camera *cam = _scene.getCamera();
    parallel_for(h, [&](int start, int end) {
        for (int i = start; i < end; ++i) {
            float ndcy = 2 * (i / (h - 1.0f)) - 1.0f;
            parallel_for(w, [&](int innerStart, int innerEnd) {
                for (int j = innerStart; j < innerEnd; ++j) {
                    // Use PerspectiveCamera to generate a ray.
                    float ndcx = 2 * (j / (w - 1.0f)) - 1.0f;
                    Ray r = cam->generateRay(Vector2f(ndcx, ndcy));
                    Hit hit;
                    Vector3f color = estimatePixel(r, 0.01, length, iters);
                    image.setPixel(j, i, color);
                }
            });
        }
    });

    // Save the output file.
    if (!_args.output_file.empty()) {
        image.savePNG(_args.output_file);
    }
}
