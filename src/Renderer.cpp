#include "Renderer.h"

#include "ArgParser.h"
#include "Camera.h"
#include "Image.h"
#include "Ray.h"
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
        std::vector<Ray> path = choosePath(ray, light, tmin, length, hits);
        Vector3f path_color = colorPath(path, tmin, hits);
        float path_prob = probPath(path);
        color += path_color / path_prob;
    }

    return color / (float) iters;
}

Vector3f weightedCosineHemisphere(const Vector3f &normal) {
    std::default_random_engine generator(rand());
    std::uniform_real_distribution<float> uniform(0.f, 1.f);

    float r = sqrt(uniform(generator));
    float v = 2.f * (float)M_PI * uniform(generator);

    // TODO: think of a better way to do this
    Vector3f x = Vector3f::cross(Vector3f(1., 2., 3.), normal).normalized();
    Vector3f y = Vector3f::cross(x, normal).normalized();

    float factor = sqrt(1-r*r);

    return r * normal + factor * (sin(v) * x + cos(v) * y);
}

std::vector<Ray> Renderer::tracePath(Ray r,
                                     float tmin,
                                     int length,
                                     std::vector<Hit> &hits) const {
    assert(length >= 1);

    std::vector<Ray> path;
    path.push_back(r);

    for (int i = 1; i < length; i++) {
        Hit h;
        if (_scene.getGroup()->intersect(r, tmin, h)) {
            Vector3f o = r.pointAtParameter(h.getT());

            Vector3f d = weightedCosineHemisphere(h.getNormal());

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
                                      std::vector<Hit> &hits) const {
    std::default_random_engine generator(rand());
    std::poisson_distribution<int> poisson(length);

    // 2. Draw light path
    int light_length = 1 + poisson(generator);
    std::vector<Hit> light_hits;
    std::vector<Ray> light_path = tracePath(light->sample(), tmin, light_length, light_hits);

    // 3. Draw eye path
    int eye_length = 1 + poisson(generator);
    std::vector<Ray> path = tracePath(r, tmin, eye_length, hits);

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

    // 5. Build Complete Path
    path[path.size() - 1] = connector;
    for (int i = (int) light_path.size() - 1; i > 0; i--) {
        path.emplace_back(light_path[i].getOrigin(),
                          -light_path[i - 1].getDirection());
    }

    // Update the hits accordingly.
    hits.emplace_back(connector_hit);
    for (int i = (int) light_hits.size() - 1; i > 0; i--) {
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
            dirToLight = (r.getDirection() -
                          2 * Vector3f::dot(r.getDirection(), h.getNormal()) * h.getNormal()).normalized();
        } else {
            dirToLight = path[i + 1].getDirection();
        }
        lightIntensity = h.getMaterial()->shade(r, h, dirToLight, lightIntensity);
    }

    return lightIntensity;
}

float Renderer::probPath(const std::vector<Ray> &path) {
    float prob = 1; // 1. / (4 * M_PI);

    return prob;
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
    for (int y = 0; y < h; ++y) {
        float ndcy = 2 * (y / (h - 1.0f)) - 1.0f;
        for (int x = 0; x < w; ++x) {
            float ndcx = 2 * (x / (w - 1.0f)) - 1.0f;
            // Use PerspectiveCamera to generate a ray.
            Ray r = cam->generateRay(Vector2f(ndcx, ndcy));

            Hit hit;
            Vector3f color = estimatePixel(r, 0.01, length, iters);

            image.setPixel(x, y, color);
        }
    }

    // Save the output file.
    if (!_args.output_file.empty()) {
        image.savePNG(_args.output_file);
    }
}
