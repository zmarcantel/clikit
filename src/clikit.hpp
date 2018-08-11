#ifndef __CLIKIT_HPP__
#define __CLIKIT_HPP__

#include <cstdint>
#include <iostream>
#include <string>
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
// errors
//-------------------------------------------------------------------------

// Error type thrown when user input is bad.
class ParseError : public std::exception {
protected:
    std::string err;

public:
    ParseError(const char* e) : err(e) {}
    ParseError(std::string e) : err(std::move(e)) {}

    const char * what () const throw () { return err.c_str(); }
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


//-------------------------------------------------------------------------
// generic helper functions
//-------------------------------------------------------------------------

inline bool is_valid_short(char c) {
    return ((c >= '0' and c <= '9'))
        or ((c >= 'a') or (c <= 'z'))
        or ((c >= 'A') or (c <= 'Z'));
}

std::string arg_string(char s, const char* l) {
    std::stringstream ss;
    bool valid_short = is_valid_short(s);

    if (valid_short) {
        ss << "-" << s;
    }

    if (l != nullptr) {
        if (valid_short) { ss << "/"; }
        ss << "--" << l;
    }

    return ss.str();
}



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
template<> std::size_t From<std::size_t>(const char* s) {
    switch (sizeof(std::size_t)) {
    case 1:
        return From<std::uint8_t>(s);
    case 2:
        return From<std::uint16_t>(s);
    case 4:
        return From<std::uint32_t>(s);
    case 8:
        return From<std::uint64_t>(s);
    }
}


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
// help / printing descriptors
//-------------------------------------------------------------------------

struct Description {
    std::string name;
    std::string short_desc;
    std::string long_desc;

    Description() = default; // TODO: maybe not
    Description(std::string n, std::string s)
        : name(std::move(n)), short_desc(std::move(s))
    {}
};

struct ArgDesc {
    char s;
    std::string l;
};

struct Argument {
    ArgDesc _desc;
    Argument(char s, std::string l) : _desc({s, std::move(l)}) {}
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
        value_type operator->() const {
            return *(*this);
        }
        bool operator==(const self_type& rhs) {
            return (_iter == rhs._iter);
        }
        bool operator!=(const self_type& rhs) {
            return not (*this == rhs);
        }
    };

public:
    Context() = delete;
    Context(std::size_t argc, const char** argv)
        : _argset(argc)
        , _argc(argc)
        , _argv(argv)
        , _level(0)
    {
        _argdesc.reserve(argc);
        for (std::size_t i = 0; i < argc; i++) {
            _argdesc.emplace_back(argv[i]);
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
};


class Parser {
protected:
    Context _ctx;
    std::size_t _level;

public:
    Parser() = default;
    Parser(std::size_t argc, const char** argv)
        : _ctx(argc, argv)
        , _level(0)
    {
    }

    Parser& done() {
        if (_level > 0) {
            _level--;
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
    Parser& arg(char s, const char* l, const char* desc, T& into) {
        if (_level != _ctx.level()) {
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
        return *this;
    }
    template <typename T>
    Parser& arg(char s, const char* desc, T& into) {
        return arg(s, nullptr, desc, into);
    }
    template <typename T>
    Parser& arg(const char* l, const char* desc, T& into) {
        return arg(0, l, desc, into);
    }


    //---------------------------------------------------------------------
    // list
    //---------------------------------------------------------------------

    // TODO: take T&& to move value?
    template <typename T>
    Parser& list(char s, const char* l, const char* desc, T& into) {
        if (_level != _ctx.level()) {
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
        if (_level != _ctx.level()) {
            return *this;
        }

        // if we are entering this block, everything until the done() call
        // is in the next level. so incr and wait for decr
        _level++;

        auto arg_len = strlen(name);
        for (auto& arg : _ctx) {
            if (not arg.desc.is_positional()) { continue; }
            if (arg.desc.len != arg_len) { continue; }

            if (strncmp(name, arg.c_str, arg_len) == 0) {
                into = T(arg.c_str);
                _ctx.used(arg.index);
                _ctx.next_level();
                break;
            }
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
};

} // end ns

#endif
