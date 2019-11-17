#include "Object3D.h"
#include "VecUtils.h"
#include "quartic.cpp"

bool Sphere::intersect(const Ray &r, float tmin, Hit &h) const
{
    // BEGIN STARTER

    // We provide sphere intersection code for you.
    // You should model other intersection implementations after this one.

    // Locate intersection point ( 2 pts )
    const Vector3f &rayOrigin = r.getOrigin(); //Ray origin in the world coordinate
    const Vector3f &dir = r.getDirection();

    Vector3f origin = rayOrigin - _center;      //Ray origin in the sphere coordinate

    float a = dir.absSquared();
    float b = 2 * Vector3f::dot(dir, origin);
    float c = origin.absSquared() - _radius * _radius;

    // no intersection
    if (b * b - 4 * a * c < 0) {
        return false;
    }

    float d = sqrt(b * b - 4 * a * c);

    float tplus = (-b + d) / (2.0f*a);
    float tminus = (-b - d) / (2.0f*a);

    // the two intersections are at the camera back
    if ((tplus < tmin) && (tminus < tmin)) {
        return false;
    }

    float t = 10000;
    // the two intersections are at the camera front
    if (tminus > tmin) {
        t = tminus;
    }

    // one intersection at the front. one at the back 
    if ((tplus > tmin) && (tminus < tmin)) {
        t = tplus;
    }

    if (t < h.getT()) {
        Vector3f normal = r.pointAtParameter(t) - _center;
        normal = normal.normalized();
        h.set(t, this->material, normal);
        return true;
    }
    // END STARTER
    return false;
}

// Add object to group
void Group::addObject(Object3D *obj) {
    m_members.push_back(obj);
}

// Return number of objects in group
int Group::getGroupSize() const {
    return (int)m_members.size();
}

bool Group::intersect(const Ray &r, float tmin, Hit &h) const
{
    // BEGIN STARTER
    // we implemented this for you
    bool hit = false;
    for (Object3D* o : m_members) {
        if (o->intersect(r, tmin, h)) {
            hit = true;
        }
    }
    return hit;
    // END STARTER
}


bool Plane::intersect(const Ray &r, float tmin, Hit &h) const
{
    // t = (d - r_0 . n) / (r_d . n)
    float t = (_d - Vector3f::dot(r.getOrigin(), _normal)) / Vector3f::dot(r.getDirection(), _normal);

    if ((t > tmin) && (t < h.getT())) {
        h.set(t, this->material, _normal);
        return true;
    }

    return false;
}

bool Triangle::intersect(const Ray &r, float tmin, Hit &h) const 
{
    Matrix3f A(_v[0] - _v[1], _v[0] - _v[2], r.getDirection(), true);
    Vector3f b = _v[0] - r.getOrigin();
    Vector3f x = A.inverse() * b;
    float beta = x[0];
    float gamma = x[1];
    float t = x[2];

    if ((beta > 0) && (gamma > 0) && (beta + gamma < 1) && (t > tmin) && (t < h.getT())) {
        Vector3f normal = (1 - beta - gamma) * _normals[0] + beta * _normals[1] + gamma * _normals[2];
        normal.normalize();
        h.set(t, this->material, normal);
        return true;
    }

    return false;
}


bool Torus::intersect(const Ray &r, float tmin, Hit &h) const
{
    // method adapted from https://github.com/sasamil/Quartic
    // and http://www.cosinekitty.com/raytrace/chapter13_torus.html
    Vector3f ray_dir = r.getDirection();
    Vector3f ray_org = r.getOrigin();

    float sum_d_sqrd = ray_dir.absSquared();
    float e = ray_org.absSquared() - _r * _r - _R * _R;
    float f = Vector3f::dot(ray_org, ray_dir);
    float four_a_sqrd = 4.f * _R * _R;

    double z = sum_d_sqrd * sum_d_sqrd; // x^4
    double a = 4. * sum_d_sqrd * f; // x^3
    double b = 2. * sum_d_sqrd * e + 4. * f * f + four_a_sqrd * ray_dir[1] * ray_dir[1]; // x^2
    double c = 4. * f * e + 2. * four_a_sqrd * ray_dir[1] * ray_org[1];
    double d = e * e - four_a_sqrd * (_r * _r - ray_org[1] * ray_org[1]);

    a /= z;
    b /= z;
    c /= z;
    d /= z;

    std::complex<double>* solutions = solve_quartic(a, b, c, d);

    // find closest solution
    std::complex<double> solution;
    float t_min = h.getT();
    float t_guess;
    float imag_eps = 0.;
    for (int i=0; i < 4; i++) {
         solution = solutions[i];

         if (abs(solution.imag()) > imag_eps) {
             continue;
         }

         t_guess = (float)solution.real();

         if ((t_guess < t_min) && (t_guess > tmin)) {
             t_min = t_guess;
         }
    }

    // check that it is the closest hit so far
    if (t_min < h.getT()) {
        Vector3f point = r.pointAtParameter(t_min);

        // sanity check
//        if (point.abs() > _R + _r) {
//            return false;
//        }

        float alpha = _R / point.xz().abs();
        Vector3f normal((1-alpha) * point[0], point[1], (1 - alpha) * point[2]);
        normal.normalize();

        h.set(t_min, this->material, normal);
        return true;
    }

    return false;
}


bool Transform::intersect(const Ray &r, float tmin, Hit &h) const
{
    Matrix4f m_inverse = _m.inverse();
    Ray new_r(VecUtils::transformPoint(m_inverse, r.getOrigin()),
            VecUtils::transformDirection(m_inverse, r.getDirection()));
    bool hit = _object->intersect(new_r, tmin, h);

    if (hit) {
        h.normal = VecUtils::transformDirection(m_inverse.transposed(), h.normal).normalized();
    }

    return hit;
}