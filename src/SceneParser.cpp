#include <cstdio>
#include <cstring>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "SceneParser.h"
#include "Camera.h"
#include "Material.h"

#include "Object3D.h"

#define DegreesToRadians(x) ((M_PI * x) / 180.0f)

static
void
_PostError(const std::string &msg) {
    std::cout << msg;
    exit(1);
}

SceneParser::SceneParser(const std::string &filename) :
        _file(NULL),
        _camera(NULL),
        _background_color(0.5, 0.5, 0.5),
        _ambient_light(0, 0, 0),
        _num_lights(0),
        _num_materials(0),
        _current_material(NULL),
        _group(NULL),
        _cubemap(NULL) {
    // Parse the file.
    assert(!filename.empty());

    if (filename.size() <= 4) {
        _PostError("ERROR: Wrong file name extension\n");
    }

    size_t last_sep = filename.find_last_of("\\/");
    if (last_sep == std::string::npos) {
        _basepath = "";
    } else {
        _basepath = filename.substr(0, last_sep + 1);
    }

    std::string ext = filename.substr(filename.size() - 4, 4);
    if (ext != ".txt") {
        _PostError("ERROR: Wrong file name extension\n");
    }

    _file = fopen(filename.c_str(), "r");

    // FIXME extract base path from scene file path
    if (_file == NULL) {
        _PostError(std::string("Cannot open scene file ") + filename + "\n");
    }

    parseFile();
    fclose(_file);
    _file = NULL;

    // If no lights are specified, set ambient light to white
    // (do solid color ray casting).
    if (lights.empty()) {
        std::cerr << "WARNING: No lights specified\n";
        _ambient_light = Vector3f(1, 1, 1);
    }

    sampler = new experimental;
}

SceneParser::~SceneParser() {
    delete _group;
    delete _camera;
    for (auto *material : _materials) {
        delete material;
    }
    for (auto *object : _objects) {
        delete object;
    }
    delete _cubemap;
}

// ====================================================================
// ====================================================================

void
SceneParser::parseFile() {
    // At the top level, the scene can have a camera,
    // background color and a group of objects.
    char token[MAX_PARSER_TOKEN_LENGTH];
    while (getToken(token)) {
        if (!strcmp(token, "PerspectiveCamera")) {
            parsePerspectiveCamera();
        } else if (!strcmp(token, "Background")) {
            parseBackground();
        } else if (!strcmp(token, "Materials")) {
            parseMaterials();
        } else if (!strcmp(token, "Group")) {
            _group = parseGroup();
        } else {
            _PostError(
                    std::string("Unknown token in parseFile: '") + token + "'\n");
            exit(1);
        }
    }
}

// ====================================================================
// ====================================================================

void
SceneParser::parsePerspectiveCamera() {
    // Read in the camera parameters.
    char token[MAX_PARSER_TOKEN_LENGTH];
    getToken(token);
    assert(!strcmp(token, "{"));
    getToken(token);
    assert(!strcmp(token, "center"));
    Vector3f center = readVector3f();
    getToken(token);
    assert(!strcmp(token, "direction"));
    Vector3f direction = readVector3f();
    getToken(token);
    assert(!strcmp(token, "up"));
    Vector3f up = readVector3f();
    getToken(token);
    assert(!strcmp(token, "angle"));
    float angle_degrees = readFloat();
    float angle_radians = (float) DegreesToRadians(angle_degrees);
    getToken(token);
    assert(!strcmp(token, "}"));
    _camera = new PerspectiveCamera(center, direction, up, angle_radians);
}

void
SceneParser::parseBackground() {
    // Read in the background color.
    char token[MAX_PARSER_TOKEN_LENGTH];
    getToken(token);
    assert(!strcmp(token, "{"));
    while (true) {
        getToken(token);
        if (!strcmp(token, "}")) {
            break;
        } else if (!strcmp(token, "color")) {
            _background_color = readVector3f();
        } else if (!strcmp(token, "ambientLight")) {
            _ambient_light = readVector3f();
        } else if (!strcmp(token, "cubeMap")) {
            _cubemap = parseCubeMap();
        } else {
            printf("Unknown token in parseBackground: '%s'\n", token);
            assert(0);
        }
    }
}

