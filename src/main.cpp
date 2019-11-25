#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>

#include "ArgParser.h"
#include "Renderer.h"



// Formats a Chrono milliseconds duration to h:m:s notation.
std::string format_duration( std::chrono::milliseconds ms ) {
    // Fetch the seconds, minutes, and hours.
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(ms);
    ms -= std::chrono::duration_cast<std::chrono::milliseconds>(secs);
    auto mins = std::chrono::duration_cast<std::chrono::minutes>(secs);
    secs -= std::chrono::duration_cast<std::chrono::seconds>(mins);
    auto hour = std::chrono::duration_cast<std::chrono::hours>(mins);
    mins -= std::chrono::duration_cast<std::chrono::minutes>(hour);

    // Write to a string and return it.
    std::stringstream ss;
    ss << hour.count() << "h " << mins.count() << "m " << secs.count() << "s " << ms.count() << "ms";
    return ss.str();
}

// Logs program output to a file for ease of access at a later time.
void logProgramOutput(const ArgParser &argParser,
                      std::chrono::high_resolution_clock::time_point start,
                      std::chrono::high_resolution_clock::time_point stop,
                      const std::string& durationString) {
    // Sanity check that the log file is specified.
    assert(!argParser.log_file.empty());

    // Create a logging object.
    std::ofstream logging;
    logging.open(argParser.log_file, std::ios_base::app);

    // Create time_t for the start and end times to get the current date and time.
    time_t startTime_t = std::chrono::high_resolution_clock::to_time_t(start);
    tm* startTime = localtime(&startTime_t);
    char startTimeBuffer[80];
    strftime(startTimeBuffer, 80, "%c", startTime);

    time_t stopTime_t = std::chrono::high_resolution_clock::to_time_t(stop);
    tm* stopTime = localtime(&stopTime_t);
    char stopTimeBuffer[80];
    strftime(stopTimeBuffer, 80, "%c", stopTime);

    // Write to the log.
    logging << "[START TIME: " << startTimeBuffer << "]\n";
    logging << "- input: " << argParser.input_file << std::endl;
    logging << "- output: " << argParser.output_file << std::endl;
    logging << "- width: " << argParser.width << std::endl;
    logging << "- height: " << argParser.height << std::endl;
    logging << "- depth_min: " << argParser.depth_min << std::endl;
    logging << "- depth_max: " << argParser.depth_max << std::endl;
    logging << "- iters: " << argParser.iters << std::endl;
    logging << "- length: " << argParser.length << std::endl;
    logging << "- bounces: " << argParser.bounces << std::endl;
    logging << "- shadows: " << argParser.shadows << std::endl;
    logging << "- refractions: " << argParser.refraction << std::endl;
    logging << "- log: " << argParser.log_file << std::endl;
    logging << "[END TIME: " << stopTimeBuffer << "]\n";
    logging << "Total Duration: " << durationString << "\n\n";
    logging.close();
}

int
main(int argc, const char *argv[]) {
    // Report help usage if no args specified.
    if (argc == 1) {
        std::cout << "Usage: a5 <args>\n"
                  << "\n"
                  << "Args:\n"
                  << "\t-input <scene>\n"
                  << "\t-size <width> <height>\n"
                  << "\t-output <image.png>\n"
                  << "\t[-depth <depth_min> <depth_max>]\n"
                  << "\t[-iters <iterations>]\n"
                  << "\t[-length <path_lengths>]\n"
                  << "\t[-bounces <max_bounces>\n]"
                  << "\t[-shadows]\n"
                  << "\t[-log <log.txt>]\n"
                  << "\n";
        return 1;
    }

    // Record the start and end time of the program to be saved later.
    auto start = std::chrono::high_resolution_clock::now();
    ArgParser argsParser(argc, argv);
    Renderer renderer(argsParser);
    renderer.Render();
    auto stop = std::chrono::high_resolution_clock::now();

    // Get the overall duration to print out.
    long duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    std::chrono::milliseconds chronoDuration = (std::chrono::milliseconds) duration;
    std::string durationString = format_duration(chronoDuration);

    // Save to the log file if specified.
    if (!argsParser.log_file.empty()) {
        logProgramOutput(argsParser, start, stop, durationString);
    }

    // Print the program duration and return.
    std::cout << "Total Duration: " << durationString << std::endl;
    return 0;
}
