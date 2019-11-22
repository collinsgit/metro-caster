#include "Renderer.h"

#include "ArgParser.h"
#include "Camera.h"
#include "Image.h"
#include "Ray.h"
#include "VecUtils.h"

#include <limits>
#include <random>


Renderer::Renderer(const ArgParser &args) :
    _args(args),
    _scene(args.input_file)
{
}

Vector3f Renderer::estimatePixel(const Ray &ray, float tmin, int length, int iters)
{
    Vector3f color;

    // Average over multiple iterations
    for (int i=0; i<iters; i++) {

        // 1. Choose a light
        std::default_random_engine generator;
        std::uniform_int_distribution<int> uniform(0, _scene.lights.size()-1);
        Object3D* light = _scene.lights[uniform(generator)];

        std::vector<Ray> path = choosePath(ray, light, tmin, length);
        //std::cout << "path length " << path.size() << "\n";

        Vector3f path_color = colorPath(path, tmin);
        float path_prob = probPath(path);
        color += path_color / path_prob;
    }

    return color / (float)iters;
}

std::vector<Ray> Renderer::tracePath(Ray r,
        float tmin,
        int length) const
{
    assert(length >= 1);

    std::vector<Ray> path;
    // Ray r = light->emit();
    path.push_back(r);

    for (int i=1; i<length; i++) {
        Hit h;
        if(_scene.getGroup()->intersect(r, tmin, h)) {
            Vector3f o = r.pointAtParameter(h.getT());

            // TODO: modify direction to allow for reflection/refraction
            Vector3f d = r.getDirection() - 2 * Vector3f::dot(r.getDirection(), h.getNormal()) * h.getNormal();
            d.normalize();

            r = Ray(o, d);
            path.push_back(r);
        } else {
            break;
        }
    }

    return path;
}

std::vector<Ray> Renderer::choosePath(const Ray &r,
        Object3D* light,
        float tmin,
        int length) const
{

    // TODO: Correct hardcoded lengths
    // 2. Draw light path
    std::vector<Ray> light_path = tracePath(light->sample(), tmin, 2);
    //std::cout << "light path length " << light_path.size() << "\n";

    // 3. Draw eye path
    std::vector<Ray> path = tracePath(r, tmin, 3);
    //std::cout << "eye path length " << path.size() << "\n";

    // 4. Check for light path obstruction
    Ray last_eye = path[path.size()-1];
    Ray last_light = light_path[light_path.size()-1];

    Vector3f dir = last_light.getOrigin() - last_eye.getOrigin();
    Ray connector = Ray(last_eye.getOrigin(), dir.normalized());
    Hit connector_hit;
    _scene.getGroup()->intersect(connector, tmin, connector_hit);

    float eps = 0.01;
    if (connector_hit.getT() + eps < dir.abs()) {
        return path;
    }

    // 5. Build Complete Path
    path[path.size()-1] = connector;
    for (int i=(int)light_path.size()-1; i>0; i--) {
        path.emplace_back(light_path[i].getOrigin(),
                -light_path[i-1].getDirection());
    }

    return path;
}

Vector3f Renderer::colorPath(const std::vector<Ray> &path, float tmin)
{
    Vector3f dirToLight(0);
    Vector3f lightIntensity = _scene.getAmbientLight();
    //std::cout << "asdf;\n";
    for (int i=(int)path.size()-1; i>=0; i--) {
        //lightIntensity.print();
        Ray r = path[i];
        Hit h;
        if (_scene.getGroup()->intersect(r, tmin, h)) {
            //lightIntensity.print();
            if (i==path.size()-1) {
                dirToLight = (r.getDirection() - 2 * Vector3f::dot(r.getDirection(), h.getNormal()) * h.getNormal()).normalized();
            } else {
                dirToLight = path[i+1].getDirection();
            }

            lightIntensity = h.getMaterial()->shade(r, h, dirToLight, lightIntensity);
        } else {
            lightIntensity = _scene.getAmbientLight();
        }
    }

    return lightIntensity;
}

float Renderer::probPath(const std::vector<Ray> &path)
{
    float prob = 1; // 1. / (4 * M_PI);

    return prob;
}

