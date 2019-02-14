#include <map>

#include "src/clikit.hpp"

static const char* PROG_NAME = "simple";
static const char* PROG_VERS = "v0.1.0";

static const char* PROG_DESC_SHORT = "example of a dynamic parser using args to make new args";
static const char* PROG_DESC_LONG = ""
"You can provide runtime-generated arguments using the command like:\n"
"\n"
"    -a \"v:verbose:provide a number for verbosity\"\n"
"    --arg \"f:foo:some silly argument\"\n"
"\n"
"These options will also show up in the --help printed.\n"
"";

class DynArg {
protected:
    char _short;
    std::string _long;
    std::string _desc;

public:
    DynArg(char s, std::string l, std::string desc)
        : _short(s)
        , _long(std::move(l))
        , _desc(std::move(desc))
    {}

    DynArg(const char* in)
        : _short(0)
    {
        auto len = strlen(in);
        if (len == 0) {
            throw std::runtime_error("received empty input string for dynarg");
        }

        // look for short
        std::size_t iter = 0;
        if (in[iter] != ':') {
            _short = in[iter];
            iter++;
        }
        if (len <= 2) {
            return; // short only
        }

        if (in[iter] != ':') {
            throw std::runtime_error("invalid dynarg format");
        } else {
            iter++; // skip short's colon
        }

        auto start = iter;
        for (; iter < len; iter++) {
            if (in[iter] == ':') {
                break;
            }
        }
        _long = std::string(in+start, iter-start);

        if (iter >= len) {
            return; // no desc
        }

        // required colon for desc
        if (in[iter] != ':') {
            throw std::runtime_error("invalid dynarg format");
        } else {
            iter++; // skip long's colon
        }

        _desc = std::string(in+iter, len-iter);
    }

    char short_arg() const { return _short; }
    const std::string& long_arg() const { return _long; }
    const std::string& description() const { return _desc; }
};

struct Options {
    std::uint8_t verbosity = 0;
    std::size_t trickery = 1234;
    std::vector<DynArg> arguments;
    std::map<std::string, const char*> dynamic_values;

};

cli::Parser parse_args(int argc, const char** argv, Options& opts) {
    cli::Parser args(argc, argv);
    args.details(PROG_NAME, PROG_DESC_SHORT, PROG_DESC_LONG)
        .version(PROG_VERS)
        .disable_help_shortcircuit()
        .count('v', "verbose", "increase verbosity level", opts.verbosity)
        .arg('t', "trickery", "prove there's no trickery needed for already-parsed values",
            opts.trickery, "NUM", cli::ArgReq::Required)
        .list('a', "arg", "add a dynamic argument (-a \"f:foo:description\")",
            opts.arguments, "FMT")
    ;

    return args;
}

void be_dynamic(cli::Parser& args, Options& opts) {
    std::uint64_t new_number = 347563755;
    opts.trickery = new_number;

    for (auto& a : opts.arguments) {
        auto key = cli::arg_string(a.short_arg(), a.long_arg().c_str());
        auto iter = opts.dynamic_values.find(key);
        if (iter == opts.dynamic_values.end()) {
            iter = opts.dynamic_values.emplace(key, nullptr).first;
        }
        args.arg(a.short_arg(), a.long_arg().c_str(), a.description().c_str(), iter->second);
    }

    // assert trickery number is still correct
    if (opts.trickery != new_number) {
        throw std::runtime_error("reset the trickery number");
    }

    for (auto& i : opts.dynamic_values) {
        std::cout << i.first << ": " << i.second << std::endl;
    }
}

int main(int argc, const char** argv) {
    Options opts;
    try {
        auto args = parse_args(argc, argv, opts);
        be_dynamic(args, opts);
        args.validate(); // assert we used all the arguments

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
    } catch (const cli::MissingArgumentError& err) {
        std::cerr << err.what() << std::endl;
        return 1;
    }

    return 0;
};
