#include "Renderer.h"

#include "ArgParser.h"
#include "Camera.h"
#include "Image.h"
#include "Ray.h"
#include "VecUtils.h"

#include <limits>


Renderer::Renderer(const ArgParser &args) :
    _args(args),
    _scene(args.input_file)
{
}

void
Renderer::Render()
{
    int w = _args.width;
    int h = _args.height;

    Image image(w, h);
    Image nimage(w, h);
    Image dimage(w, h);

    // loop through all the pixels in the image
    // generate all the samples

    // This look generates camera rays and callse traceRay.
    // It also write to the color, normal, and depth images.
    // You should understand what this code does.
    Camera* cam = _scene.getCamera();
    for (int y = 0; y < h; ++y) {
        float ndcy = 2 * (y / (h - 1.0f)) - 1.0f;
        for (int x = 0; x < w; ++x) {
            float ndcx = 2 * (x / (w - 1.0f)) - 1.0f;
            // Use PerspectiveCamera to generate a ray.
            // You should understand what generateRay() does.
            Ray r = cam->generateRay(Vector2f(ndcx, ndcy));

            Hit h;
            Vector3f color = traceRay(r, cam->getTMin(), _args.bounces, h, 1.f);

            image.setPixel(x, y, color);
            nimage.setPixel(x, y, (h.getNormal() + 1.0f) / 2.0f);
            float range = (_args.depth_max - _args.depth_min);
            if (range) {
                dimage.setPixel(x, y, Vector3f((h.t - _args.depth_min) / range));
            }
        }
    }
    // END SOLN

    // save the files 
    if (_args.output_file.size()) {
        image.savePNG(_args.output_file);
    }
    if (_args.depth_file.size()) {
        dimage.savePNG(_args.depth_file);
    }
    if (_args.normals_file.size()) {
        nimage.savePNG(_args.normals_file);
    }
}



Vector3f
Renderer::traceRay(const Ray &r,
    float tmin,
    int bounces,
    Hit &h,
    float refIndex) const
{
    // The starter code only implements basic drawing of sphere primitives.
    // You will implement phong shading, recursive ray tracing, and shadow rays.
    float eps = 0.01;

    if (_scene.getGroup()->intersect(r, tmin, h)) {
        Vector3f total_light = _scene.getAmbientLight() * h.getMaterial()->getDiffuseColor();

        for (Light* light : _scene.lights) {
            Vector3f tolight;
            Vector3f intensity;
            float distToLight;

            light->getIllumination(r.pointAtParameter(h.getT()), tolight, intensity, distToLight);

            if (_args.shadows) {
                Vector3f shadow_origin = r.pointAtParameter(h.getT());
                Vector3f shadow_dir = tolight.normalized();

                Hit shadow_hit;
                Ray shadow_ray(shadow_origin, shadow_dir);

                if (_scene.getGroup()->intersect(shadow_ray, tmin > eps ? tmin : eps, shadow_hit)) {
                    continue;
                }
            }

            total_light += h.getMaterial()->shade(r, h, tolight, intensity);
        }

        if (bounces > 0) {
            // reflection
            Vector3f bounce_origin = r.pointAtParameter(h.getT());
            Vector3f bounce_dir = r.getDirection() - 2 * Vector3f::dot(r.getDirection(), h.getNormal()) * h.getNormal();
            bounce_dir.normalize();

            Ray bounce_ray(bounce_origin, bounce_dir);
            Hit bounce_h;
            Vector3f bounce_color = traceRay(bounce_ray, tmin > eps ? tmin : eps, bounces-1, bounce_h, refIndex);
            total_light += bounce_color * h.getMaterial()->getSpecularColor();

            // refraction
            if (_args.refraction && (h.getMaterial()->getTransColor().abs() > 0)) {
                float matRefIndex = h.material->getRefIndex();
                float relRefIndex = refIndex == matRefIndex ? matRefIndex : refIndex / matRefIndex;
                Vector3f I = -r.getDirection();
                Vector3f N = refIndex == matRefIndex ? -h.getNormal() : h.getNormal();
                float vecDot = Vector3f::dot(I, N);

                float refract_sqrt = 1 - relRefIndex * relRefIndex * (1 - vecDot * vecDot);
                if (refract_sqrt > 0) {
                    refract_sqrt = sqrt(refract_sqrt);
                    Vector3f refract_dir = (relRefIndex * vecDot - refract_sqrt) * N - relRefIndex * I;

                    Ray refract_ray(bounce_origin, refract_dir);
                    Hit refract_h;
                    Vector3f refract_color = traceRay(refract_ray, tmin > eps ? tmin : eps, bounces-1, refract_h, matRefIndex);
                    total_light += refract_color * h.getMaterial()->getTransColor();
                }
            }
        }

        return total_light;
    } else {
        return _scene.getBackgroundColor(r.getDirection());
    };
}

