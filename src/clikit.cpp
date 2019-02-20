#include "src/clikit.hpp"

namespace cli {

//-------------------------------------------------------------------------
// bitset
//-------------------------------------------------------------------------

std::size_t BitSet::num_elements() const {
    return (N / BITS_PER_SIZET) + ((N % BITS_PER_SIZET) ? 1 : 0);
}
std::size_t BitSet::arr_index(std::size_t linear) const {
    return linear / BITS_PER_SIZET;
}
std::size_t BitSet::bit_index(std::size_t linear) const {
    return linear % BITS_PER_SIZET;
}

std::size_t BitSet::set(std::size_t linear) {
    auto arr = arr_index(linear);
    auto bit = bit_index(linear);

    if (linear >= N) {
        #ifndef __EXCEPTIONS
        return -1;
        #else
        std::stringstream ss;
        ss << "linear index " << linear << " is out of the bitset bounds " << N;
        throw std::runtime_error(ss.str());
        #endif
    }
    data[arr] |= (std::size_t)(1) << bit;

    return linear;
}
bool BitSet::is_set(std::size_t linear) {
    if (linear >= N) {
        #ifndef __EXCEPTIONS
        return -1;
        #else
        std::stringstream ss;
        ss << "linear index " << linear << " is out of the bitset bounds " << N;
        throw std::runtime_error(ss.str());
        #endif
    }
    return data[arr_index(linear)] & ((std::size_t)(1) << bit_index(linear));
}
void BitSet::unset(std::size_t linear) {
    if (not is_set(linear)) {
        return;
    }
    data[arr_index(linear)] ^= (std::size_t)(1) << bit_index(linear);
}

std::size_t BitSet::total() const { return N; }
std::size_t BitSet::remaining() const { return total() - size(); }
std::size_t BitSet::size() const {
    std::size_t count = 0;
    for (std::size_t i = 0; i < num_elements(); ++i) {
        count += __builtin_popcountll(data[i]);
    }
    return count;
}

//
// set_iterator
//

void BitSet::set_iterator::find_next_bit() {
    if (set->total() == 0) {
        return;
    }
    // increment to actually find next
    cursor++;

    while (cursor < set->total()) {
        std::size_t idx = set->arr_index(cursor);
        std::size_t off = set->bit_index(cursor);

        auto delta = __builtin_ffsll( set->data[idx] >> off );
        if (delta) {
            cursor += delta - 1; // -1 to account for initially added incr
            return;
        } else {
            cursor += std::min( // round up to next element or end of list
                (set->total() - cursor),
                (BITS_PER_SIZET - off)
            );
            continue;
        }
    }
}

//
// unset_iterator
//

void BitSet::unset_iterator::find_next_zero() {
    if (set->total() == 0) {
        return;
    }

    cursor++; // finding next, so move
    while (cursor < set->total()) {
        // get the index+offset to address into
        std::size_t index = set->arr_index(cursor);
        std::size_t offset = set->bit_index(cursor);
        // remove all the bits we've already considered, then logical not
        // since the 0s are now 1s we can just ffs like the other iter
        std::size_t adjusted = ~(set->data[index] >> offset);
        std::size_t delta = __builtin_ffsll(adjusted);

        // if we see 63 bits, we've shifted the whole ptr[index]
        // and need to move along
        if (delta and ((offset + delta - 1) < BITS_PER_SIZET)) { //(delta < (BITS_PER_SIZET-1))) {
            cursor += delta - 1; // -1 to adjust for the cursor adjustment on entry
            return;
        }

        // round up to next element or end of list
        cursor += std::min(
            (set->total() - cursor),
            (BITS_PER_SIZET - offset)
        );
    }
}




//-------------------------------------------------------------------------
// generic helper functions
//-------------------------------------------------------------------------

bool is_valid_short(char c) {
    return ((c >= '0' and c <= '9'))
        or ((c >= 'a') and (c <= 'z'))
        or ((c >= 'A') and (c <= 'Z'));
}

void arg_string(std::ostream& ss, char s, const char* l, bool pad) {
    bool valid_short = is_valid_short(s);

    if (valid_short) {
        ss << "-" << s;
    } else if (pad) {
        ss << "  ";
    }

    if (l != nullptr) {
        if (valid_short) { ss << "/"; } else if (pad) { ss << " "; }
        ss << "--" << l;
    }
}
std::string arg_string(char s, const char* l, bool pad) {
    std::stringstream ss;
    arg_string(ss, s, l, pad);
    return ss.str();
}



//-------------------------------------------------------------------------
// arg -> ctor delegation
//-------------------------------------------------------------------------

// unsigned
template<> std::uint8_t From<std::uint8_t>(const char* s) { return   std::stoul(s); }
template<> std::uint16_t From<std::uint16_t>(const char* s) { return std::stoul(s); }
template<> std::uint32_t From<std::uint32_t>(const char* s) { return std::stoul(s); }
template<> std::uint64_t From<std::uint64_t>(const char* s) { return std::stoull(s); }


// signed
template<> std::int8_t From<std::int8_t>(const char* s) { return   std::stol(s); }
template<> std::int16_t From<std::int16_t>(const char* s) { return std::stol(s); }
template<> std::int32_t From<std::int32_t>(const char* s) { return std::stol(s); }
template<> std::int64_t From<std::int64_t>(const char* s) { return std::stoll(s); }


// floats
template<> float From<float>(const char* s) { return std::stof(s); }
template<> double From<double>(const char* s) { return std::stod(s); }
template<> long double From<long double>(const char* s) { return std::stold(s); }




//-------------------------------------------------------------------------
// help / printing descriptors
//-------------------------------------------------------------------------


// returns whether there are an args registered (including in groups)
// but subcommands do not count as they are not args
bool HelpMap::has_args() const {
    if (not _args.empty()) { return true; }
    if (not _pos.empty()) { return true; }

    for (auto& g : _groups) {
        if (not g.second.empty()) { return true; }
    }

    return false;
}

void HelpMap::print_usage_args(std::ostream& ss) const {
    std::vector<const ArgHelp*> required;
    std::vector<const ArgHelp*> optional;

    // sort into required and not
    for (auto& g : _groups) {
        for (auto& a : g.second) {
            if (a.required()) {
                required.emplace_back(&a);
            } else {
                optional.emplace_back(&a);
            }
        }
    }
    for (auto& a : _args) {
        if (a.required()) {
            required.emplace_back(&a);
        } else {
            optional.emplace_back(&a);
        }
    }

    // generate strings for each class
    // TODO: this is gross... but the code to do it ad-hoc is even grosser imho
    auto short_requireds = combine_all_shorts(required);
    auto long_requireds = combine_all_nonshorts(required);

    auto short_optionals = combine_all_shorts(optional);
    auto long_optionals = combine_all_nonshorts(optional);

    // print things to the stream

    if (not short_requireds.empty()) {
        ss << "-" << short_requireds;
    }
    if (not long_requireds.empty()) {
        if (not short_requireds.empty()) {
            ss << " ";
        }
        ss << long_requireds;
    }

    if (not optional.empty()) {
        // do we need to separate from required's output
        if (not required.empty()) {
            ss << " ";
        }
        ss << "[";
    }

    if (not short_optionals.empty()) {
        ss << "-" << short_optionals;
    }
    if (not long_optionals.empty()) {
        if (not short_optionals.empty()) {
            ss << " ";
        }
        ss << long_optionals;
    }

    // close the bracket
    if (not optional.empty()) {
        ss << "]";
    }

    // print positionals
    if (not required.empty() or not optional.empty()) {
        ss << " ";
    }
    std::size_t pos_idx = 0;
    for (auto& p : _pos) {
        if (pos_idx) { ss << " "; }
        ss << p.name;
        if (p.variadic()) { ss << "..."; }
        pos_idx++;
    }
}

void HelpMap::details(const char* name, const char* desc, const char* long_desc) {
    _desc = Description(name, desc, long_desc);
}
void HelpMap::subcommand_details(const char* name, const char* desc, const char* long_desc) {
    _subcommand_desc = Description(name, desc, long_desc);
    _subcommands += " ";
    _subcommands += name;
}

void HelpMap::clear_subcommands() {
    _subs.clear();
}
void HelpMap::add_subcommand(const char* name, const char* desc) {
    _subs.emplace_back(name, desc, "");
    _longest_flag = std::max(_longest_flag, _subs.back().name_len + _indent_width);
}

void HelpMap::new_group(const char* name, const char* desc) {
    std::vector<ArgHelp> vec;
    _groups.emplace_back(Description(name, desc, ""), vec);
}

void HelpMap::print(std::ostream& s) const {
    auto right_col_start =  _indent_width + _longest_flag + _indent_width;

    bool in_subcommand = not _subcommands.empty();

    // app leading line and usage
    if (_desc.name_len) {
        // leading line
        s << _desc.name;

        if (in_subcommand) {
            s << _subcommands; // string includes a leading space
        } else if (_app_version) {
            s << " " << _app_version; // only print version when !subcommand
        }

        if (_subcommand_desc.short_len) {
            s << " - " << _subcommand_desc.short_desc;
        } else if (_desc.short_len) {
            s << " - " << _desc.short_desc;
        }
        s << std::endl << std::endl;
    }


    // usage
    if (has_args()) {
        s << "usage: " << _desc.name;

        if (in_subcommand) {
            s << _subcommands;
        }

        if (not _subs.empty()) {
            s << " [cmd...]";
        }

        s << " ";

        print_usage_args(s);
        s << std::endl << std::endl;
    }

    // long description
    if (_desc.long_len) {
        if (_desc.long_len) {
            s << _desc.long_desc << std::endl;
        }

        s << std::endl;
    }


    // subcommands
    if (_subs.size()) {
        s << "subcommands:" << std::endl;
        for (auto& sub : _subs) {
            indent_stream(s, _indent_width);
            s << sub.name;
            indent_stream(s, right_col_start - _indent_width - sub.name_len);
            s << sub.short_desc << std::endl;
        }
        s << std::endl;
    }

    // groups
    if (_groups.size()) {
        for (auto& g : _groups) {
            s << g.first.name << ": ";
            indent_stream(s, right_col_start - g.first.name_len - 2);
            s << g.first.short_desc << std::endl;
            for (auto& a : g.second) {
                indent_stream(s, _indent_width);
                s << a.flags_string() << " " << a.arg_name;
                indent_stream(s, right_col_start - _indent_width - a.left_col_width());
                s << a.desc << std::endl;
            }
            s << std::endl;
        }
    }

    // args
    if (_args.size()) {
        s << "options:" << std::endl;
        for (auto& a : _args) {
            indent_stream(s, _indent_width);
            s << a.flags_string() << " " << a.arg_name;
            indent_stream(s, right_col_start - _indent_width -  a.left_col_width());
            s << a.desc << std::endl;
        }
        s << std::endl;
    }

    // positionals
    if (_pos.size()) {
        s << "positionals:" << std::endl;
        for (auto& p : _pos) {
            indent_stream(s, _indent_width);
            s << p.name;
            if (p.variadic()) {
                s << "...";
            }
            indent_stream(s, right_col_start - _indent_width - p.left_col_width());
            s << p.desc << std::endl;
        }
        s << std::endl;
    }

    // pretty spacing
    s << std::endl;
}



//-------------------------------------------------------------------------
// parsing helpers
//-------------------------------------------------------------------------


bool ParseDesc::is_positional() const {
    return not (is_short or is_long);
}

std::size_t ParseDesc::matches(const char* arg, char s) const {
    if (not is_short) { return false; }

    std::size_t result = 0;
    for (std::size_t i = 1; i < len; i++) {
        if (arg[i] == s) {
            result += 1;
        }
    }

    return result;
}

bool ParseDesc::matches(const char* arg, const char* l) const {
    if (not is_long or l == nullptr) { return false; }

    auto cmplen = eq_offset>0 ? eq_offset : len;
    return strncmp(arg+2, l, cmplen-2) == 0;
}



//
// parser
//


Parser& Parser::done() {
    // handle groups first -- does not add a level though so skip that
    if (_in_group) {
        _in_group = false;
        return *this;
    }

    if (_level > 0) {
        _level--;
        _ctx.end_level();
    }
    return *this;
}

bool Parser::wants_help() const {
    return _ctx.wants_help();
}

void Parser::print() const {
    if (not _ctx.wants_help()) {
        return;
    }

    _help->print(std::cout);
}

// finalizer that asserts no unused arguments
void Parser::validate() {
    // if we are just printing help, don't validate
    if (_ctx.wants_help()) {
        return;
    }

    if (_ctx.remaining()) {
        auto arg = *_ctx.begin();
        std::stringstream ss;
        ss << "unknown argument '" << arg.c_str << "'";
        throw ParseError(ss.str());
    }
}

// finalizer that returns all unused args
std::vector<const char*> Parser::gather_remaining() {
    std::vector<const char*> unused;
    for (auto& a : _ctx) {
        unused.emplace_back(a.c_str);
    }

    return unused;
}



} // ns cli