void Renderer::Render()
{
    int w = _args.width;
    int h = _args.height;
    int iters = 5;

    Image image(w, h);
    Image nimage(w, h);
    Image dimage(w, h);

    // _scene.getAmbientLight().print();

    // loop through all the pixels in the image
    // generate all the samples

    // This look generates camera rays and calls traceRay.
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

            Hit hit;
            Vector3f color = estimatePixel(r, 0.01, _args.bounces, iters);

//            if (x == w/2 and y == h/2) {
//                color = estimatePixel(r, 0.01, _args.bounces, iters);
//            } else {
//                color = Vector3f(0.);
//            }
            // Vector3f color = traceRay(r, cam->getTMin(), _args.bounces, hit, 1.f);

            image.setPixel(x, y, color);

//            nimage.setPixel(x, y, (hit.getNormal() + 1.0f) / 2.0f);
//            float range = (_args.depth_max - _args.depth_min);
//            if (range) {
//                dimage.setPixel(x, y, Vector3f((hit.t - _args.depth_min) / range));
//            }
            //break;
        }
        //break;
    }
    // END SOLN

    // save the files 
    if (!_args.output_file.empty()) {
        image.savePNG(_args.output_file);
    }
//    if (_args.depth_file.size()) {
//        dimage.savePNG(_args.depth_file);
//    }
//    if (_args.normals_file.size()) {
//        nimage.savePNG(_args.normals_file);
//    }
}



//Vector3f Renderer::traceRay(const Ray &r,
//    float tmin,
//    int bounces,
//    Hit &h,
//    float refIndex) const
//{
//    // The starter code only implements basic drawing of sphere primitives.
//    // You will implement phong shading, recursive ray tracing, and shadow rays.
//    float eps = 0.01;
//
//    if (_scene.getGroup()->intersect(r, tmin, h)) {
//        Vector3f total_light = _scene.getAmbientLight() * h.getMaterial()->getDiffuseColor();
//
//        for (Light* light : _scene.lights) {
//            Vector3f tolight;
//            Vector3f intensity;
//            float distToLight;
//
//            light->getIllumination(r.pointAtParameter(h.getT()), tolight, intensity, distToLight);
//
//            if (_args.shadows) {
//                Vector3f shadow_origin = r.pointAtParameter(h.getT());
//                Vector3f shadow_dir = tolight.normalized();
//
//                Hit shadow_hit;
//                Ray shadow_ray(shadow_origin, shadow_dir);
//
//                if (_scene.getGroup()->intersect(shadow_ray, tmin > eps ? tmin : eps, shadow_hit)) {
//                    continue;
//                }
//            }
//
//            total_light += h.getMaterial()->shade(r, h, tolight, intensity);
//        }
//
//        if (bounces > 0) {
//            // reflection
//            Vector3f bounce_origin = r.pointAtParameter(h.getT());
//            Vector3f bounce_dir = r.getDirection() - 2 * Vector3f::dot(r.getDirection(), h.getNormal()) * h.getNormal();
//            bounce_dir.normalize();
//
//            Ray bounce_ray(bounce_origin, bounce_dir);
//            Hit bounce_h;
//            Vector3f bounce_color = traceRay(bounce_ray, tmin > eps ? tmin : eps, bounces-1, bounce_h, refIndex);
//            total_light += bounce_color * h.getMaterial()->getSpecularColor();
//
//            // refraction
//            if (_args.refraction && (h.getMaterial()->getTransColor().abs() > 0)) {
//                float matRefIndex = h.material->getRefIndex();
//                float relRefIndex = refIndex == matRefIndex ? matRefIndex : refIndex / matRefIndex;
//                Vector3f I = -r.getDirection();
//                Vector3f N = refIndex == matRefIndex ? -h.getNormal() : h.getNormal();
//                float vecDot = Vector3f::dot(I, N);
//
//                float refract_sqrt = 1 - relRefIndex * relRefIndex * (1 - vecDot * vecDot);
//                if (refract_sqrt > 0) {
//                    refract_sqrt = sqrt(refract_sqrt);
//                    Vector3f refract_dir = (relRefIndex * vecDot - refract_sqrt) * N - relRefIndex * I;
//
//                    Ray refract_ray(bounce_origin, refract_dir);
//                    Hit refract_h;
//                    Vector3f refract_color = traceRay(refract_ray, tmin > eps ? tmin : eps, bounces-1, refract_h, matRefIndex);
//                    total_light += refract_color * h.getMaterial()->getTransColor();
//                }
//            }
//        }
//
//        return total_light;
//    } else {
//        return _scene.getBackgroundColor(r.getDirection());
//    };
// }