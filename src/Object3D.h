#ifndef OBJECT3D_H
#define OBJECT3D_H

#include "Ray.h"
#include "Material.h"

#include <string>

class Object3D {
public:
    Object3D() {
        material = NULL;
    }

    virtual ~Object3D() {}

    Object3D(Material *material) {
        this->material = material;
    }

    std::string getType() const {
        return type;
    }

    Material *getMaterial() const {
        return material;
    }

    virtual bool intersect(const Ray &r, float tmin, Hit &h) const = 0;

    virtual const Ray sample() {
        return Ray(Vector3f(0), Vector3f(0));
    }

    std::string type;
    Material *material;
};


class Sphere : public Object3D {
public:
    // default contstructor: unit ball at origin
    Sphere() {
        _center = Vector3f(0.0, 0.0, 0.0);
        _radius = 1.0f;
    }

    Sphere(const Vector3f &center,
           float radius,
           Material *material) :
            Object3D(material),
            _center(center),
            _radius(radius) {
    }

    virtual ~Sphere() {}

    virtual bool intersect(const Ray &r, float tmin, Hit &h) const override;

    virtual const Ray sample() override;

private:
    Vector3f _center;
    float _radius;
};

class Group : public Object3D {
public:
    virtual ~Group() {}

    // Return true if intersection found
    virtual bool intersect(const Ray &r, float tmin, Hit &h) const override;

    // Add object to group
    void addObject(Object3D *obj);

    // Return number of objects in group
    int getGroupSize() const;

private:
    std::vector<Object3D *> m_members;
};

class Plane : public Object3D {
public:
    Plane(const Vector3f &normal,
          float d,
          Material *m) :
            Object3D(m),
            _normal(normal),
            _d(d) {
    }

    virtual ~Plane() {}

    virtual bool intersect(const Ray &r, float tmin, Hit &h) const override;

private:
    Vector3f _normal;
    float _d;
};


class Triangle : public Object3D {
public:
    Triangle(const Vector3f &a,
             const Vector3f &b,
             const Vector3f &c,
             const Vector3f &na,
             const Vector3f &nb,
             const Vector3f &nc,
             Material *m) :
            Object3D(m) {
        _v[0] = a;
        _v[1] = b;
        _v[2] = c;
        _normals[0] = na;
        _normals[1] = nb;
        _normals[2] = nc;
    }

    virtual ~Triangle() {}

    virtual bool intersect(const Ray &ray, float tmin, Hit &hit) const override;

    const Vector3f &getVertex(int index) const {
        assert(index < 3);
        return _v[index];
    }

    const Vector3f &getNormal(int index) const {
        assert(index < 3);
        return _normals[index];
    }

private:
    Vector3f _v[3];
    Vector3f _normals[3];
};

class Torus : public Object3D {
public:
    Torus(float R,
          float r,
          Material *m) :
            _R(R),
            _r(r),
            Object3D(m) {
    }

    virtual ~Torus() {}

    virtual bool intersect(const Ray &ray, float tmin, Hit &hit) const override;

private:
    float _R;
    float _r;
};


// So that the intersect function first transforms the ray
class Transform : public Object3D {
public:
    Transform(const Matrix4f &m, Object3D *obj) : _object(obj), _m(m), _m_inverse(m.inverse()) {}

    virtual ~Transform() {}

    bool intersect(const Ray &r, float tmin, Hit &h) const override;

private:
    Object3D *_object;  // un-transformed object
    Matrix4f _m;
    Matrix4f _m_inverse;
};


#endif
