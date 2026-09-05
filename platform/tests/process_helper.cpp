#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

namespace {

int ParseInt(const char *text) {
    if (text == nullptr || *text == '\0') {
        return 0;
    }
    char *end = nullptr;
    const long value = std::strtol(text, &end, 10);
    if (end == text || *end != '\0') {
        return 0;
    }
    if (value < 0) {
        return 0;
    }
    if (value > 100000000) {
        return 100000000;
    }
    return static_cast<int>(value);
}

int WriteBytes(FILE *stream, int count) {
    for (int i = 0; i < count; ++i) {
        if (fputc('x', stream) == EOF) {
            return 1;
        }
    }
    return fflush(stream) == 0 ? 0 : 1;
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 2) {
        return 2;
    }

    const std::string mode = argv[1];
    if (mode == "print-ids") {
        std::printf("%d %d\n", static_cast<int>(getpid()), static_cast<int>(getpgrp()));
        return 0;
    }
    if (mode == "exit") {
        return ParseInt(argc > 2 ? argv[2] : "0");
    }
    if (mode == "sleep-ms") {
        const int total_ms = ParseInt(argc > 2 ? argv[2] : "1000");
        int remaining = total_ms;
        while (remaining > 0) {
            const int chunk = remaining > 50 ? 50 : remaining;
            usleep(static_cast<useconds_t>(chunk) * 1000);
            remaining -= chunk;
        }
        return 0;
    }
    if (mode == "ignore-term") {
        std::signal(SIGTERM, SIG_IGN);
        sleep(30);
        return 0;
    }
    if (mode == "write-both") {
        const int count = ParseInt(argc > 2 ? argv[2] : "0");
        return WriteBytes(stdout, count) || WriteBytes(stderr, count);
    }
    if (mode == "flood") {
        for (;;) {
            if (WriteBytes(stdout, 4096) != 0) {
                return 1;
            }
        }
    }
    if (mode == "write-stdout") {
        return WriteBytes(stdout, ParseInt(argc > 2 ? argv[2] : "0"));
    }
    if (mode == "write-stderr") {
        return WriteBytes(stderr, ParseInt(argc > 2 ? argv[2] : "0"));
    }
    if (mode == "self-term") {
        std::raise(SIGTERM);
        return 0;
    }
    if (mode == "fork-sleep") {
        const pid_t child = fork();
        if (child < 0) {
            return 1;
        }
        if (child == 0) {
            std::signal(SIGTERM, SIG_IGN);
            sleep(30);
            _exit(0);
        }
        if (argc > 2) {
            FILE *file = std::fopen(argv[2], "w");
            if (file == nullptr) {
                return 1;
            }
            std::fprintf(file, "%d %d %d %d\n", static_cast<int>(getpid()), static_cast<int>(getpgrp()),
                         static_cast<int>(child), static_cast<int>(getpgid(child)));
            std::fclose(file);
        }
        sleep(30);
        return 0;
    }
    return 2;
}
