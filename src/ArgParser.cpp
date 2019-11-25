#include "ArgParser.h"

#include <cstring>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>

ArgParser::ArgParser(int argc, const char *argv[]) {
    defaultValues();

    for (int i = 1; i < argc; i++) {
        // rendering output
        if (!strcmp(argv[i], "-input")) {
            i++;
            assert (i < argc);
            input_file = argv[i];
        } else if (!strcmp(argv[i], "-output")) {
            i++;
            assert (i < argc);
            output_file = argv[i];
        } else if (!strcmp(argv[i], "-size")) {
            i++;
            assert (i < argc);
            width = atoi(argv[i]);
            i++;
            assert (i < argc);
            height = atoi(argv[i]);
        }

        // rendering options
        else if (!strcmp(argv[i], "-iters")) {
            i++;
            assert (i < argc);
            iters = atoi(argv[i]);
        } else if (!strcmp(argv[i], "-length")) {
            i++;
            assert (i < argc);
            length = atof(argv[i]);
        }

        // logging
        else if (!strcmp(argv[i], "-log")) {
            i++;
            assert (i < argc);
            log_file = argv[i];
        }

        // Unknown argument.
        else {
            printf("Unknown command line argument %d: '%s'\n", i, argv[i]);
            exit(1);
        }
    }

    std::cout << "Args:\n";
    std::cout << "- input: " << input_file << std::endl;
    std::cout << "- output: " << output_file << std::endl;
    std::cout << "- width: " << width << std::endl;
    std::cout << "- height: " << height << std::endl;
    std::cout << "- iters: " << iters << std::endl;
    std::cout << "- length: " << length << std::endl;
    std::cout << "- log: " << log_file << std::endl;
}

void
ArgParser::defaultValues() {
    // rendering output
    input_file = "";
    output_file = "";
    width = 100;
    height = 100;

    // rendering options
    iters = 10;
    length = 1.f;

    // logging
    log_file = "";
}
