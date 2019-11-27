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
    // Average over multiple iterations.
    Vector3f color;
    parallel_for(iters, [&](int start, int end) {
        for (int i = start; i < end; i++) {
            // 1. Choose a light
            std::default_random_engine generator(rand());
            std::uniform_int_distribution<int> uniform(0, _scene.lights.size() - 1);
            Object3D *light = _scene.lights[uniform(generator)];

            float prob_path = 1;

            std::vector<Ray> eye_path;
            std::vector<Ray> light_path;
            std::vector<Hit> eye_hits;
            std::vector<Hit> light_hits;

            choosePath(ray, light, tmin, length, prob_path, eye_path, eye_hits, light_path, light_hits);
            Vector3f path_color = colorPath(tmin, light, eye_path, eye_hits, light_path, light_hits);
            // TODO: Include probability factor
            // color += path_color / prob_path;
            color += path_color;
        }
    }, length > 100);
    return color / (float) iters;
}

void Renderer::tracePath(const Ray &r, float tmin, int length, float &prob_path, std::vector<Ray> &path, std::vector<Hit> &hits) const {
    assert(length >= 1);

    Ray ray = r;
    path.push_back(r);

    for (int i = 1; i < length; i++) {
        Hit h;
        if (_scene.getGroup()->intersect(ray, tmin, h)) {
            Vector3f o = ray.pointAtParameter(h.getT());
            Vector3f d = _scene.sampler->sample(ray, h);
            prob_path *= _scene.sampler->pdf(d, h);

            ray = Ray(o, d);
            path.push_back(ray);
            hits.push_back(h);
        } else {
            break;
        }
    }
}

void Renderer::choosePath(const Ray &r, Object3D *light, float tmin, float length, float &prob_path,
                std::vector<Ray> &eye_path, std::vector<Hit> &eye_hits, std::vector<Ray> &light_path, std::vector<Hit> &light_hits) const {
    std::default_random_engine generator(rand());
    std::geometric_distribution<int> geometric(1. / length);

    // 2. Draw light path
    int light_length = 1 + geometric(generator);
    float light_prob = 1;
    tracePath(light->sample(), tmin, light_length, light_prob, light_path, light_hits);

    // 3. Draw eye path
    int eye_length = 2 + geometric(generator);
    float eye_prob = 1;
    tracePath(r, tmin, eye_length, eye_prob, eye_path, eye_hits);
    // prob_path *= eye_prob;

    // 4. Check for light path obstruction
//    Ray last_eye = path[path.size() - 1];
//    Ray last_light = light_path[light_path.size() - 1];
//
//    Vector3f dir = last_light.getOrigin() - last_eye.getOrigin();
//    Ray connector = Ray(last_eye.getOrigin(), dir.normalized());
//    Hit connector_hit;
//    _scene.getGroup()->intersect(connector, tmin, connector_hit);
//
//    float eps = 0.01;
//    if (connector_hit.getT() + eps < dir.abs()) {
//        return path;
//    }
//    prob_path *= light_prob;
//
//    // 5. Build Complete Path
//    path[path.size() - 1] = connector;
//    for (int i = (int) light_path.size() - 1; i > 0; i--) {
//        path.emplace_back(light_path[i].getOrigin(),
//                          -light_path[i - 1].getDirection());
//    }
//
//    // Update the hits accordingly.
//   hits.emplace_back(connector_hit);
//    for (int i = (int) light_hits.size() - 2; i >= 0; i--) {
//        hits.emplace_back(light_hits[i]);
//    }
//
//    return path;
}

Vector3f Renderer::colorPath(float tmin, Object3D *light, const std::vector<Ray> &eye_path, std::vector<Hit> &eye_hits, const std::vector<Ray> &light_path, std::vector<Hit> &light_hits) {
    // check connector
    Ray last_eye = eye_path[eye_path.size() - 1];
    Ray last_light = light_path[light_path.size() - 1];

    Vector3f dir = last_light.getOrigin() - last_eye.getOrigin();
    Ray connector = Ray(last_eye.getOrigin(), dir.normalized());
    Hit connector_hit;
    _scene.getGroup()->intersect(connector, tmin, connector_hit);

    if (connector_hit.getT() + tmin < dir.abs()) {
        return Vector3f(0);
    }

    // Set up the intensity and light directions.
    Vector3f lightIntensity = light->getMaterial()->getLight();

    std::vector<float> eye_probs;
    std::vector<float> light_probs;

    float pdf_w;
    Vector3f bsdf(0.);
    Ray incoming(Vector3f(0.), Vector3f(0.));
    Vector3f outgoing(0.);
    Hit hit;

    for (int i=0; i<=(int)eye_path.size()-2; i++) {
        incoming = eye_path[i];
        hit = eye_hits[i];

        if (i==eye_path.size()-2) {
            outgoing = connector.getDirection();
            pdf_w = 1;
        } else {
            outgoing = eye_path[i+1].getDirection();
            pdf_w = _scene.sampler->pdf(outgoing, hit);
        }

        float dot = 1; // Vector3f::dot(hit.getNormal(), outgoing);

        bsdf = hit.getMaterial()->shade(incoming, hit, outgoing);

        lightIntensity = bsdf * dot * lightIntensity; // / pdf_w;
    }

    for (int i=0; i<(int)light_path.size()-2; i++) {
        incoming = light_path[i];
        hit = light_hits[i];

        if (i==light_path.size()-2) {
            outgoing = -connector.getDirection();
            pdf_w = 1;
        } else {
            outgoing = light_path[i+1].getDirection();
            pdf_w = _scene.sampler->pdf(outgoing, hit);
        }

        float dot = 1; // Vector3f::dot(hit.getNormal(), outgoing);

        bsdf = hit.getMaterial()->shade(incoming, hit, outgoing);

        lightIntensity = bsdf * dot * lightIntensity; // / pdf_w;
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
                    Vector3f color = estimatePixel(r, 0.01, length, iters);
                    image.setPixel(j, i, color);
                }
            }, false);
        }
    }, false);

    // Save the output file.
    if (!_args.output_file.empty()) {
        image.savePNG(_args.output_file);
    }
}