CubeMap *
SceneParser::parseCubeMap() {
    char token[MAX_PARSER_TOKEN_LENGTH];
    getToken(token);
    return new CubeMap(_basepath + token);
}

// ====================================================================
// ====================================================================

void
SceneParser::parseMaterials() {
    // Ensure the structure is valid.
    char token[MAX_PARSER_TOKEN_LENGTH];
    getToken(token);
    assert(!strcmp(token, "{"));

    // Read in the number of objects.
    getToken(token);
    assert(!strcmp(token, "numMaterials"));
    _num_materials = readInt();

    // Read in the objects.
    int count = 0;
    while (_num_materials > count) {
        getToken(token);
        if (!strcmp(token, "Material") ||
            !strcmp(token, "PhongMaterial")) {
            _materials.push_back(parseMaterial());
        } else {
            printf("Unknown token in parseMaterial: '%s'\n", token);
            exit(0);
        }
        count++;
    }
    getToken(token);
    assert(!strcmp(token, "}"));
}

Material *
SceneParser::parseMaterial() {
    char token[MAX_PARSER_TOKEN_LENGTH];
    char filename[MAX_PARSER_TOKEN_LENGTH];
    filename[0] = 0;
    Vector3f diffuseColor(1), specularColor(0), transColor(0), light(0);
    float shininess = 1;
    float refIndex = 1;
    getToken(token);
    assert(!strcmp(token, "{"));
    while (true) {
        getToken(token);
        if (strcmp(token, "diffuseColor") == 0) {
            diffuseColor = readVector3f();
        } else if (strcmp(token, "specularColor") == 0) {
            specularColor = readVector3f();
        } else if (strcmp(token, "shininess") == 0) {
            shininess = readFloat();
        } else if (strcmp(token, "transColor") == 0) {
            transColor = readVector3f();
        } else if (strcmp(token, "light") == 0) {
            light = readVector3f();
        } else if (strcmp(token, "refIndex") == 0) {
            refIndex = readFloat();
        } else if (strcmp(token, "bump") == 0) {
            getToken(token);
        } else {
            assert(!strcmp(token, "}"));
            break;
        }
    }
    Material *answer = new Material(diffuseColor, specularColor, transColor, light, shininess, refIndex);


    return answer;
}

// ====================================================================
// ====================================================================

Object3D *
SceneParser::parseObject(char token[MAX_PARSER_TOKEN_LENGTH]) {
    Object3D *answer = nullptr;
    if (!strcmp(token, "Group")) {
        answer = (Object3D *) parseGroup();
    } else if (!strcmp(token, "Sphere")) {
        answer = (Object3D *) parseSphere();
    } else if (!strcmp(token, "Torus")) {
        answer = (Object3D *) parseTorus();
    } else if (!strcmp(token, "Plane")) {
        answer = (Object3D *) parsePlane();
    } else if (!strcmp(token, "Area")) {
        answer = (Object3D *) parseArea();
    } else if (!strcmp(token, "Triangle")) {
        answer = (Object3D *) parseTriangle();
    } else if (!strcmp(token, "TriangleMesh")) {
        answer = (Object3D *) parseTriangleMesh();
    } else if (!strcmp(token, "Transform")) {
        answer = (Object3D *) parseTransform();
    } else {
        printf("Unknown token in parseObject: '%s'\n", token);
        exit(0);
    }
    return answer;
}

// ====================================================================
// ====================================================================

