#ifndef __CLIKIT_HPP__
#define __CLIKIT_HPP__

#include <cstdint>
#include <iostream>
#include <string>
#include <cstring>
#include <memory>
#include <type_traits>
#include <vector>

#ifdef __EXCEPTIONS
#include <sstream>
#include <exception>
#endif

namespace cli {


//-------------------------------------------------------------------------
// bitset
//-------------------------------------------------------------------------

class BitSet {
public:
    static const std::size_t BITS_PER_SIZET = sizeof(std::size_t) * 8;

protected:
    // TODO: if only this could be a tempalte variable, but argc obviously runtime
    std::size_t N = 0;
    std::vector<std::size_t> data;

protected:
    std::size_t num_elements() const {
        return (N / BITS_PER_SIZET) + ((N % BITS_PER_SIZET) ? 1 : 0);
    }
    std::size_t arr_index(std::size_t linear) const {
        return linear / BITS_PER_SIZET;
    }
    std::size_t bit_index(std::size_t linear) const {
        return linear % BITS_PER_SIZET;
    }

public:
    BitSet() = delete;
    BitSet(std::size_t n)
        : N(n)
        , data(num_elements())
    {}

    std::size_t set(std::size_t linear) {
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
    bool is_set(std::size_t linear) {
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
    void unset(std::size_t linear) {
        if (not is_set(linear)) {
            return;
        }
        data[arr_index(linear)] ^= (std::size_t)(1) << bit_index(linear);
    }

    std::size_t total() const { return N; }
    std::size_t remaining() const { return total() - size(); }
    std::size_t size() const {
        std::size_t count = 0;
        for (std::size_t i = 0; i < num_elements(); ++i) {
            count += __builtin_popcountll(data[i]);
        }
        return count;
    }

public:
    class set_iterator {
    public:
        using self_type = set_iterator;
        using value_type = std::size_t;
        using reference = const value_type&;
        using pointer = const value_type*;
        using set_iterator_category = std::forward_iterator_tag;
        using difference_type = std::size_t;

    protected:
        const BitSet* set;
        std::size_t cursor;

        void find_next_bit() {
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


    public:
        set_iterator() = delete;
        set_iterator(const BitSet* set)
            : set(set)
            , cursor(0)
        {
            if (set->total() == 0) {
                return;
            }
            if (not (set->data[0] & 0x01)) { find_next_bit(); }
        }
        set_iterator(const BitSet* set, std::size_t index)
            : set(set)
            , cursor(index)
        {}
        self_type operator++() { // PREFIX
            // on entry, do not increment if at end
            if (cursor == set->total()) {
                return *this;
            }

            find_next_bit();
            return *this;
        }
        self_type operator++(int junk) { // POSTFIX
            self_type i = *this;
            ++(*this);
            return i;
        }
        value_type operator*() {
            return cursor;
        }
        value_type operator->() {
            return cursor;
        }
        bool operator==(const self_type& rhs) {
            return (set == rhs.set) and (cursor == rhs.cursor);
        }
        bool operator!=(const self_type& rhs) {
            return not (*this == rhs);
        }
    };

    class unset_iterator {
    public:
        using self_type = unset_iterator;
        using value_type = std::size_t;
        using reference = const value_type&;
        using pointer = const value_type*;
        using unset_iterator_category = std::forward_iterator_tag;
        using difference_type = std::size_t;

    protected:
        const BitSet* set;
        std::size_t cursor;

        inline void find_next_zero() {
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


    public:
        unset_iterator() = delete;
        unset_iterator(const BitSet* set)
            : set(set)
            , cursor(0)
        {
            if (set->total() == 0) {
                return;
            }
            // if LSB is set, find next zero, otherwise, we're at it
            if (set->data[0] & 0x01) {find_next_zero(); }
        }
        unset_iterator(const BitSet* set, std::size_t index)
            : set(set)
            , cursor(index)
        {}
        self_type operator++() { // PREFIX
            // on entry, do not increment if at end
            if (cursor == set->total()) {
                return *this;
            }

            find_next_zero();
            return *this;
        }
        self_type operator++(int junk) { // POSTFIX
            self_type i = *this;
            ++(*this);
            return i;
        }
        value_type operator*() const {
            return cursor;
        }
        value_type operator->() const {
            return cursor;
        }
        bool operator==(const self_type& rhs) {
            return (set == rhs.set) and (cursor == rhs.cursor);
        }
        bool operator!=(const self_type& rhs) {
            return not (*this == rhs);
        }
    };

    set_iterator set_begin() const {
        return set_iterator(this);
    }
    set_iterator set_end() const {
        return set_iterator(this, N);
    }

    unset_iterator unset_begin() const {
        return unset_iterator(this);
    }
    unset_iterator unset_end() const {
        return unset_iterator(this, N);
    }
}; // end of BitSet


//-------------------------------------------------------------------------
// generic helper functions
//-------------------------------------------------------------------------

inline bool is_valid_short(char c) {
    return ((c >= '0' and c <= '9'))
        or ((c >= 'a') and (c <= 'z'))
        or ((c >= 'A') and (c <= 'Z'));
}

void arg_string(std::ostream& ss, char s, const char* l, bool pad = true) {
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
std::string arg_string(char s, const char* l, bool pad = true) {
    std::stringstream ss;
    arg_string(ss, s, l, pad);
    return ss.str();
}



//-------------------------------------------------------------------------
// errors
//-------------------------------------------------------------------------

// Error type thrown when user input is bad.
class ParseError : public std::exception {
protected:
    std::string err;

public:
    ParseError(const char* e) : err(e) {}
    ParseError(std::string e) : err(std::move(e)) {}

    const char* what() const throw () { return err.c_str(); }
};

// Error type thrown when internal state or CLI building
// contains an error.
class InternalError : public std::exception {
public:
protected:
    std::string err;

public:
    InternalError(const char* e) : err(e) {}
    InternalError(std::string e) : err(std::move(e)) {}

    const char * what () const throw () { return err.c_str(); }
};

// Error type thrown when required argument not given
class MissingArgumentError : public std::exception {
protected:
    std::string err;

public:
    MissingArgumentError(char short_name, const char* long_name) {
        std::stringstream ss;
        ss << "missing argument: ";
        arg_string(ss, short_name, long_name, false);
        err = ss.str();
    }

    const char* what() const throw () { return err.c_str(); }
};


//-------------------------------------------------------------------------
// arg -> ctor delegation
//-------------------------------------------------------------------------

// generic ctor fallback
template <typename Into>
Into From(const char* s) {
    return Into(s);
}

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


// emplace -- containters
template <typename Into>
auto Emplace(Into& into, const char* arg)
-> typename std::enable_if<
    std::is_constructible<typename Into::value_type, const char*>::value,
void>::type
{
    into.emplace_back(arg);
}
template <typename Into>
auto Emplace(Into& into, const char* arg)
-> typename std::enable_if<
    not std::is_constructible<typename Into::value_type, const char*>::value
    and std::is_move_constructible<typename Into::value_type>::value
, void>::type
{
    into.emplace_back(std::move(From<typename Into::value_type>(arg)));
}
template <typename Into>
auto Emplace(Into& into, const char* arg)
-> typename std::enable_if<
    not std::is_constructible<typename Into::value_type, const char*>::value
    and not std::is_move_constructible<typename Into::value_type>::value
, void>::type
{
    into.push_back(From<typename Into::value_type>(arg));
}


//-------------------------------------------------------------------------
// shared / fwdecls / enums
//-------------------------------------------------------------------------

enum class ArgReq : std::uint8_t {
    Optional = 0,
    Required
};


//-------------------------------------------------------------------------
// help / printing descriptors
//-------------------------------------------------------------------------

struct Description {
    const char* name = nullptr;
    std::size_t name_len = 0;

    const char* short_desc = nullptr;
    std::size_t short_len = 0;

    const char* long_desc = nullptr;
    std::size_t long_len = 0;

    Description() = default; // TODO: maybe not
    Description(
        const char* n, std::size_t n_len,
        const char* s, std::size_t s_len,
        const char* l, std::size_t l_len
    )
        : name(n), name_len(n_len)
        , short_desc(s), short_len(s_len)
        , long_desc(l), long_len(s_len)
    {}
    Description(const char* n, const char* s, const char* l)
        // TODO: strnlen -- set a MEX_LENGTH variable? #define?
    {
        if (n == nullptr) { n = ""; }
        if (s == nullptr) { s = ""; }
        if (l == nullptr) { l = ""; }

        name = n;
        name_len = strlen(n);

        short_desc = s;
        short_len = strlen(s);

        long_desc = l;
        long_len = strlen(l);
    }
};

struct ArgHelp {
    char short_flag;
    const char* long_flag;

    const char* arg_name; // i.e.   -f/--file FILE
    std::size_t name_len = 0;
    const char* desc;
    std::size_t desc_len = 0;
    ArgReq _require;

    ArgHelp(
        char s, const char* l,
        const char* name=nullptr,
        const char* desc=nullptr
    )
        : short_flag(s)
        , long_flag(l)
        , arg_name(name)
        , desc(desc)
    {
        // TODO: stnlen -- but need a mex length variable of some sort
        if (name != nullptr) { name_len = strlen(name); } else { name = ""; }
        if (desc != nullptr) { desc_len = strlen(desc); } else { desc = ""; }
    }

    std::size_t left_col_width() const {
        // TODO: get count without creating the string
        // 1 for the space between arg and name
        return arg_string(short_flag, long_flag).size() + name_len + 1;
    }

    std::string flags_string() const {
        return arg_string(short_flag, long_flag);
    }

    bool required() const {
        return _require == ArgReq::Required;
    }
};

struct PositionalHelp {
    const char* name; // i.e.   -f/--file FILE
    std::size_t name_len = 0;
    const char* desc;
    std::size_t desc_len = 0;
    bool _is_variadic = false;

    PositionalHelp(const char* name, const char* desc=nullptr)
        : name(name)
        , desc(desc)
        , _is_variadic(false)
    {
        // TODO: strnlen -- but need a max length variable of some sort
        if (name != nullptr) { name_len = strlen(name); } else { name = ""; }
        if (desc != nullptr) { desc_len = strlen(desc); } else { desc = ""; }
    }

    PositionalHelp(bool variadic, const char* name, const char* desc=nullptr)
        : PositionalHelp(name, desc)
    {
        _is_variadic = variadic;
    }

    std::size_t left_col_width() const {
        // 1 for the space between arg and name
        // 3 for the '...' in variadics
        return name_len + 1 + (variadic() ? 3 : 0);
    }

    bool variadic() const { return _is_variadic; }
};

class HelpMap {
public:
    using GroupValue = std::pair<Description, std::vector<ArgHelp>>;

    std::vector<Description> _subs;
    std::vector<GroupValue> _groups;
    std::vector<ArgHelp> _args;
    std::vector<PositionalHelp> _pos;

    Description _desc;
    const char* _app_version;

    std::size_t _longest_flag;
    std::size_t _indent_width;

protected:

    static void indent_stream(std::ostream& s, std::size_t indent) {
        for (std::size_t i = 0; i < indent; i++) {
            s << " ";
        }
    }

    // returns whether there are an args registered (including in groups)
    // but subcommands do not count as they are not args
    bool has_args() const {
        if (not _args.empty()) { return true; }
        if (not _pos.empty()) { return true; }

        for (auto& g : _groups) {
            if (not g.second.empty()) { return true; }
        }

        return false;
    }

    static std::string combine_all_shorts(const std::vector<const ArgHelp*> args) {
        std::stringstream ss;
        for (auto& a : args) {
            if (is_valid_short(a->short_flag)) {
                ss << a->short_flag;
            }
        }
        return ss.str();
    }

    static std::string combine_all_nonshorts(const std::vector<const ArgHelp*> args) {
        std::stringstream ss;
        for (auto& a : args) {
            if (is_valid_short(a->short_flag)) {
                continue;
            }

            if (ss.tellp()) {
                ss << " ";
            }

            ss << "--" << a->long_flag;
        }
        return ss.str();
    }

    void print_usage_args(std::ostream& ss) const {
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

public:
    HelpMap()
        : _app_version(nullptr)
        , _longest_flag(0)
        , _indent_width(4)
    {}
    HelpMap(const char* name, const char* short_desc)
        : _desc(name, short_desc, "")
        , _app_version(nullptr)
        , _longest_flag(0)
        , _indent_width(4)
    {}

    void details(const char* name, const char* desc, const char* long_desc="") {
        _desc = Description(name, desc, long_desc);
    }

    template <typename... Args>
    void add_arg(bool in_group, const Args ...h) {
        if (in_group) {
            _groups.back().second.emplace_back(h...);

            auto added = _groups.back().second.back();
            _longest_flag = std::max(
                _longest_flag,
                _indent_width + added.left_col_width() // extra indent
            );
        } else {
            _args.emplace_back(h...);
            _longest_flag = std::max(
                _longest_flag,
                _args.back().left_col_width()
            );
        }
    }

    template <typename... Args>
    void add_positional(const Args ...h) {
        _pos.emplace_back(h...);
        _longest_flag = std::max(
            _longest_flag,
            _pos.back().left_col_width()
        );
    }

    template <typename... Args>
    void add_variadic_positional(const Args ...h) {
        _pos.emplace_back(true, h...);
        _longest_flag = std::max(
            _longest_flag,
            _pos.back().left_col_width()
        );
    }

    void clear_subcommands() {
        _subs.clear();
    }
    void add_subcommand(const char* name, const char* desc) {
        _subs.emplace_back(name, desc, "");
        _longest_flag = std::max(_longest_flag, _subs.back().name_len + _indent_width);
    }

    void new_group(const char* name, const char* desc) {
        std::vector<ArgHelp> vec;
        _groups.emplace_back(Description(name, desc, ""), vec);
    }

    void print(std::ostream& s) const {
        auto right_col_start =  _indent_width + _longest_flag + _indent_width;

        // app leading line and usage
        if (_desc.name_len) {
            // leading line
            s << _desc.name;
            if (_app_version) {
                s << " " << _app_version;
            }
            if (_desc.short_len) {
                s << " - " << _desc.short_desc;
            }
            s << std::endl << std::endl;

            // usage
            s << "usage: " << _desc.name;
            if (has_args()) {
                s << " ";
                print_usage_args(s);
            }
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
            s << std::endl << std::endl;
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
            }
            s << std::endl;
        }

        // args
        if (_args.size()) {
            s << "options:" << std::endl;
            for (auto& a : _args) {
                indent_stream(s, _indent_width);
                s << a.flags_string() << " " << a.arg_name;
                indent_stream(s, right_col_start - a.left_col_width());
                s << a.desc << std::endl;
            }
            s << std::endl;
        }

        // positionals
        if (_args.size()) {
            s << "positionals:" << std::endl;
            for (auto& p : _pos) {
                indent_stream(s, _indent_width);
                s << p.name;
                if (p.variadic()) {
                    s << "...";
                }
                indent_stream(s, right_col_start - p.left_col_width());
                s << p.desc << std::endl;
            }
            s << std::endl;
        }

        // pretty spacing
        s << std::endl;
    }
};


//-------------------------------------------------------------------------
// parsing
//-------------------------------------------------------------------------

struct ParseDesc {
    bool is_short = false;
    bool is_long = false;
    std::uint16_t len = 0;
    std::uint16_t eq_offset = 0;
    std::uint16_t runs_remaining = 0;

    ParseDesc(const char* arg) {
        len = strlen(arg);

        // determine if POTENTIALLY a long or short code
        // for instance, the args "-n -1" may be taking "-1" as an
        // argument to the "-n" operator. it is up to the parser
        // to set the "-1" as used thus we never inspect this struct
        if ( (len>=3) and (arg[0] == '-') and (arg[1] == '-') ) {
            is_long = true;
        } else if ( (len>=2) and (arg[0] == '-') ) {
            is_short = true;
        }

        // not an arg, so keep parsing
        if (is_positional()) {
            return;
        }

        // find the '=' separator if it exists
        for (std::size_t i = 0; i < len; i++) {
            if (arg[i] == '=') {
                eq_offset = i;
                break;
            }
        }

        // argument types that support the '=' separator
        // cannot have runs, so that defaults to 0. otherwise
        // we use the len minus the '-'
        runs_remaining = (eq_offset ? 0 : len-1);
    }

    bool is_positional() const {
        return not (is_short or is_long);
    }

    std::size_t matches(const char* arg, char s) const {
        if (not is_short) { return false; }

        std::size_t result = 0;
        for (std::size_t i = 1; i < len; i++) {
            if (arg[i] == s) {
                result += 1;
            }
        }

        return result;
    }

    bool matches(const char* arg, const char* l) const {
        if (not is_long or l == nullptr) { return false; }

        auto cmplen = eq_offset>0 ? eq_offset : len;
        return strncmp(arg+2, l, cmplen-2) == 0;
    }
};



class Context {
protected:
    BitSet _argset;
    std::vector<ParseDesc> _argdesc;

    std::size_t _argc;
    const char** _argv;

    std::size_t _level;
    bool _help;

public:
    class iterator {
    public:
        struct value {
            std::size_t index;
            const char* c_str;
            ParseDesc& desc;
        };

        using self_type = iterator;
        using value_type = const value;
        using reference = const value_type&;
        using pointer = const value_type*;
        using unset_iterator_category = std::forward_iterator_tag;
        using difference_type = std::size_t;

    protected:
        BitSet::unset_iterator _iter;
        const BitSet::unset_iterator _end;

        const char** _argv;
        std::vector<ParseDesc>& _desc;


    public:
        iterator() = delete;
        iterator(
            const char** argv, std::vector<ParseDesc>& desc,
            const BitSet::unset_iterator begin, const BitSet::unset_iterator end
        )
            : _iter(begin)
            , _end(end)
            , _argv(argv)
            , _desc(desc)
        {}
        iterator(const char** argv, std::vector<ParseDesc>& desc, const BitSet& set)
            : iterator(argv, desc, set.unset_begin(), set.unset_end())
        {}
        inline self_type operator++() {
            _iter++;
            return *this;
        }
        self_type operator++(int junk) { // POSTFIX
            self_type i = *this;
            ++(*this);
            return i;
        }
        value_type operator*() const {
            return value{*_iter, _argv[*_iter], _desc[*_iter]};
        }
        bool operator==(const self_type& rhs) {
            return (_iter == rhs._iter);
        }
        bool operator!=(const self_type& rhs) {
            return not (*this == rhs);
        }

        const char* c_str() const { return _argv[*_iter]; }
        ParseDesc& desc() const { return _desc[*_iter]; }
        std::size_t index() const { return *_iter; }
    };

public:
    Context() = delete;
    Context(std::size_t argc, const char** argv, char help_short='h', const char* help_long="help")
        : _argset(argc)
        , _argc(argc)
        , _argv(argv)
        , _level(0)
        , _help(false)
    {
        _argdesc.reserve(argc);
        for (std::size_t i = 0; i < argc; i++) {
            _argdesc.emplace_back(argv[i]);

            if (not _argdesc.back().is_positional()) {
                if (
                    _argdesc.back().matches(_argv[i], help_short)
                    or _argdesc.back().matches(_argv[i], help_long)
                ) {
                    _help = true;
                    _argset.set(i);
                }
            }
        }
    }

    iterator begin() {
        return iterator(_argv, _argdesc, _argset);
    }
    iterator end() {
        return iterator(_argv, _argdesc, _argset.unset_end(), _argset.unset_end());
    }

    void used(std::size_t i) {
        _argset.set(i);
    }

    std::size_t remaining() const {
        return _argset.remaining();
    }

    // returns nullptr if there are no more args to take
    const char* get_arg_or_eq(std::size_t i) {
        if (_argdesc[i].eq_offset) {
            return &_argv[i][_argdesc[i].eq_offset + 1];
        }

        if (i == (_argset.total()-1)) {
            return nullptr;
        }

        _argset.set(i + 1);
        return _argv[i + 1];
    }

    void next_level() {
        _level++;
    }
    std::size_t level() const {
        return _level;
    }

    bool wants_help() const {
        return _help;
    }
};


class Parser {
protected:
    Context _ctx;

    bool _in_group;
    std::size_t _level;

    std::unique_ptr<HelpMap> _help;

protected:

    template <typename Into>
    auto handle_positional(Into& into, const char* arg)
    -> typename std::enable_if<
        std::is_constructible<typename Into::value_type, const char*>::value,
    void>::type
    {
        into.emplace_back(arg);
    }
    template <typename Into>
    auto handle_positional(Into& into, const char* arg)
    -> typename std::enable_if<std::is_constructible<Into, const char*>::value, void>::type
    {
        into = Into(arg);
    }

public:
    Parser() = default;
    Parser(
        std::size_t argc, const char** argv,
        char help_short='h', const char* help_long="help"
    )
        : _ctx(argc-1, argv+1, help_short, help_long)
        , _in_group(false)
        , _level(0)
    {
        if (_ctx.wants_help()) {
            _help = std::unique_ptr<HelpMap>(new HelpMap());
        }
    }

    inline Parser& done() {
        // handle groups first
        if (_in_group) {
            _in_group = false;
            return *this;
        }

        if (_level == 0) {
            throw InternalError("cannot call done() on top-level parser");
        }

        _level--;
        return *this;
    }

    inline bool wants_help() const {
        return _ctx.wants_help();
    }

    void print() const {
        if (not _ctx.wants_help()) {
            return;
        }

        _help->print(std::cout);
    }

    // finalizer that asserts no unused arguments
    void validate() {
        if (_ctx.remaining()) {
            auto arg = *_ctx.begin();
            std::stringstream ss;
            ss << "unknown argument '" << arg.c_str << "'";
            throw ParseError(ss.str());
        }
    }


    //---------------------------------------------------------------------
    // help setup
    //---------------------------------------------------------------------

    Parser& details(const char* name, const char* desc, const char* long_desc="") {
        if (_ctx.wants_help()) {
            _help->_desc = Description(name, desc, long_desc);
        }

        return *this;
    }

    Parser& version(const char* v) {
        if (_ctx.wants_help()) {
            _help->_app_version = v;
        }

        return *this;
    }

    Parser& indent_width(std::uint8_t w) {
        if (_ctx.wants_help()) {
            _help->_indent_width = w;
        }
        return *this;
    }

    //---------------------------------------------------------------------
    // flag
    //---------------------------------------------------------------------

    // TODO: take T&& to move value?
    Parser& flag(char s, const char* l, const char* desc, bool& into, bool invert=false) {
        if (_level != _ctx.level()) {
            return *this;
        }

        if (wants_help()) {
            _help->add_arg(_in_group, s, l, "", desc);
            return *this;
        }

        bool has_seen = false;
        for (auto& arg : _ctx) {
            if (arg.desc.is_positional()) { continue; }

            auto run_count = arg.desc.matches(arg.c_str, s);
            if (run_count or arg.desc.matches(arg.c_str, l)) {
                // flags can only be set once so if we've seen it already, bail
                if (has_seen or (run_count > 1)) {
                    std::stringstream ss;
                    ss << "flag argument '" << arg_string(s, l) << "' provided more than once";
                    throw ParseError(ss.str());
                }

                has_seen = true;
                into = not invert;
                _ctx.used(arg.index);
            }
        }
        return *this;
    }
    Parser& flag(char s, const char* desc, bool& into, bool invert=false) {
        return flag(s, nullptr, desc, into, invert);
    }
    Parser& flag(const char* l, const char* desc, bool& into, bool invert=false) {
        return flag(0, l, desc, into, invert);
    }


    //---------------------------------------------------------------------
    // count
    //---------------------------------------------------------------------

    // TODO: take T&& to move value?
    template <typename T>
    Parser& count(char s, const char* l, const char* desc, T& into) {
        if (_level != _ctx.level()) {
            return *this;
        }

        if (wants_help()) {
            _help->add_arg(_in_group, s, l, "", desc);
            return *this;
        }

        for (auto& arg : _ctx) {
            if (arg.desc.is_positional()) { continue; }

            auto run_count = arg.desc.matches(arg.c_str, s);
            if (arg.desc.is_short and run_count) {
                into += run_count;
                arg.desc.runs_remaining -= run_count;
            } else if (arg.desc.is_long and arg.desc.matches(arg.c_str, l)) {
                into += 1;
            } else {
                continue;
            }

            if (arg.desc.runs_remaining == 0) {
                _ctx.used(arg.index);
            }
        }
        return *this;
    }
    template <typename T>
    Parser& count(char s, const char* desc, T& into) {
        return count(s, nullptr, desc, into);
    }
    template <typename T>
    Parser& count(const char* l, const char* desc, T& into) {
        return count(0, l, desc, into);
    }


    //---------------------------------------------------------------------
    // arg
    //---------------------------------------------------------------------

    // TODO: take T&& to move value?
    template <typename T>
    Parser& arg(
        char s, const char* l, const char* desc, T& into,
         const char* arg_desc=nullptr, ArgReq req = ArgReq::Optional
    ) {
        if (_level != _ctx.level()) {
            return *this;
        }

        if (wants_help()) {
            _help->add_arg(_in_group, s, l, arg_desc, desc);
            return *this;
        }

        bool has_seen = false;
        for (auto& arg : _ctx) {
            if (arg.desc.is_positional()) { continue; }

            auto run_count = arg.desc.matches(arg.c_str, s);
            bool match_long = arg.desc.matches(arg.c_str, l);

            // expect it to match, otherwise skip
            if (not (run_count or match_long)) {
                continue;
            }

            // it matched, so is it a dupe?
            if (has_seen) {
                std::stringstream ss;
                ss << "argument '" << arg_string(s, l) << "' cannot be provided multiple times";
                throw ParseError(ss.str());
            }

            // if short, disallow runs
            if (arg.desc.is_short and (run_count > 1)) {
                std::stringstream ss;
                ss << "argument '" << s << "' cannot be given in a run";
                throw ParseError(ss.str());
            }

            // get the arg to construct with this may be the next
            // argument in argv or it could be an '=' sep
            auto ctor_arg = _ctx.get_arg_or_eq(arg.index);
            if (ctor_arg == nullptr) {
                std::stringstream ss;
                ss << "no argument value provided to '" << arg_string(s, l) << "'";
                throw ParseError(ss.str());
            }

            // construct the value
            into = From<T>(ctor_arg);

            // mark this arg as done regardless of the eq separator or not
            _ctx.used(arg.index);
            has_seen = true;
        }

        if (not has_seen and req == ArgReq::Required) {
            throw MissingArgumentError(s, l);
        }

        return *this;
    }
    template <typename T>
    Parser& arg(
        char s, const char* desc, T& into,
        const char* arg_desc=nullptr, ArgReq req = ArgReq::Optional
    ) {
        return arg(s, nullptr, desc, into, arg_desc, req);
    }
    template <typename T>
    Parser& arg(
        const char* l, const char* desc, T& into,
        const char* arg_desc=nullptr, ArgReq req = ArgReq::Optional
    ) {
        return arg(0, l, desc, into, arg_desc, req);
    }


    //---------------------------------------------------------------------
    // list
    //---------------------------------------------------------------------

    // TODO: take T&& to move value?
    template <typename T>
    Parser& list(char s, const char* l, const char* desc, T& into, const char* arg_desc=nullptr) {
        if (_level != _ctx.level()) {
            return *this;
        }

        if (wants_help()) {
            _help->add_arg(_in_group, s, l, arg_desc, desc);
            return *this;
        }

        for (auto& arg : _ctx) {
            if (arg.desc.is_positional()) { continue; }

            auto run_count = arg.desc.matches(arg.c_str, s);
            bool match_long = arg.desc.matches(arg.c_str, l);

            // expect it to match, otherwise skip
            if (not (run_count or match_long)) {
                continue;
            }

            // if short, disallow runs
            if (arg.desc.is_short and (run_count > 1)) {
                std::stringstream ss;
                ss << "argument '" << s << "' cannot be given in a run";
                throw ParseError(ss.str());
            }

            // get the arg to construct with this may be the next
            // argument in argv or it could be an '=' sep
            auto ctor_arg = _ctx.get_arg_or_eq(arg.index);
            if (ctor_arg == nullptr) {
                std::stringstream ss;
                ss << "no argument value provided to list '" << arg_string(s, l) << "'";
                throw ParseError(ss.str());
            }

            // emplace the arg into the container
            Emplace(into, ctor_arg);

            // mark this arg as done regardless of the eq separator or not
            _ctx.used(arg.index);
        }

        return *this;
    }
    template <typename T>
    Parser& list(char s, const char* desc, T& into) {
        return list(s, nullptr, desc, into);
    }
    template <typename T>
    Parser& list(const char* l, const char* desc, T& into) {
        return list(0, l, desc, into);
    }

    //---------------------------------------------------------------------
    // subcommand
    //---------------------------------------------------------------------

    template <typename T>
    auto subcommand(const char* name, const char* desc, T& into)
    -> typename std::enable_if<std::is_constructible<T, const char*>::value, Parser&>::type
    {
        // if we are entering this block, everything until the done() call
        // is in the next level. so incr and wait for decr
        _level++;

        if (_level != (_ctx.level() + 1)) {
            return *this;
        }

        // if we're same level but there are no args, add ourself and return
        if (wants_help() and not _ctx.remaining()) {
            _help->add_subcommand(name, desc);
            return *this;
        }

        auto arg_len = strlen(name);

        // subcommands only operate on the first available arg
        // so we can just use the iterator
        auto arg = *_ctx.begin();
        if (not arg.desc.is_positional()) {
            std::stringstream ss;
            ss << "argument '" << arg.c_str << "' not available at this (sub)command";
            throw ParseError(ss.str());
        }
        if (arg.desc.len != arg_len) { return *this; }

        if (strncmp(name, arg.c_str, arg_len) == 0) {
            into = T(arg.c_str);
            _ctx.used(arg.index);
            _ctx.next_level();

            if (wants_help()) {
                // just reset the name... aggregate all options available
                // to the given subcommand
                _help->details(name, desc);
                // but clear the subcommands as they do not cross levels
                //_help.clear_subcommands();
            }
        }
        else if (wants_help()) {
            // if we dont change levels and have help arg, add ourselves as a subcommand
            _help->add_subcommand(name, desc);
        }

        return *this;
    }
    template <typename T>
    auto subcommand(const char* name, const char* desc, T& into)
    -> typename std::enable_if<
         std::is_constructible<typename T::value_type, const char*>::value
    , Parser&>::type
    {
        const char* test = nullptr;
        this->subcommand(name, desc, test);
        if (test != nullptr) {
            Emplace(into, test);
        }
        return *this;
    }
    Parser& subcommand(const char* name, const char* desc, bool& into) {
        const char* test = nullptr;
        this->subcommand(name, desc, test);
        into = (test != nullptr);
        return *this;
    }


    //---------------------------------------------------------------------
    // group
    //---------------------------------------------------------------------

    Parser& group(const char* name, const char* desc) {
        // groups do nothing when not printing help
        if (not _ctx.wants_help()) {
            return *this;
        }

        if (_in_group) {
            throw InternalError("nested groups are not allowed");
        }

        _in_group = true;
        _help->new_group(name, desc);
        return *this;
    }


    //---------------------------------------------------------------------
    // positionals
    //---------------------------------------------------------------------

    template <typename T>
    Parser& positional(
        const char* name, const char* desc, T& into,
        ArgReq req = ArgReq::Optional
    ) {
        if (_level != _ctx.level()) {
            return *this;
        }

        if (wants_help()) {
            _help->add_positional(name, desc);
            return *this;
        }

        // like subcommands, we  only operate on the first available arg
        // so we can just use the iterator and assert it is positional
        auto arg = _ctx.begin();

        if (arg == _ctx.end()) {
            if (req == ArgReq::Required) {
                throw MissingArgumentError(0, name);
            }
            // no arg here
            return *this;
        }

        if (not arg.desc().is_positional()) {
            std::stringstream ss;
            ss << "argument '" << arg.c_str() << "' not available at this (sub)command";
            throw ParseError(ss.str());
        }

        handle_positional(into, arg.c_str());
        _ctx.used(arg.index());
        return *this;
    }

    // return void to terminate the chain
    // does the same as Parser::validate() where an exception is thrown when
    // not all arguments were consumed.
    template <typename T>
    void all_positionals(const char* name, const char* desc, T& into) {
        // becuase this is a finalizer, we do not consider level

        if (wants_help()) {
            _help->add_variadic_positional(name, desc);
            return;
        }

        for (auto& a : _ctx) {
            if (not a.desc.is_positional()) {
                std::stringstream ss;
                ss << "unknown argument '" << a.c_str << "'";
                throw ParseError(ss.str());
            }

            _ctx.used(a.index);
            Emplace(into, a.c_str);
        }
    }
};

} // end ns

#endif
