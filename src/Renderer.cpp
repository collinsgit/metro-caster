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


// Beta value for MIS. The value below is recommended by E. Veach.
const int MIS_BETA = 2.f;

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
            color += path_color;
        }
    }, length > 100);
    return color / (float) iters;
}

void Renderer::tracePath(const Ray &r, float tmin, int length, float &prob_path, std::vector<Ray> &path,
                         std::vector<Hit> &hits) const {
    assert(length >= 1);

    Ray ray = r;
    path.push_back(r);

    for (int i = 1; i < length; i++) {
        Hit h;
        if (_scene.getGroup()->intersect(ray, tmin, h)) {
            Vector3f o = ray.pointAtParameter(h.getT());
            Vector3f d = _scene.sampler->sample(ray, h);
            prob_path *= _scene.sampler->pdf(ray, d, h);

            ray = Ray(o, d);
            path.push_back(ray);
            hits.push_back(h);
        } else {
            break;
        }
    }
}

void Renderer::choosePath(const Ray &r, Object3D *light, float tmin, float length, float &prob_path,
                          std::vector<Ray> &eye_path, std::vector<Hit> &eye_hits, std::vector<Ray> &light_path,
                          std::vector<Hit> &light_hits) const {
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
}

void Renderer::precomputeCumulativeBSDF(const std::vector<Ray> &path,
                                        const std::vector<Hit> &hits,
                                        std::vector<Vector3f> &bsdf,
                                        std::vector<float> &pdf) {

    // Find each BSDF iteratively.
    for (unsigned long i = 0; i < path.size() - 2; i++) {
        float currentPDF = _scene.sampler->pdf(path[i], path[i + 1].getDirection(), const_cast<Hit &>(hits[i]));
        Vector3f currentBSDF = (hits[i].getMaterial()->shade(path[i], hits[i], path[i + 1].getDirection()))
                               / currentPDF;
        if (i == 0) {
            bsdf.emplace_back(currentBSDF);
            pdf.emplace_back(pow(currentPDF, MIS_BETA));
        } else {
            bsdf.emplace_back(bsdf[i - 1] * currentBSDF);
            pdf.emplace_back(pdf[i-1]*pow(currentPDF, MIS_BETA));
        }
        // Also multiplying by:
        // Vector3f::dot(hit.getNormal(), outgoing);
        // Will be good once that's added.
    }
}

Vector3f Renderer::colorPath(float tmin, Object3D *light, const std::vector<Ray> &eye_path, std::vector<Hit> &eye_hits,
                             const std::vector<Ray> &light_path, std::vector<Hit> &light_hits) {

    // First, pre-compute the BSDF and weights for each component of the paths.
    std::vector<float> eye_pdfs, light_pdfs;
    std::vector<Vector3f> eye_bsdf, light_bsdf;
    if (eye_path.size() > 2) {
        precomputeCumulativeBSDF(eye_path, eye_hits, eye_bsdf, eye_pdfs);
    }
    if (light_path.size() > 2) {
        precomputeCumulativeBSDF(light_path, light_hits, light_bsdf, light_pdfs);
    }

    // For each combination, find the intensity; average once all are found.
    Vector3f intensity;
    float overallDensity = 0;
    for (unsigned long i = 2; i <= eye_path.size(); i++) {
        for (unsigned long j = 1; j <= light_path.size(); j++) {
            intensity += colorPathCombination(tmin, light, eye_path, eye_hits, eye_bsdf, eye_pdfs, light_path,
                                              light_hits, light_bsdf, light_pdfs, i, j, overallDensity);
        }
    }
    return intensity / overallDensity;
}

Vector3f Renderer::colorPathCombination(float tmin, Object3D *light, const std::vector<Ray> &eye_path,
                                        const std::vector<Hit> &eye_hits, const std::vector<Vector3f> &eye_bsdf,
                                        const std::vector<float> &eye_pdfs,
                                        const std::vector<Ray> &light_path, const std::vector<Hit> &light_hits,
                                        const std::vector<Vector3f> &light_bsdf,
                                        const std::vector<float> &light_pdfs, unsigned long eye_length,
                                        unsigned long light_length, float &overallDensity) {

    // First, create a connector between the end of the eye segment and the beginning of the light segment.
    Ray last_eye = eye_path[eye_length - 1];
    Ray last_light = light_path[light_length - 1];
    Vector3f connectorDir = last_light.getOrigin() - last_eye.getOrigin();
    Ray connector = Ray(last_eye.getOrigin(), connectorDir.normalized());

    // Find an intersection with the connector and the scene.
    Hit connector_hit;
    _scene.getGroup()->intersect(connector, tmin, connector_hit);

    // Calculate the overall light intensity.
    // Start off with the initial emitted light, eye path, and light path.
    Vector3f lightIntensity = light->getMaterial()->getLight();
    float weight = 1;
    if (eye_length >= 3) {
        lightIntensity = lightIntensity * eye_bsdf[eye_length - 3];
        weight = weight * eye_pdfs[eye_length - 3];
    }
    if (light_length >= 3) {
        lightIntensity = lightIntensity * light_bsdf[light_length - 3];
        weight = weight * light_pdfs[light_length - 3];
    }

    // Terminate early if there is an intersection with the scene.
    if (connector_hit.getT() + tmin < connectorDir.abs()) {
        overallDensity += weight;
        return Vector3f::ZERO;
    }

    // Consider the connector (the PDF of the connector is 1).
    // Add the connector's contribution to the eye path.
    Hit lastEyeHit = eye_hits[eye_length - 2];
    Vector3f lastEye_bsdf = lastEyeHit.getMaterial()->shade(eye_path[eye_length - 2], lastEyeHit,
                                                            connector.getDirection());
    float eyeDot = 1; // Vector3f::dot(lastEyeHit.getNormal(), connector.getDirection());
    lightIntensity = lightIntensity * lastEye_bsdf * eyeDot;

    // Add the connector's contribution to the light path and return.
    // In the case where the light length is 1 we can ignore this.
    if (light_length >= 2) {
        Hit lastLightHit = light_hits[light_length - 2];
        Vector3f lastLight_bsdf = lastLightHit.getMaterial()->shade(light_path[light_length - 2], lastLightHit,
                                                                    -connector.getDirection());
        float lightDot = 1; // Vector3f::dot(lastLightHit.getNormal(), -connector.getDirection());
        lightIntensity = lightIntensity * lastLight_bsdf * lightDot;
    }

    // Add on light found on the way
    for (int i=0; i<eye_length-1; i++) {
        Hit h = eye_hits[i];
        if (h.getMaterial()->getLight() != Vector3f::ZERO) {
            Vector3f bsdf = i > 0 ? eye_bsdf[i-1] : Vector3f(1.);
            lightIntensity += bsdf * h.getMaterial()->getLight();
        }
    }

    // Record the weight, apply it, and return.
    overallDensity += weight;
    return weight*lightIntensity;
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
            }, true);
        }
    }, true);

    // Save the output file.
    if (!_args.output_file.empty()) {
        image.savePNG(_args.output_file);
    }
}