Group *
SceneParser::parseGroup() {
    // Each group starts with an integer that specifies
    // the number of objects in the group.
    //
    // The material index sets the material of all objects which follow,
    // until the next material index (scoping for the materials is very
    // simple, and essentially ignores any tree hierarchy).
    char token[MAX_PARSER_TOKEN_LENGTH];
    getToken(token);
    assert(!strcmp(token, "{"));

    // Read in the number of objects.
    getToken(token);
    assert(!strcmp(token, "numObjects"));
    int num_objects = readInt();

    Group *answer = new Group();

    // Read in the objects.
    int count = 0;
    bool in_light = false;
    while (num_objects > count) {
        getToken(token);
        if (!strcmp(token, "MaterialIndex")) {
            // Change the current material.
            int index = readInt();
            assert(index >= 0 && index <= getNumMaterials());
            _current_material = getMaterial(index);
            in_light = _current_material->getLight() != Vector3f::ZERO;
        } else {
            Object3D *object = parseObject(token);
            assert(object != NULL);
            answer->addObject(object);
            count++;
            _objects.push_back(object);
            if (in_light) {
                lights.push_back(object);
            }
        }
    }
    getToken(token);
    assert(!strcmp(token, "}"));

    // Return the group.
    return answer;
}

// ====================================================================
// ====================================================================

Sphere *
SceneParser::parseSphere() {
    char token[MAX_PARSER_TOKEN_LENGTH];
    getToken(token);
    assert(!strcmp(token, "{"));
    getToken(token);
    assert(!strcmp(token, "center"));
    Vector3f center = readVector3f();
    getToken(token);
    assert(!strcmp(token, "radius"));
    float radius = readFloat();
    getToken(token);
    assert(!strcmp(token, "}"));
    assert(_current_material != NULL);
    return new Sphere(center, radius, _current_material);
}

Torus *
SceneParser::parseTorus() {
    char token[MAX_PARSER_TOKEN_LENGTH];
    getToken(token);
    assert(!strcmp(token, "{"));
    getToken(token);
    assert(!strcmp(token, "R"));
    float R = readFloat();
    getToken(token);
    assert(!strcmp(token, "r"));
    float r = readFloat();
    getToken(token);
    assert(!strcmp(token, "}"));
    assert(_current_material != NULL);
    return new Torus(R, r, _current_material);
}

Plane *
SceneParser::parsePlane() {
    char token[MAX_PARSER_TOKEN_LENGTH];
    getToken(token);
    assert(!strcmp(token, "{"));
    getToken(token);
    assert(!strcmp(token, "normal"));
    Vector3f normal = readVector3f();
    getToken(token);
    assert(!strcmp(token, "offset"));
    float offset = readFloat();
    getToken(token);
    assert(!strcmp(token, "}"));
    assert(_current_material != NULL);
    return new Plane(normal, offset, _current_material);
}

Area *
SceneParser::parseArea() {
    char token[MAX_PARSER_TOKEN_LENGTH];
    getToken(token);
    assert(!strcmp(token, "{"));
    getToken(token);
    assert(!strcmp(token, "corner"));
    Vector3f corner = readVector3f();
    getToken(token);
    assert(!strcmp(token, "sideOne"));
    Vector3f sideOne = readVector3f();
    getToken(token);
    assert(!strcmp(token, "sideTwo"));
    Vector3f sideTwo = readVector3f();
    getToken(token);
    assert(!strcmp(token, "}"));
    assert(_current_material != NULL);
    return new Area(corner, sideOne, sideTwo, _current_material);
}

Triangle *
SceneParser::parseTriangle() {
    char token[MAX_PARSER_TOKEN_LENGTH];
    getToken(token);
    assert(!strcmp(token, "{"));
    getToken(token);
    assert(!strcmp(token, "vertex0"));
    Vector3f v0 = readVector3f();
    getToken(token);
    assert(!strcmp(token, "vertex1"));
    Vector3f v1 = readVector3f();
    getToken(token);
    assert(!strcmp(token, "vertex2"));
    Vector3f v2 = readVector3f();
    getToken(token);
    assert(!strcmp(token, "}"));
    assert(_current_material != NULL);
    Vector3f a = v1 - v0;
    Vector3f b = v2 - v0;
    Vector3f n = Vector3f::cross(a, b).normalized();
    return new Triangle(v0, v1, v2, n, n, n, _current_material);
}

Mesh *
SceneParser::parseTriangleMesh() {
    char token[MAX_PARSER_TOKEN_LENGTH];
    char filename[MAX_PARSER_TOKEN_LENGTH];
    // get the filename
    getToken(token);
    assert(!strcmp(token, "{"));
    getToken(token);
    assert(!strcmp(token, "obj_file"));
    getToken(filename);
    getToken(token);
    assert(!strcmp(token, "}"));
    const char *ext = &filename[strlen(filename) - 4];
    assert(!strcmp(ext, ".obj"));
    Mesh *answer = new Mesh(_basepath + filename, _current_material);

    return answer;
}

