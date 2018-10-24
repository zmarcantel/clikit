#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "src/clikit.hpp"

static const char* PROG_NAME = "simple";
static const char* PROG_VERS = "v0.1.0";

struct Options {
    bool out_err = false;
    std::uint8_t verbosity = 0;
    std::size_t block_size = 4096;

    std::vector<const char*> inputs;

};

cli::Parser parse_args(int argc, const char** argv, Options& opts) {
    cli::Parser args(argc, argv);
    args.details(PROG_NAME, "example tool that prints files")
        .version(PROG_VERS)
        .count('v', "verbose", "increase verbosity level", opts.verbosity)
        .flag("err", "print to stderr rather than stdout", opts.out_err)
        .arg('b', "block-size", "block size to read/write with", opts.block_size, "BYTES")
        .all_positionals("files", "list of files to print out", opts.inputs);
    ;

    return args;
}

void print(const std::string& fname, decltype(STDOUT_FILENO) outfd, std::size_t block_size) {
    int fd = open(fname.c_str(), O_RDONLY);
    if (fd == -1) {
        auto err = errno; // save in case of stderr write(2) failures which can overwrite
        std::cerr<< "failed to open " << fname << ": " << strerror(err) << std::endl;
        close(fd);
        exit(1);
    }

    int read_size = 0;
    std::uint8_t buf[block_size];
    while ((read_size = read(fd, buf, block_size))) {
        if (read_size == -1) {
            auto err = errno;
            std::cerr << "failed to read from file: " << strerror(err) << std::endl;
            break;
        }
        int wrote = 0;
        while ((wrote = write(outfd, static_cast<const void*>(buf+wrote), read_size-wrote))) {
            if (wrote == -1) {
                // just abandon.... is it worth trying stderr?
                close(fd);
                exit(errno);
            }
        }
    }

    close(fd);
}


int main(int argc, const char** argv) {
    Options opts;
    try {
        auto args = parse_args(argc, argv, opts);
        if (args.wants_help()) {
            args.print();
            return 0;
        }
    } catch (const cli::ParseError& err) {
        std::cerr <<  err.what() << std::endl;
        return 1;
    } catch (const cli::InternalError& err) {
        std::cerr << "INTERNAL ERROR: " << err.what() << std::endl;
        return 1;
    }

    // do stuff with opts
    for (auto f : opts.inputs) {
        print(f, opts.out_err ? STDERR_FILENO : STDOUT_FILENO, opts.block_size);
    }

    return 0;
};
