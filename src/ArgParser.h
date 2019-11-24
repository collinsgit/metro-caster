#ifndef ARG_PARSER_H
#define ARG_PARSER_H

#include <string>

class ArgParser {
public:
    ArgParser(int argc, const char *argv[]);

    // ==============
    // REPRESENTATION
    // All public! (no accessors).

    // rendering output
    std::string input_file;
    std::string output_file;
    int width;
    int height;
    int stats;

    // rendering options
    float depth_min;
    float depth_max;
    int iters;
    float length;
    int bounces;
    bool shadows;
    bool refraction;

    // supersampling
    bool jitter;
    bool filter;

    // logging
    std::string log_file;

private:
    void defaultValues();
};

#endif // ARG_PARSER_H