Transform *
SceneParser::parseTransform() {
    char token[MAX_PARSER_TOKEN_LENGTH];
    Matrix4f matrix = Matrix4f::identity();
    Object3D *object = NULL;
    getToken(token);
    assert(!strcmp(token, "{"));

    // Read in transformations:
    // apply to the LEFT side of the current matrix (so the first
    // transform in the list is the last applied to the object).
    getToken(token);
    while (true) {
        if (!strcmp(token, "Scale")) {
            Vector3f s = readVector3f();
            matrix = matrix * Matrix4f::scaling(s[0], s[1], s[2]);
        } else if (!strcmp(token, "UniformScale")) {
            float s = readFloat();
            matrix = matrix * Matrix4f::uniformScaling(s);
        } else if (!strcmp(token, "Translate")) {
            matrix = matrix * Matrix4f::translation(readVector3f());
        } else if (!strcmp(token, "XRotate")) {
            matrix = matrix * Matrix4f::rotateX((float) DegreesToRadians(readFloat()));
        } else if (!strcmp(token, "YRotate")) {
            matrix = matrix * Matrix4f::rotateY((float) DegreesToRadians(readFloat()));
        } else if (!strcmp(token, "ZRotate")) {
            matrix = matrix * Matrix4f::rotateZ((float) DegreesToRadians(readFloat()));
        } else if (!strcmp(token, "Rotate")) {
            getToken(token);
            assert(!strcmp(token, "{"));
            Vector3f axis = readVector3f();
            float degrees = readFloat();
            float radians = (float) DegreesToRadians(degrees);
            matrix = matrix * Matrix4f::rotation(axis, radians);
            getToken(token);
            assert(!strcmp(token, "}"));
        } else if (!strcmp(token, "Matrix4f")) {
            Matrix4f matrix2 = Matrix4f::identity();
            getToken(token);
            assert(!strcmp(token, "{"));
            for (int j = 0; j < 4; j++) {
                for (int i = 0; i < 4; i++) {
                    float v = readFloat();
                    matrix2(i, j) = v;
                }
            }
            getToken(token);
            assert(!strcmp(token, "}"));
            matrix = matrix2 * matrix;
        } else {
            // Otherwise this must be an object,
            // and there are no more transformations.
            object = parseObject(token);
            break;
        }
        getToken(token);
    }

    assert(object != NULL);
    getToken(token);
    assert(!strcmp(token, "}"));
    return new Transform(matrix, object);
}

// ====================================================================
// ====================================================================

int
SceneParser::getToken(char token[MAX_PARSER_TOKEN_LENGTH]) {
    // For simplicity, tokens must be separated by whitespace.
    assert(_file != NULL);
    int success = fscanf(_file, "%s ", token);
    if (success == EOF) {
        token[0] = '\0';
        return 0;
    }
    return 1;
}

Vector3f
SceneParser::readVector3f() {
    float x, y, z;
    int count = fscanf(_file, "%f %f %f", &x, &y, &z);
    if (count != 3) {
        printf("Error trying to read 3 floats to make a Vector3f\n");
        assert(0);
    }
    return Vector3f(x, y, z);
}


Vector2f
SceneParser::readVec2f() {
    float u, v;
    int count = fscanf(_file, "%f %f", &u, &v);
    if (count != 2) {
        printf("Error trying to read 2 floats to make a Vec2f\n");
        assert(0);
    }
    return Vector2f(u, v);
}

float
SceneParser::readFloat() {
    float answer;
    int count = fscanf(_file, "%f", &answer);
    if (count != 1) {
        printf("Error trying to read 1 float\n");
        assert(0);
    }
    return answer;
}


int
SceneParser::readInt() {
    int answer;
    int count = fscanf(_file, "%d", &answer);
    if (count != 1) {
        printf("Error trying to read 1 int\n");
        assert(0);
    }
    return answer;
}
