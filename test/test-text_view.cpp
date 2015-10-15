// Copyright (c) 2015, Tom Honermann
//
// This file is distributed under the MIT License. See the accompanying file
// LICENSE.txt or http://www.opensource.org/licenses/mit-license.php for terms
// and conditions.

#include <text_view_detail/adl_customization.hpp>
#include <text_view_detail/advance_to.hpp>
#include <text_view_detail/riterator.hpp>
#include <algorithm>
#include <array>
#include <cassert>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <forward_list>
#include <list>
#include <vector>
#include <utility>
#include <string>
#include <text_view>


using namespace std;
using namespace std::experimental;


// code_unit_map specifies the relationship between a state transition or a
// character with a code unit sequence.  Any of the state_transitions,
// characters, and code_units vectors may be empty.  In general,
// state_transitions or characters will have one element and code_units will be
// non-empty.  Encoding a state transition may or may not require encoding a
// sequence of code units.  State transitions that encode code units must not be
// specified along with a character since decode tests would then be unable to
// validate the underlying code unit sequence specific to the character.
template<Codec CT>
struct code_unit_map {
    using state_transition_type = typename CT::state_transition_type;
    using code_unit_type = typename CT::code_unit_type;
    using character_type = typename CT::character_type;

    // FIXME: std::optional would be a better choice than vector for specifying
    // FIXME: a state transition or a character.  The intent is to allow
    // FIXME: associating zero or one state transitions or characters with a
    // FIXME: code unit sequence.
    vector<state_transition_type> state_transitions;
    vector<character_type> characters;
    vector<code_unit_type> code_units;
};

template<Codec CT>
using code_unit_map_sequence = vector<code_unit_map<CT>>;

template<Codec CT>
int character_count(const code_unit_map_sequence<CT> &code_unit_maps) {
    int result = 0;
    for (const auto &cum : code_unit_maps) {
        result += cum.characters.size();
    }
    return result;
}

// Input iterator model.  The standard doesn't provide input iterator types that
// don't also satisfy forward iterator requirements or impose additional
// requirements.  istream_iterator, for example, requires a char_traits
// specialization for its character type.
template<origin::Input_iterator IT>
class input_iterator
    : public iterator<
                 input_iterator_tag,
                 typename iterator_traits<IT>::value_type,
                 typename iterator_traits<IT>::difference_type,
                 typename iterator_traits<IT>::pointer,
                 typename iterator_traits<IT>::reference>
{
public:
    input_iterator(IT it) : it(it) {}

    friend bool operator==(
        const input_iterator &l,
        const input_iterator &r)
    {
        return l.it == r.it;
    }
    friend bool operator!=(
        const input_iterator &l,
        const input_iterator &r)
    {
        return !(l == r);
    }

    auto operator*() const -> typename iterator_traits<IT>::reference {
        return *it;
    }

    auto operator++() -> input_iterator& {
        ++it;
        return *this;
    }
    auto operator++(int) -> input_iterator {
        input_iterator tmp{*this};
        ++*this;
        return tmp;
    }

private:
    IT it;
};


// Output iterator model.  The standard doesn't provide output iterator types
// that don't also satisfy forward iterator requirements or impose additional
// requirements.  ostream_iterator, for example, requires a char_traits
// specialization for its character type.
template<
    typename IT,
    typename T>
requires origin::Output_iterator<IT, T>()
class output_iterator
    : public iterator<
                 output_iterator_tag,
                 typename iterator_traits<IT>::value_type,
                 typename iterator_traits<IT>::difference_type,
                 typename iterator_traits<IT>::pointer,
                 typename iterator_traits<IT>::reference>
{
public:
    output_iterator(IT it) : it(it) {}

    friend bool operator==(
        const output_iterator &l,
        const output_iterator &r)
    {
        return l.it == r.it;
    }
    friend bool operator!=(
        const output_iterator &l,
        const output_iterator &r)
    {
        return !(l == r);
    }

    auto operator*() -> output_iterator& {
        return *this;
    }
    void operator=(const T& t) {
        *it = t;
    }

    auto operator++() -> output_iterator& {
        ++it;
        return *this;
    }
    auto operator++(int) -> output_iterator {
        output_iterator tmp{*this};
        ++*this;
        return tmp;
    }

private:
    IT it;
};


// Input range model.  The standard doesn't provide a container type with
// input iterators that aren't also forward, bidirectional, or random access
// iterators.  input_range_view is used to provide wrapped input iterators
// for another container type.
template<origin::Range RT>
class input_range_view {
public:
    input_range_view(RT &r) : r(r) {}

    auto begin() const
        -> input_iterator<decltype(text_detail::adl_begin(declval<const RT>()))>
    {
        return input_iterator<
            decltype(text_detail::adl_begin(declval<const RT>()))>{text_detail::adl_begin(r)};
    }
    auto end() const
        -> input_iterator<decltype(text_detail::adl_end(declval<const RT>()))>
    {
        return input_iterator<
            decltype(text_detail::adl_end(declval<const RT>()))>{text_detail::adl_end(r)};
    }

private:
    RT &r;
};


// Iterable model.  The standard doesn't provide a container type that has
// different return types for begin() and end() as permitted by the N4382
// Iterable concept.  This template class is used to wrap a type that models
// the N4382 Range concept to produce a type that merely models the N4382
// Iterable concept.
template<origin::Range RT>
class iterable_view {
public:
    using range_type = origin::Remove_reference<RT>;
    using iterator = origin::Iterator_type<RT>;

    class sentinel {
    public:
        sentinel(iterator i) : i{i} {}

        friend bool operator==(
            const sentinel &l,
            const sentinel &r)
        {
            // Sentinels always compare equal regardless of any internal state.
            // See N4128, 10.1 "Sentinel Equality".
            return true;
        }
        friend bool operator!=(
            const sentinel &l,
            const sentinel &r)
        {
            return !(l == r);
        }

        friend bool operator==(
            const iterator &i,
            const sentinel &s)
        {
            return i == s.i;
        }
        friend bool operator!=(
            const iterator &i,
            const sentinel &s)
        {
            return !(i == s);
        }
        friend bool operator==(
            const sentinel &s,
            const iterator &i)
        {
            return i == s;
        }
        friend bool operator!=(
            const sentinel &s,
            const iterator &i)
        {
            return !(s == i);
        }

        friend bool operator<(
            const sentinel &l,
            const sentinel &r)
        {
            // Sentinels always compare equal regardless of any internal state.
            // See N4128, 10.1 "Sentinel Equality".
            return false;
        }   
        friend bool operator>(
            const sentinel &l,
            const sentinel &r)
        {
            return r < l;
        }   
        friend bool operator<=(
            const sentinel &l,
            const sentinel &r)
        {
            return !(r < l);
        }   
        friend bool operator>=(
            const sentinel &l,
            const sentinel &r)
        {
            return !(l < r);
        }   

        friend bool operator<(
            const iterator &i,
            const sentinel &s)
        {   
            return i < s.i;
        }   
        friend bool operator>(
            const iterator &i,
            const sentinel &s)
        {   
            return i > s.i;
        }   
        friend bool operator<=(
            const iterator &i,
            const sentinel &s)
        {   
            return i <= s.i;
        }   
        friend bool operator>=(
            const iterator &i,
            const sentinel &s)
        {   
            return i >= s.i;
        }   

        friend bool operator<(
            const sentinel &s,
            const iterator &i)
        {
            return s.i < i;
        }
        friend bool operator>(
            const sentinel &s,
            const iterator &i)
        {
            return s.i > i;
        }
        friend bool operator<=(
            const sentinel &s,
            const iterator &i)
        {
            return s.i <= i;
        }
        friend bool operator>=(
            const sentinel &s,
            const iterator &i)
        {
            return s.i >= i;
        }

        iterator base() const {
            return i;
        }

    private:
        iterator i;
    };

    iterable_view(range_type &r) : r{r} {}

    iterator begin() const {
        return text_detail::adl_begin(r);
    }
    sentinel end() const {
        return sentinel(text_detail::adl_end(r));
    }

private:
    range_type &r;
};


void test_code_unit_models() {
    // Archetypes
    static_assert(Code_unit<code_unit_archetype>(), "");
    // std
    static_assert(Code_unit<char>(), "");
    static_assert(Code_unit<wchar_t>(), "");
    static_assert(Code_unit<char16_t>(), "");
    static_assert(Code_unit<char32_t>(), "");
    static_assert(Code_unit<unsigned char>(), "");
    static_assert(Code_unit<unsigned short>(), "");
    static_assert(Code_unit<unsigned int>(), "");
    static_assert(Code_unit<unsigned long>(), "");
    static_assert(Code_unit<unsigned long long>(), "");
}

void test_code_point_models() {
    // Archetypes
    static_assert(Code_point<code_point_archetype>(), "");
    // std
    static_assert(Code_point<char>(), "");
    static_assert(Code_point<wchar_t>(), "");
    static_assert(Code_point<char16_t>(), "");
    static_assert(Code_point<char32_t>(), "");
    static_assert(Code_point<unsigned char>(), "");
    static_assert(Code_point<unsigned short>(), "");
    static_assert(Code_point<unsigned int>(), "");
    static_assert(Code_point<unsigned long>(), "");
    static_assert(Code_point<unsigned long long>(), "");
}

void test_character_set_models() {
    // Archetypes
    static_assert(Character_set<character_set_archetype>(), "");
    // std
    static_assert(Character_set<execution_character_set>(), "");
    static_assert(Character_set<execution_wide_character_set>(), "");
    static_assert(Character_set<universal_character_set>(), "");
    static_assert(Character_set<any_character_set>(), "");
}

void test_character_models() {
    // Archetypes
    static_assert(Character<character_archetype>(), "");
    // std
    static_assert(Character<character<execution_character_set>>(), "");
    static_assert(Character<character<execution_wide_character_set>>(), "");
    static_assert(Character<character<universal_character_set>>(), "");
    static_assert(Character<character<any_character_set>>(), "");
}

void test_code_unit_iterator_models() {
    // Archetypes
    static_assert(Code_unit_iterator<code_unit_iterator_archetype>(), "");
    // std
    static_assert(Code_unit_iterator<char*>(), "");
    static_assert(Code_unit_iterator<const char*>(), "");
    static_assert(Code_unit_iterator<wchar_t*>(), "");
    static_assert(Code_unit_iterator<char16_t*>(), "");
    static_assert(Code_unit_iterator<char32_t*>(), "");
}

void test_codec_state_models() {
    // Archetypes
    static_assert(Codec_state<codec_state_archetype>(), "");
    // std
    static_assert(Codec_state<trivial_codec_state>(), "");
}

void test_codec_models() {
    // Archetypes
    static_assert(Forward_codec<
                      codec_archetype,
                      code_unit_iterator_archetype>(), "");
    static_assert(Bidirectional_codec<
                      codec_archetype,
                      code_unit_iterator_archetype>(), "");
    // std
    static_assert(Bidirectional_codec<
                      trivial_codec<character_archetype, code_unit_archetype>,
                      code_unit_iterator_archetype>(), "");
    static_assert(Bidirectional_codec<
                      utf8_encoding::codec_type,
                      char*>(), "");
    static_assert(Bidirectional_codec<
                      utf16_encoding::codec_type,
                      char16_t*>(), "");
    static_assert(Bidirectional_codec<
                      utf16be_encoding::codec_type,
                      uint_least8_t*>(), "");
    static_assert(Bidirectional_codec<
                      utf16le_encoding::codec_type,
                      uint_least8_t*>(), "");
    static_assert(Bidirectional_codec<
                      utf32_encoding::codec_type,
                      char32_t*>(), "");
    static_assert(Bidirectional_codec<
                      utf32be_encoding::codec_type,
                      uint_least8_t*>(), "");
    static_assert(Bidirectional_codec<
                      utf32le_encoding::codec_type,
                      uint_least8_t*>(), "");
    static_assert(Forward_codec<
                      basic_execution_character_encoding::codec_type,
                      char*>(), "");
    static_assert(Forward_codec<
                      basic_execution_wide_character_encoding::codec_type,
                      wchar_t*>(), "");
#if defined(__STDC_ISO_10646__)
    static_assert(Bidirectional_codec<
                      iso_10646_wide_character_encoding::codec_type,
                      wchar_t*>(), "");
#endif // __STDC_ISO_10646__
    static_assert(Forward_codec<
                      execution_character_encoding::codec_type,
                      char*>(), "");
    static_assert(Forward_codec<
                      execution_wide_character_encoding::codec_type,
                      wchar_t*>(), "");
    static_assert(Bidirectional_codec<
                      char8_character_encoding::codec_type,
                      char*>(), "");
    static_assert(Bidirectional_codec<
                      char16_character_encoding::codec_type,
                      char16_t*>(), "");
    static_assert(Bidirectional_codec<
                      char32_character_encoding::codec_type,
                      char32_t*>(), "");
}

void test_encoding_models() {
    // Archetypes
    static_assert(Encoding<encoding_archetype>(), "");
    // std
    static_assert(Encoding<utf8_encoding>(), "");
    static_assert(Encoding<utf16_encoding>(), "");
    static_assert(Encoding<utf16be_encoding>(), "");
    static_assert(Encoding<utf16le_encoding>(), "");
    static_assert(Encoding<utf32_encoding>(), "");
    static_assert(Encoding<utf32be_encoding>(), "");
    static_assert(Encoding<utf32le_encoding>(), "");
    static_assert(Encoding<basic_execution_character_encoding>(), "");
    static_assert(Encoding<basic_execution_wide_character_encoding>(), "");
#if defined(__STDC_ISO_10646__)
    static_assert(Encoding<iso_10646_wide_character_encoding>(), "");
#endif
    static_assert(Encoding<execution_character_encoding>(), "");
    static_assert(Encoding<execution_wide_character_encoding>(), "");
    static_assert(Encoding<char8_character_encoding>(), "");
    static_assert(Encoding<char16_character_encoding>(), "");
    static_assert(Encoding<char32_character_encoding>(), "");
}

void test_text_iterator_models() {
    // Archetypes
    static_assert(Text_iterator<text_iterator_archetype>(), "");
    static_assert(Text_iterator<text_view_archetype::iterator>(), "");
    static_assert(Text_iterator<otext_iterator<encoding_archetype, code_unit_iterator_archetype>>(), "");
    // std input iterators
    static_assert(Text_iterator<itext_iterator<basic_execution_character_encoding, char(&)[5]>>(), "");
    static_assert(Text_iterator<itext_iterator<basic_execution_wide_character_encoding, wchar_t(&)[5]>>(), "");
#if defined(__STDC_ISO_10646__)
    static_assert(Text_iterator<itext_iterator<iso_10646_wide_character_encoding, wchar_t(&)[5]>>(), "");
#endif
    static_assert(Text_iterator<itext_iterator<utf8_encoding, char(&)[5]>>(), "");
    static_assert(Text_iterator<itext_iterator<utf16_encoding, char16_t(&)[5]>>(), "");
    static_assert(Text_iterator<itext_iterator<utf16be_encoding, uint_least8_t(&)[5]>>(), "");
    static_assert(Text_iterator<itext_iterator<utf16le_encoding, uint_least8_t(&)[5]>>(), "");
    static_assert(Text_iterator<itext_iterator<utf32_encoding, char32_t(&)[5]>>(), "");
    static_assert(Text_iterator<itext_iterator<utf32be_encoding, uint_least8_t(&)[5]>>(), "");
    static_assert(Text_iterator<itext_iterator<utf32le_encoding, uint_least8_t(&)[5]>>(), "");
    // std output iterators
    static_assert(Text_iterator<otext_iterator<basic_execution_character_encoding, char*>>(), "");
    static_assert(Text_iterator<otext_iterator<basic_execution_wide_character_encoding, wchar_t*>>(), "");
#if defined(__STDC_ISO_10646__)
    static_assert(Text_iterator<otext_iterator<iso_10646_wide_character_encoding, wchar_t*>>(), "");
#endif
    static_assert(Text_iterator<otext_iterator<utf8_encoding, uint_least8_t*>>(), "");
    static_assert(Text_iterator<otext_iterator<utf16_encoding, char16_t*>>(), "");
    static_assert(Text_iterator<otext_iterator<utf16be_encoding, uint_least8_t*>>(), "");
    static_assert(Text_iterator<otext_iterator<utf16le_encoding, uint_least8_t*>>(), "");
    static_assert(Text_iterator<otext_iterator<utf32_encoding, char32_t*>>(), "");
    static_assert(Text_iterator<otext_iterator<utf32be_encoding, uint_least8_t*>>(), "");
    static_assert(Text_iterator<otext_iterator<utf32le_encoding, uint_least8_t*>>(), "");
}

void test_text_view_models() {
    // Archetypes
    static_assert(Text_view<text_view_archetype>(), "");
    // std
    static_assert(Text_view<text_view>(), "");
    static_assert(Text_view<wtext_view>(), "");
    static_assert(Text_view<u8text_view>(), "");
    static_assert(Text_view<u16text_view>(), "");
    static_assert(Text_view<u32text_view>(), "");
    static_assert(Text_view<basic_text_view<execution_character_encoding, char(&)[5]>>(), "");
    static_assert(Text_view<basic_text_view<execution_character_encoding, const char(&)[5]>>(), "");
    static_assert(Text_view<basic_text_view<execution_wide_character_encoding, wchar_t(&)[5]>>(), "");
    static_assert(Text_view<basic_text_view<char8_character_encoding, uint_least8_t(&)[5]>>(), "");
    static_assert(Text_view<basic_text_view<char16_character_encoding, char16_t(&)[5]>>(), "");
    static_assert(Text_view<basic_text_view<char32_character_encoding, char32_t(&)[5]>>(), "");
    static_assert(Text_view<basic_text_view<execution_character_encoding, string&>>(), "");
    static_assert(Text_view<basic_text_view<execution_character_encoding, array<char, 5>&>>(), "");
    static_assert(Text_view<basic_text_view<execution_character_encoding, vector<char>&>>(), "");
    // FIXME: The following basic_text_view instantiations should be rejected
    // FIXME: since the range type they are instantiated with owns the
    // FIXME: underlying storage.
    static_assert(Text_view<basic_text_view<execution_character_encoding, char[5]>>(), "");
    static_assert(Text_view<basic_text_view<execution_character_encoding, string>>(), "");
    static_assert(Text_view<basic_text_view<execution_character_encoding, array<char, 5>>>(), "");
    static_assert(Text_view<basic_text_view<execution_character_encoding, vector<char>>>(), "");
}

// Test any_character_set.
void test_any_character_set() {
    character<any_character_set> c1(U'\U00011141');
    character<any_character_set> c2(get_character_set_id<unicode_character_set>(), U'\U00011141');
    character<unicode_character_set> c3(U'\U00011141');
    assert(c1 == c1);
    assert(c2 == c2);
    assert(c3 == c3);
    assert(c1 != c2);
    assert(c2 != c1);
    assert(c1.get_character_set_id() != c2.get_character_set_id());
    assert(c1.get_code_point() == c2.get_code_point());
    assert(c2 == c3);
    assert(c3 == c2);
    assert(c2.get_character_set_id() == c3.get_character_set_id());
    assert(c2.get_code_point() == c3.get_code_point());
    c2.set_character_set_id(get_character_set_id<any_character_set>());
    assert(c2 != c3);
    assert(c3 != c2);
    assert(c2.get_character_set_id() != c3.get_character_set_id());
    assert(c2.get_code_point() == c3.get_code_point());
}

// Test forward encoding of the state transitions and characters present in
// 'code_unit_maps' and ensure that the encoded code units match.  'it' is
// expected to be an output iterator for this test.  Characters are encoded via
// 'it' which must write the resulting code units to the container reflected by
// 'code_unit_range'.
template<
    Encoding ET,
    origin::Input_range RT,
    Text_iterator TIT>
requires origin::Output_iterator<TIT, typename ET::codec_type::character_type>()
void test_forward_encode(
    const code_unit_map_sequence<typename ET::codec_type> &code_unit_maps,
    const RT &code_unit_range,
    TIT it)
{
    for (const auto &cum : code_unit_maps) {
        for (auto st : cum.state_transitions) {
            *it++ = st;
        }
        for (auto c : cum.characters) {
            *it++ = c;
        }
    }
    auto code_unit_it = begin(code_unit_range);
    for (const auto &cum : code_unit_maps) {
        for (auto ecu : cum.code_units) {
            auto acu = *code_unit_it++;
            assert(ecu == acu);
        }
    }
}

// Test forward encoding of the state transitions and characters present in
// 'code_unit_maps' and ensure that the encoded code units match.  'it' is
// expected to be i forward, bidirectional, or random access terator for this
// test.  Characters are encoded via 'it' which must write the resulting code
// units to the container reflected by 'code_unit_range'.
template<
    Encoding ET,
    origin::Input_range RT,
    Text_iterator TIT>
requires origin::Forward_iterator<TIT>()
void test_forward_encode(
    const code_unit_map_sequence<typename ET::codec_type> &code_unit_maps,
    const RT &code_unit_range,
    TIT it)
{
    for (const auto &cum : code_unit_maps) {
        for (auto st : cum.state_transitions) {
            *it++ = st;
        }
        for (auto c : cum.characters) {
            *it++ = c;
        }
    }
    assert(it.base() == end(code_unit_range));
    auto code_unit_it = begin(code_unit_range);
    for (const auto &cum : code_unit_maps) {
        for (auto ecu : cum.code_units) {
            auto acu = *code_unit_it++;
            assert(ecu == acu);
        }
    }
    assert(code_unit_it == end(code_unit_range));
}

// Test forward decoding of the code unit sequence present in the
// 'code_unit_range' range using the 'tv' text view of that range.  The
// 'code_unit_maps' sequence provides the state transitions, characters, and
// code unit sequences to compare against.  'tv' is expected to provide input
// iterators for this test.  Despite the text iterators being input iterators,
// this test presumes that text view iteration is restartable so that pre and
// post increment iteration and iterator equality comparisons can be tested.
template<
    origin::Input_range RT,
    Text_view TVT>
requires origin::Input_iterator<origin::Iterator_type<TVT>>()
void test_forward_decode(
    const code_unit_map_sequence<
              typename encoding_type_of<TVT>::codec_type> &code_unit_maps,
    const RT &code_unit_range,
    TVT tv)
{
    // Validate pre-increment iteration.
    auto tvit = begin(tv);
    for (const auto &cum : code_unit_maps) {
        for (auto c : cum.characters) {
            // Decode and advance.
            assert(tvit != end(tv));
            auto tvcp = *tvit;
            ++tvit;
            // Validate the decoded character.
            assert(tvcp == c);
        }
    }
    // Validate base code unit iterators.
    assert(tvit.base() == end(code_unit_range));
    assert(tvit == end(tv));

    // Validate post-increment iteration.
    tvit = begin(tv);
    for (const auto &cum : code_unit_maps) {
        for (auto c : cum.characters) {
            // Decode and advance.
            assert(tvit != end(tv));
            auto tvcp = *tvit++;
            // Validate the decoded character.
            assert(tvcp == c);
        }
    }
    // Validate base code unit iterators.
    assert(tvit.base() == end(code_unit_range));
    assert(tvit == end(tv));

    // Validate iterator equality comparison.
    assert(begin(tv) == begin(tv));
    if (character_count(code_unit_maps) > 0) {
        assert(begin(tv) != end(tv));
    } else {
        assert(begin(tv) == end(tv));
    }
}

// Test forward decoding of the code unit sequence present in the
// 'code_unit_range' range using the 'tv' text view of that range.  The
// 'code_unit_maps' sequence provides the state transitions, characters, and
// code unit sequences to compare against.  'tv' is expected to provide forward,
// bidirectional, or random access iterators for this test.
template<
    origin::Forward_range RT,
    Text_view TVT>
requires origin::Forward_iterator<origin::Iterator_type<TVT>>()
void test_forward_decode(
    const code_unit_map_sequence<
              typename encoding_type_of<TVT>::codec_type> &code_unit_maps,
    const RT &code_unit_range,
    TVT tv)
{
    // Validate pre-increment iteration.
    auto tvit = begin(tv);
    auto cuit = begin(code_unit_range);
    for (const auto &cum : code_unit_maps) {
        // Validate the code unit sequence.
        assert(distance(cuit, text_detail::advance_to(cuit, end(code_unit_range)))
               >= (int)cum.code_units.size());
        assert(equal(
            begin(cum.code_units),
            end(cum.code_units),
            cuit));

        if (cum.characters.empty()) {
            advance(cuit, cum.code_units.size());
        } else for (auto c : cum.characters) {
            // Validate base code unit iterators.
            assert(tvit.base() == cuit);
            assert(begin(tvit.base_range()) == cuit);
            advance(cuit, cum.code_units.size());
            assert(end(tvit.base_range()) == cuit);
            // Validate the underlying code unit sequence.
            assert(equal(
                begin(tvit.base_range()),
                end(tvit.base_range()),
                begin(cum.code_units)));
            // Decode and advance.
            assert(tvit != end(tv));
            auto tvcp = *tvit;
            ++tvit;
            // Validate the decoded character.
            assert(tvcp == c);
        }
    }
    // Validate iteration to the end.
    assert(tvit == end(tv));
    assert(cuit == end(code_unit_range));
    // Validate base code unit iterators.
    assert(tvit.base() == cuit);
    assert(begin(tvit.base_range()) == cuit);
    assert(end(tvit.base_range()) == cuit);
    assert(begin(tvit.base_range()) == end(code_unit_range));

    // Validate post-increment iteration.
    tvit = begin(tv);
    cuit = begin(code_unit_range);
    for (const auto &cum : code_unit_maps) {
        // Validate the underlying code unit sequence.
        assert(distance(cuit, text_detail::advance_to(cuit, end(code_unit_range)))
               >= (int)cum.code_units.size());
        assert(equal(
            begin(cum.code_units),
            end(cum.code_units),
            cuit));

        if (cum.characters.empty()) {
            advance(cuit, cum.code_units.size());
        } else for (auto c : cum.characters) {
            // Validate base code unit iterators.
            assert(tvit.base() == cuit);
            assert(begin(tvit.base_range()) == cuit);
            advance(cuit, cum.code_units.size());
            assert(end(tvit.base_range()) == cuit);
            // Validate the underlying code unit sequence.
            assert(equal(
                begin(tvit.base_range()),
                end(tvit.base_range()),
                begin(cum.code_units)));
            // Decode and advance.
            assert(tvit != end(tv));
            auto tvcp = *tvit++;
            // Validate the decoded character.
            assert(tvcp == c);
        }
    }
    // Validate iteration to the end.
    assert(tvit == end(tv));
    assert(cuit == end(code_unit_range));
    // Validate base code unit iterators.
    assert(tvit.base() == cuit);
    assert(begin(tvit.base_range()) == cuit);
    assert(end(tvit.base_range()) == cuit);
    assert(begin(tvit.base_range()) == end(code_unit_range));

    // Validate iterator equality comparison.
    assert(begin(tv) == begin(tv));
    if (character_count(code_unit_maps) > 0) {
        assert(begin(tv) != end(tv));
    } else {
        assert(begin(tv) == end(tv));
    }

    // Validate underlying iterator access.
    assert(end(tv).base() == end(code_unit_range));
}

// Test reverse decoding of the code unit sequence present in the
// 'code_unit_range' range using the 'tv' text view of that range.  The
// 'code_unit_maps' sequence provides the state transitions, characters, and
// code unit sequences to compare against.  'tv' is expected to provide
// bidirectional or random access iterators for this test.
template<
    origin::Bidirectional_range RT,
    Text_view TVT>
requires origin::Bidirectional_iterator<origin::Iterator_type<TVT>>()
void test_reverse_decode(
    const code_unit_map_sequence<
              typename encoding_type_of<TVT>::codec_type> &code_unit_maps,
    const RT &code_unit_range,
    TVT tv)
{
    // Validate pre-decrement.
    auto tvit = text_detail::advance_to(begin(tv), end(tv));
    auto rcuit = text_detail::rbegin(code_unit_range);
    for (auto cumit = text_detail::rbegin(code_unit_maps);
         cumit != text_detail::rend(code_unit_maps);
         ++cumit)
    {
        auto& cum = *cumit;

        // Validate the code unit sequence.
        assert(distance(rcuit, text_detail::rend(code_unit_range))
               >= (int)cum.code_units.size());
        assert(equal(
            text_detail::rbegin(cum.code_units),
            text_detail::rend(cum.code_units),
            rcuit));

        if (cum.characters.empty()) {
            advance(rcuit, cum.code_units.size());
        } else for (auto c : cum.characters) {
            // Decode and decrement.
            assert(tvit != begin(tv));
            auto tvcp = *--tvit;
            // Validate base code unit iterators.
            assert(end(tvit.base_range()) == rcuit.base());
            advance(rcuit, cum.code_units.size());
            assert(begin(tvit.base_range()) == rcuit.base());
            assert(tvit.base() == rcuit.base());
            // Validate the underlying code unit sequence.
            assert(equal(
                begin(tvit.base_range()),
                end(tvit.base_range()),
                begin(cum.code_units)));
            // Validate the decoded character.
            assert(tvcp == c);
        }
    }
    // Validate iteration to the beginning.
    assert(tvit == begin(tv));
    assert(rcuit == text_detail::rend(code_unit_range));

    // Validate post-decrement.
    tvit = text_detail::advance_to(begin(tv), end(tv));
    rcuit = text_detail::rbegin(code_unit_range);
    for (auto cumit = text_detail::rbegin(code_unit_maps);
         cumit != text_detail::rend(code_unit_maps);
         ++cumit)
    {
        auto& cum = *cumit;

        // Validate the code unit sequence.
        assert(distance(rcuit, text_detail::rend(code_unit_range))
               >= (int)cum.code_units.size());
        assert(equal(
            text_detail::rbegin(cum.code_units),
            text_detail::rend(cum.code_units),
            rcuit));

        if (cum.characters.empty()) {
            advance(rcuit, cum.code_units.size());
        } else for (auto c : cum.characters) {
            // Decode and decrement.
            assert(tvit != begin(tv));
            tvit--;
            auto tvcp = *tvit;
            // Validate base code unit iterators.
            assert(end(tvit.base_range()) == rcuit.base());
            advance(rcuit, cum.code_units.size());
            assert(begin(tvit.base_range()) == rcuit.base());
            assert(tvit.base() == rcuit.base());
            // Validate the underlying code unit sequence.
            assert(equal(
                begin(tvit.base_range()),
                end(tvit.base_range()),
                begin(cum.code_units)));
            // Validate the decoded character.
            assert(tvcp == c);
        }
    }
    // Validate iteration to the beginning.
    assert(tvit == begin(tv));
    assert(rcuit == text_detail::rend(code_unit_range));
}

// Test reverse decoding of the code unit sequence present in the
// 'code_unit_range' range using the 'tv' text view of that range.  The
// 'code_unit_maps' sequence provides the state transitions, characters, and
// code unit sequences to compare against.  'tv' is expected to provide
// bidirectional or random access iterators for this test.

// Test random access decoding of the code unit sequence present in the
// 'code_unit_range' range using the 'tv' text view of that range.  The
// 'code_unit_maps' sequence provides the state transitions, characters, and
// code unit sequences to compare against.  'tv' is expected to provide random
// access iterators for this test.
template<
    origin::Random_access_range RT,
    Text_view TVT>
requires origin::Random_access_iterator<origin::Iterator_type<TVT>>()
void test_random_decode(
    const code_unit_map_sequence<
              typename encoding_type_of<TVT>::codec_type> &code_unit_maps,
    const RT &code_unit_range,
    TVT tv)
{
    // Validate random access.
    int i = 0;
    auto tvit = begin(tv);
    auto cuit = begin(code_unit_range);
    for (const auto &cum : code_unit_maps) {
        // Validate the underlying code unit sequence.
        assert(distance(cuit, text_detail::advance_to(cuit, end(code_unit_range)))
               >= (int)cum.code_units.size());
        assert(equal(
            begin(cum.code_units),
            end(cum.code_units),
            cuit));

        if (cum.characters.empty()) {
            advance(cuit, cum.code_units.size());
        } else for (auto c : cum.characters) {
            // Validate base code unit iterators.
            assert((tvit+i).base() == cuit);
            assert(begin((tvit+i).base_range()) == cuit);
            advance(cuit, cum.code_units.size());
            assert(end((tvit+i).base_range()) == cuit);
            // Validate the underlying code unit sequence.
            assert(equal(
                begin((tvit+i).base_range()),
                end((tvit+i).base_range()),
                begin(cum.code_units)));
            // Validate the decoded character.
            assert(tvit[i] == c);
        }
        // Advance.
        ++i;
    }
    // Validate base code unit iterators.
    assert(end(tv).base() == cuit);
    assert(begin(text_detail::advance_to(begin(tv), end(tv)).base_range()) == cuit);
    assert(end(text_detail::advance_to(begin(tv), end(tv)).base_range()) == cuit);

    // Validate random access iterator requirements.
    int num_characters = character_count(code_unit_maps);
    if (num_characters > 0) {
        assert(begin(tv) < end(tv));
        assert(end(tv)   > begin(tv));
    }
    assert(begin(tv) <= end(tv));
    assert(end(tv)   >= begin(tv));
    assert(static_cast<decltype(num_characters)>(
                   text_detail::advance_to(begin(tv), end(tv)) - begin(tv))
               == num_characters);
    assert(begin(tv) + num_characters == end(tv));
    assert(num_characters + begin(tv) == end(tv));
    assert(text_detail::advance_to(begin(tv), end(tv)) - num_characters
           == begin(tv));
    assert(-num_characters + text_detail::advance_to(begin(tv), end(tv))
           == begin(tv));
    assert(text_detail::advance_to(begin(tv), end(tv))[-num_characters]
           == *begin(tv));
    tvit = begin(tv);
    tvit += num_characters;
    assert(tvit == end(tv));
    tvit = text_detail::advance_to(begin(tv), end(tv));
    tvit -= num_characters;
    assert(tvit == begin(tv));
    tvit = text_detail::advance_to(begin(tv), end(tv));
    tvit += -num_characters;
    assert(tvit == begin(tv));
    tvit = begin(tv);
    tvit -= -num_characters;
    assert(tvit == end(tv));
}

// Test forward encoding and decoding of the state transitions, characters, and
// code unit sequences present in the 'code_unit_maps' sequence for the
// character encoding specified by 'ET'.  This test exercises encoding and
// decoding using input and output text iterators with underlying input, output,
// forward, bidirectional, and random access iterators.
template<Encoding ET>
void test_forward_encoding(
    const code_unit_map_sequence<typename ET::codec_type> &code_unit_maps)
{
    using codec_type = typename ET::codec_type;
    using code_unit_type = typename codec_type::code_unit_type;

    int num_code_units = 0;
    for (const auto &cum : code_unit_maps) {
        num_code_units += cum.code_units.size();
    }

    // Test otext_iterator with an underlying output iterator.
    {
    forward_list<code_unit_type> container;
    fill_n(
        front_insert_iterator<decltype(container)>{container},
        num_code_units,
        code_unit_type{});
    using base_iterator_type =
        output_iterator<decltype(begin(container)), code_unit_type>;
    base_iterator_type base_iterator(begin(container));
    otext_iterator<ET, base_iterator_type>
        it(ET::initial_state(), base_iterator);
    test_forward_encode<ET>(code_unit_maps, container, it);
    }

    // Test otext_iterator with an underlying forward iterator.
    {
    forward_list<code_unit_type> container;
    fill_n(
        front_insert_iterator<decltype(container)>{container},
        num_code_units,
        code_unit_type{});
    otext_iterator<ET, decltype(begin(container))>
        it(ET::initial_state(), begin(container));
    test_forward_encode<ET>(code_unit_maps, container, it);
    }

    // Test otext_iterator with an underlying bidirectional iterator.
    {
    list<code_unit_type> container;
    fill_n(
        front_insert_iterator<decltype(container)>{container},
        num_code_units,
        code_unit_type{});
    otext_iterator<ET, decltype(begin(container))>
        it(ET::initial_state(), begin(container));
    test_forward_encode<ET>(code_unit_maps, container, it);
    }

    // Test otext_iterator with an underlying random access iterator.
    {
    vector<code_unit_type> container(num_code_units);
    otext_iterator<ET, decltype(begin(container))>
        it(ET::initial_state(), begin(container));
    test_forward_encode<ET>(code_unit_maps, container, it);
    }


    // Test itext_iterator with an underlying input iterator.
    {
    forward_list<code_unit_type> container;
    auto insert_it = container.before_begin();
    for (const auto &cum : code_unit_maps) {
        for (const auto &cu : cum.code_units) {
            insert_it = container.insert_after(insert_it, cu);
        }
    }
    auto input_container =
        input_range_view<forward_list<code_unit_type>>{container};
    auto tv = make_text_view<ET>(input_container);
    test_forward_decode(code_unit_maps, input_container, tv);
    }

    // Test itext_iterator with an underlying forward iterator.
    {
    forward_list<code_unit_type> container;
    auto insert_it = container.before_begin();
    for (const auto &cum : code_unit_maps) {
        for (const auto &cu : cum.code_units) {
            insert_it = container.insert_after(insert_it, cu);
        }
    }
    auto tv = make_text_view<ET>(container);
    test_forward_decode(code_unit_maps, container, tv);
    }

    // Test itext_iterator with an underlying bidirectional iterator.
    {
    list<code_unit_type> container;
    for (const auto &cum : code_unit_maps) {
        for (const auto &cu : cum.code_units) {
            container.push_back(cu);
        }
    }
    auto tv = make_text_view<ET>(container);
    test_forward_decode(code_unit_maps, container, tv);
    }

    // Test itext_iterator with an underlying random access iterator.
    {
    vector<code_unit_type> container;
    for (const auto &cum : code_unit_maps) {
        for (const auto &cu : cum.code_units) {
            container.push_back(cu);
        }
    }
    auto tv = make_text_view<ET>(container);
    test_forward_decode(code_unit_maps, container, tv);
    }

    // Test itext_iterator with an underlying N4382 Iterable.
    {
    vector<code_unit_type> container;
    for (const auto &cum : code_unit_maps) {
        for (const auto &cu : cum.code_units) {
            container.push_back(cu);
        }
    }
    iterable_view<decltype(container)> iv_container{container};
    auto tv = make_text_view<ET>(iv_container);
    test_forward_decode(code_unit_maps, iv_container, tv);
    }
}

// Test bidirectional encoding and decoding of the state transitions,
// characters, and code unit sequences present in the 'code_unit_maps' sequence
// for the character encoding specified by 'ET'.  This test exercises encoding
// and decoding using input text iterators with underlying bidirectional, and
// random access iterators and reverse output text iterators with underlying
// output, forward, bidirectional, and random access iterators.
template<Encoding ET>
void test_bidirectional_encoding(
    const code_unit_map_sequence<typename ET::codec_type> &code_unit_maps)
{
    test_forward_encoding<ET>(code_unit_maps);

    using codec_type = typename ET::codec_type;
    using code_unit_type = typename codec_type::code_unit_type;

    // Test itext_iterator with an underlying bidirectional iterator.
    {
    list<code_unit_type> container;
    for (const auto &cum : code_unit_maps) {
        for (const auto &cu : cum.code_units) {
            container.push_back(cu);
        }
    }
    auto tv = make_text_view<ET>(container);
    test_reverse_decode(code_unit_maps, container, tv);
    }

    // Test itext_iterator with an underlying random access iterator.
    {
    vector<code_unit_type> container;
    for (const auto &cum : code_unit_maps) {
        for (const auto &cu : cum.code_units) {
            container.push_back(cu);
        }
    }
    auto tv = make_text_view<ET>(container);
    test_reverse_decode(code_unit_maps, container, tv);
    }

    // Test itext_iterator with an underlying N4382 Iterable.
    {
    vector<code_unit_type> container;
    for (const auto &cum : code_unit_maps) {
        for (const auto &cu : cum.code_units) {
            container.push_back(cu);
        }
    }
    iterable_view<decltype(container)> iv_container{container};
    auto tv = make_text_view<ET>(iv_container);
    test_reverse_decode(code_unit_maps, iv_container, tv);
    }
}

// Test random access encoding and decoding of the state transitions,
// characters, and code unit sequences present in the 'code_unit_maps' sequence
// for the character encoding specified by 'ET'.  This test exercises decoding
// using input text iterators with underlying random access iterators.
template<Encoding ET>
void test_random_access_encoding(
    const code_unit_map_sequence<typename ET::codec_type> &code_unit_maps)
{
    test_bidirectional_encoding<ET>(code_unit_maps);

    using codec_type = typename ET::codec_type;
    using code_unit_type = typename codec_type::code_unit_type;

    // Test itext_iterator with an underlying random access iterator.
    {
    vector<code_unit_type> container;
    for (const auto &cum : code_unit_maps) {
        for (const auto &cu : cum.code_units) {
            container.push_back(cu);
        }
    }
    auto tv = make_text_view<ET>(container);
    test_random_decode(code_unit_maps, container, tv);
    }

    // Test itext_iterator with an underlying N4382 Iterable.
    {
    vector<code_unit_type> container;
    for (const auto &cum : code_unit_maps) {
        for (const auto &cu : cum.code_units) {
            container.push_back(cu);
        }
    }
    iterable_view<decltype(container)> iv_container{container};
    auto tv = make_text_view<ET>(iv_container);
    test_random_decode(code_unit_maps, iv_container, tv);
    }
}

template<
    Text_view TVT,
    origin::Input_range RT>
void test_text_view(
    const code_unit_map_sequence<
              typename encoding_type_of<TVT>::codec_type> &code_unit_maps,
    const RT &code_unit_range,
    TVT tv)
{
    test_forward_decode(code_unit_maps, code_unit_range, tv);
}

template<
    Text_view TVT,
    size_t cstr_length,
    size_t ary_length,
    typename String>
void test_construct_text_view(
    const code_unit_map_sequence<typename encoding_type_of<TVT>::codec_type>
        &code_unit_maps_with_terminator,
    const code_unit_map_sequence<typename encoding_type_of<TVT>::codec_type>
        &code_unit_maps_without_terminator,
    const typename encoding_type_of<TVT>::codec_type::
        code_unit_type (&cstr)[cstr_length],
    const array<
        typename encoding_type_of<TVT>::codec_type::code_unit_type,
        ary_length> &ary,
    const String &str,
    const initializer_list<
        const typename encoding_type_of<TVT>::codec_type::code_unit_type> &il)
{
    using ET = encoding_type_of<TVT>;
    using RT = typename TVT::range_type;
    using CUMS = code_unit_map_sequence<typename encoding_type_of<TVT>::codec_type>;

    // Note: copy-initialization is used in these tests to ensure these forms
    // are valid for function arguments.  list-initialization is used to ensure
    // no interference from initializer-list constructors.

    // Test initialization via the default constructor.
    TVT tv1 = {};
    test_text_view<TVT>(CUMS{}, RT{}, tv1);

    // Test initialization with an explicit initial state and underlying range.
    TVT tv2 = {ET::initial_state(), RT{&cstr[0], &cstr[cstr_length]}};
    test_text_view<TVT>(code_unit_maps_with_terminator,
                        RT{&cstr[0], &cstr[cstr_length]},
                        tv2);

    // Test initialization with an implicit initial state and underlying range.
    TVT tv3 = {RT{&cstr[0], &cstr[cstr_length]}};
    test_text_view<TVT>(code_unit_maps_with_terminator,
                        RT{&cstr[0], &cstr[cstr_length]},
                        tv3);

    // Test initialization with an explicit initial state and an iterator range.
    TVT tv4 = {ET::initial_state(), &cstr[0], &cstr[cstr_length]};
    test_text_view<TVT>(code_unit_maps_with_terminator,
                        RT{&cstr[0], &cstr[cstr_length]},
                        tv4);

    // Test initialization with an implicit initial state and an iterator range.
    TVT tv5 = {&cstr[0], &cstr[cstr_length]};
    test_text_view<TVT>(code_unit_maps_with_terminator,
                        RT{&cstr[0], &cstr[cstr_length]},
                        tv5);

    // Test initialization with an explicit initial state and a sized iterator range.
    TVT tv6 = {ET::initial_state(), &cstr[0], cstr_length};
    test_text_view<TVT>(code_unit_maps_with_terminator,
                        RT{&cstr[0], &cstr[cstr_length]},
                        tv6);

    // Test initialization with an implicit initial state and a sized iterator range.
    TVT tv7 = {&cstr[0], cstr_length};
    test_text_view<TVT>(code_unit_maps_with_terminator,
                        RT{&cstr[0], &cstr[cstr_length]},
                        tv7);

    // Test initialization with an explicit initial state and a std::string.
    TVT tv8 = {ET::initial_state(), str};
    test_text_view<TVT>(code_unit_maps_without_terminator,
                        RT{str.c_str(), str.c_str() + str.size()},
                        tv8);

    // Test initialization with an implicit initial state and a std::string.
    TVT tv9 = {str};
    test_text_view<TVT>(code_unit_maps_without_terminator,
                        RT{str.c_str(), str.c_str() + str.size()},
                        tv9);

    // Test initialization with an explicit initial state and an iterable (C array).
    TVT tv10 = {ET::initial_state(), cstr};
    test_text_view<TVT>(code_unit_maps_with_terminator,
                        RT{&cstr[0], &cstr[cstr_length]},
                        tv10);

    // Test initialization with an implicit initial state and an iterable (C array).
    TVT tv11 = {cstr};
    test_text_view<TVT>(code_unit_maps_with_terminator,
                        RT{&cstr[0], &cstr[cstr_length]},
                        tv11);

    // Test initialization with an explicit initial state and an iterable (std::array).
    TVT tv12 = {ET::initial_state(), ary};
    test_text_view<TVT>(code_unit_maps_without_terminator,
                        ary,
                        tv12);

    // Test initialization with an implicit initial state and an iterable (std::array).
    TVT tv13 = {ary};
    test_text_view<TVT>(code_unit_maps_without_terminator,
                        ary,
                        tv13);

    // Test initialization with an explicit initial state and an iterable (std::initializer_list).
    TVT tv14 = {ET::initial_state(), il};
    test_text_view<TVT>(code_unit_maps_without_terminator,
                        il,
                        tv14);

    // Test initialization with an implicit initial state and an iterable (std::initializer_list).
    TVT tv15 = {il};
    test_text_view<TVT>(code_unit_maps_without_terminator,
                        il,
                        tv15);

    // Test initialization with an explicit initial state and an iterable (make_cstr_range).
    TVT tv16 = {ET::initial_state(), make_cstr_range(cstr)};
    test_text_view<TVT>(code_unit_maps_without_terminator,
                        make_cstr_range(cstr),
                        tv16);
                             
    // Test initialization with an implicit initial state and an iterable (make_cstr_range).
    TVT tv17 = {make_cstr_range(cstr)};
    test_text_view<TVT>(code_unit_maps_without_terminator,
                        make_cstr_range(cstr),
                        tv17);

    // Test initialization with a text iterator pair.
    TVT tv18 = {begin(tv17), end(tv17)};
    test_text_view<TVT>(code_unit_maps_without_terminator,
                        RT{begin(begin(tv17).base_range()), end(end(tv17).base_range())},
                        tv18);
                             
    // Test initialization via the copy constructor.
    TVT tv19 = {tv2}; // Note: used to test move construction below.
    test_text_view<TVT>(code_unit_maps_with_terminator,
                        RT{&cstr[0], &cstr[cstr_length]},
                        tv19);
                             
    // Test initialization via the move constructor.
    TVT tv20 = {move(tv19)};
    test_text_view<TVT>(code_unit_maps_with_terminator,
                        RT{&cstr[0], &cstr[cstr_length]},
                        tv20);

    // Test copy assignment.
    TVT tv21; // Note: used to test move assignment below.
    tv21 = {tv2};
    test_text_view<TVT>(code_unit_maps_with_terminator,
                        RT{&cstr[0], &cstr[cstr_length]},
                        tv21);

    // Test move assignment.
    TVT tv22;
    tv22 = {move(tv21)};
    test_text_view<TVT>(code_unit_maps_with_terminator,
                        RT{&cstr[0], &cstr[cstr_length]},
                        tv22);
}

template<
    Encoding ET,
    size_t cstr_length,
    size_t ary_length,
    typename String>
void test_make_text_view(
    const code_unit_map_sequence<typename ET::codec_type>
        &code_unit_maps_with_terminator,
    const code_unit_map_sequence<typename ET::codec_type>
        &code_unit_maps_without_terminator,
    const typename ET::codec_type::code_unit_type (&cstr)[cstr_length],
    const array<typename ET::codec_type::code_unit_type, ary_length> &ary,
    const String &str,
    const initializer_list<const typename ET::codec_type::code_unit_type> &il)
{
    // Test construction with an explicit initial state and an iterator range.
    test_text_view(code_unit_maps_without_terminator,
                   ary,
                   make_text_view<ET>(ET::initial_state(), begin(ary), end(ary)));

    // Test construction with an implicit initial state and an iterator range.
    test_text_view(code_unit_maps_without_terminator,
                   ary,
                   make_text_view<ET>(begin(ary), end(ary)));

    // Test construction with an explicit initial state and a sized iterator range.
    test_text_view(code_unit_maps_without_terminator,
                   ary,
                   make_text_view<ET>(ET::initial_state(), begin(ary), ary.size()));

    // Test construction with an implicit initial state and a sized iterator range.
    test_text_view(code_unit_maps_without_terminator,
                   ary,
                   make_text_view<ET>(begin(ary), ary.size()));

    // Test construction with an explicit initial state and an iterable (C array).
    test_text_view(code_unit_maps_with_terminator,
                   cstr,
                   make_text_view<ET>(ET::initial_state(), cstr));

    // Test construction with an implicit initial state and an iterable (C array).
    test_text_view(code_unit_maps_with_terminator,
                   cstr,
                   make_text_view<ET>(cstr));

    // Test construction with an explicit initial state and an iterable (std::array).
    test_text_view(code_unit_maps_without_terminator,
                   ary,
                   make_text_view<ET>(ET::initial_state(), ary));

    // Test construction with an implicit initial state and an iterable (std::array).
    test_text_view(code_unit_maps_without_terminator,
                   ary,
                   make_text_view<ET>(ary));

    // Test construction with an explicit initial state and an iterable (std::string).
    test_text_view(code_unit_maps_without_terminator,
                   str,
                   make_text_view<ET>(ET::initial_state(), str));

    // Test construction with an implicit initial state and an iterable (std::string).
    test_text_view(code_unit_maps_without_terminator,
                   str,
                   make_text_view<ET>(str));

    // Test construction with an explicit initial state and an iterable (std::initializer_list).
    test_text_view(code_unit_maps_without_terminator,
                   il,
                   make_text_view<ET>(ET::initial_state(), il));

    // Test construction with an implicit initial state and an iterable (std::initializer_list).
    test_text_view(code_unit_maps_without_terminator,
                   il,
                   make_text_view<ET>(il));

    // Test construction with an explicit initial state and an iterable (make_cstr_range).
    test_text_view(code_unit_maps_without_terminator,
                   make_cstr_range(cstr),
                   make_text_view<ET>(ET::initial_state(), make_cstr_range(cstr)));

    // Test construction with an implicit initial state and an iterable (make_cstr_range).
    test_text_view(code_unit_maps_without_terminator,
                   make_cstr_range(cstr),
                   make_text_view<ET>(make_cstr_range(cstr)));

    // Test construction via the copy constructor.  Note that an explicit
    // encoding is not specified in the make_text_view call.
    auto tv1 = make_text_view<ET>(ary);
    test_text_view(code_unit_maps_without_terminator,
                   ary,
                   make_text_view(tv1));

    // Test construction via the move constructor.  Note that an explicit
    // encoding is not specified in the make_text_view call.
    auto tv2 = tv1;
    test_text_view(code_unit_maps_without_terminator,
                   ary,
                   make_text_view(move(tv2)));

    // Test construction with a text iterator pair.  Note that an explicit
    // encoding is not specified in the make_text_view call.
    test_text_view(code_unit_maps_without_terminator,
                   ary,
                   make_text_view(begin(tv1), end(tv1)));
}

void test_text_view() {
    using TVT = text_view;
    using ET = encoding_type_of<TVT>;
    using CT = ET::codec_type::character_type;
    using CUMS = code_unit_map_sequence<ET::codec_type>;

    CUMS code_unit_maps_with_terminator{
        { {}, { CT{'t'} },  { 't'  } },
        { {}, { CT{'e'} },  { 'e'  } },
        { {}, { CT{'x'} },  { 'x'  } },
        { {}, { CT{'t'} },  { 't'  } },
        { {}, { CT{'\0'} }, { '\0' } } };
    CUMS code_unit_maps_without_terminator{
        { {}, { CT{'t'} },  { 't'  } },
        { {}, { CT{'e'} },  { 'e'  } },
        { {}, { CT{'x'} },  { 'x'  } },
        { {}, { CT{'t'} },  { 't'  } } };

    // Underlying code unit containers.
    static const char cstr[] = "text";
    static const array<char, 4> ary{ 't', 'e', 'x', 't' };
    static const string str{"text"};
    static const auto il = { 't', 'e', 'x', 't' }; // std::initializer_list<char>.

    test_construct_text_view<TVT>(
        code_unit_maps_with_terminator,
        code_unit_maps_without_terminator,
        cstr,
        ary,
        str,
        il);

    test_make_text_view<ET>(
        code_unit_maps_with_terminator,
        code_unit_maps_without_terminator,
        cstr,
        ary,
        str,
        il);
}

void test_wtext_view() {
    using TVT = wtext_view;
    using ET = encoding_type_of<TVT>;
    using CT = ET::codec_type::character_type;
    using CUMS = code_unit_map_sequence<ET::codec_type>;

    CUMS code_unit_maps_with_terminator{
        { {}, { CT{L't'} },  { L't'  } },
        { {}, { CT{L'e'} },  { L'e'  } },
        { {}, { CT{L'x'} },  { L'x'  } },
        { {}, { CT{L't'} },  { L't'  } },
        { {}, { CT{L'\0'} }, { L'\0' } } };
    CUMS code_unit_maps_without_terminator{
        { {}, { CT{L't'} },  { L't'  } },
        { {}, { CT{L'e'} },  { L'e'  } },
        { {}, { CT{L'x'} },  { L'x'  } },
        { {}, { CT{L't'} },  { L't'  } } };

    // Underlying code unit containers.
    static const wchar_t cstr[] = L"text";
    static const array<wchar_t, 4> ary{ L't', L'e', L'x', L't' };
    static const wstring str{L"text"};
    static const auto il = { L't', L'e', L'x', L't' }; // std::initializer_list<wchar_t>.

    test_construct_text_view<TVT>(
        code_unit_maps_with_terminator,
        code_unit_maps_without_terminator,
        cstr,
        ary,
        str,
        il);

    test_make_text_view<ET>(
        code_unit_maps_with_terminator,
        code_unit_maps_without_terminator,
        cstr,
        ary,
        str,
        il);
}

void test_u8text_view() {
    using TVT = u8text_view;
    using ET = encoding_type_of<TVT>;
    using CT = ET::codec_type::character_type;
    using CUMS = code_unit_map_sequence<ET::codec_type>;

    // FIXME: Once N4267 is implemented, replace these character literals with
    // FIXME: u8 prefixed ones.
    CUMS code_unit_maps_with_terminator{
        { {}, { CT{U't'} },  { 't'  } },
        { {}, { CT{U'e'} },  { 'e'  } },
        { {}, { CT{U'x'} },  { 'x'  } },
        { {}, { CT{U't'} },  { 't'  } },
        { {}, { CT{U'\0'} }, { '\0' } } };
    CUMS code_unit_maps_without_terminator{
        { {}, { CT{U't'} },  { 't'  } },
        { {}, { CT{U'e'} },  { 'e'  } },
        { {}, { CT{U'x'} },  { 'x'  } },
        { {}, { CT{U't'} },  { 't'  } } };

    // Underlying code unit containers.
    // FIXME: If N3398 were to be adopted, replace char with char8_t.
    static const char cstr[] = u8"text";
    // FIXME: If N3398 were to be adopted, replace char with char8_t.
    // FIXME: Once N4267 is implemented, replace these character literals with
    // FIXME: u8 prefixed ones.
    static const array<char, 4> ary{ 't', 'e', 'x', 't' };
    static const string str{u8"text"};
    // FIXME: If N3398 were to be adopted, replace char with char8_t.
    // FIXME: Once N4267 is implemented, replace these character literals with
    // FIXME: u8 prefixed ones.
    static const auto il = { 't', 'e', 'x', 't' }; // std::initializer_list<char>.

    test_construct_text_view<TVT>(
        code_unit_maps_with_terminator,
        code_unit_maps_without_terminator,
        cstr,
        ary,
        str,
        il);

    test_make_text_view<ET>(
        code_unit_maps_with_terminator,
        code_unit_maps_without_terminator,
        cstr,
        ary,
        str,
        il);
}

void test_u16text_view() {
    using TVT = u16text_view;
    using ET = encoding_type_of<TVT>;
    using CT = ET::codec_type::character_type;
    using CUMS = code_unit_map_sequence<ET::codec_type>;

    CUMS code_unit_maps_with_terminator{
        { {}, { CT{U't'} },  { u't'  } },
        { {}, { CT{U'e'} },  { u'e'  } },
        { {}, { CT{U'x'} },  { u'x'  } },
        { {}, { CT{U't'} },  { u't'  } },
        { {}, { CT{U'\0'} }, { u'\0' } } };
    CUMS code_unit_maps_without_terminator{
        { {}, { CT{U't'} },  { u't'  } },
        { {}, { CT{U'e'} },  { u'e'  } },
        { {}, { CT{U'x'} },  { u'x'  } },
        { {}, { CT{U't'} },  { u't'  } } };

    // Underlying code unit containers.
    static const char16_t cstr[] = u"text";
    static const array<char16_t, 4> ary{ u't', u'e', u'x', u't' };
    static const u16string str{u"text"};
    static const auto il = { u't', u'e', u'x', u't' }; // std::initializer_list<char16_t>.

    test_construct_text_view<TVT>(
        code_unit_maps_with_terminator,
        code_unit_maps_without_terminator,
        cstr,
        ary,
        str,
        il);

    test_make_text_view<ET>(
        code_unit_maps_with_terminator,
        code_unit_maps_without_terminator,
        cstr,
        ary,
        str,
        il);
}

void test_u32text_view() {
    using TVT = u32text_view;
    using ET = encoding_type_of<TVT>;
    using CT = ET::codec_type::character_type;
    using CUMS = code_unit_map_sequence<ET::codec_type>;

    CUMS code_unit_maps_with_terminator{
        { {}, { CT{U't'} },  { U't'  } },
        { {}, { CT{U'e'} },  { U'e'  } },
        { {}, { CT{U'x'} },  { U'x'  } },
        { {}, { CT{U't'} },  { U't'  } },
        { {}, { CT{U'\0'} }, { U'\0' } } };
    CUMS code_unit_maps_without_terminator{
        { {}, { CT{U't'} },  { U't'  } },
        { {}, { CT{U'e'} },  { U'e'  } },
        { {}, { CT{U'x'} },  { U'x'  } },
        { {}, { CT{U't'} },  { U't'  } } };

    // Underlying code unit containers.
    static const char32_t cstr[] = U"text";
    static const array<char32_t, 4> ary{ U't', U'e', U'x', U't' };
    static const u32string str{U"text"};
    static const auto il = { U't', U'e', U'x', U't' }; // std::initializer_list<char32_t>.

    test_construct_text_view<TVT>(
        code_unit_maps_with_terminator,
        code_unit_maps_without_terminator,
        cstr,
        ary,
        str,
        il);

    test_make_text_view<ET>(
        code_unit_maps_with_terminator,
        code_unit_maps_without_terminator,
        cstr,
        ary,
        str,
        il);
}

void test_utf8_encoding() {
    using ET = utf8_encoding;
    using CT = ET::codec_type::character_type;
    using CUT = ET::codec_type::code_unit_type;
    using CUMS = code_unit_map_sequence<ET::codec_type>;

    // FIXME: code_unit_type for UTF-8 is char, but the values below require
    // FIXME: an unsigned (8-bit) char.  An initializer that allows narrowing
    // FIXME: conversions is used to support implementations with a signed
    // FIXME: 8-bit char.

    // Test an empty code unit sequence.
    CUMS code_unit_maps_empty{};
    test_bidirectional_encoding<ET>(code_unit_maps_empty);

    // Test a non-empty code unit sequence.
    CUMS code_unit_maps{
        { {}, { CT{U'\U00000041'} }, { CUT(0x41) } },
        { {}, { CT{U'\U00000141'} }, { CUT(0xC5), CUT(0x81) } },
        { {}, { CT{U'\U00001141'} }, { CUT(0xE1), CUT(0x85), CUT(0x81) } },
        { {}, { CT{U'\U00011141'} }, { CUT(0xF0), CUT(0x91), CUT(0x85), CUT(0x81) } },
        { {}, { CT{U'\0'} },         { CUT(0x00) } } };
    test_bidirectional_encoding<ET>(code_unit_maps);

    string encoded_string(u8"a\U00011141z");
    auto tv = make_text_view<ET>(encoded_string);
    static_assert(origin::Iterator<decltype(end(tv))>(), "");
    auto tvit = find(begin(tv), end(tv), CT{U'\U00011141'});
    assert(begin(tvit.base_range()) == begin(encoded_string) + 1);
    assert(end(tvit.base_range()) == begin(encoded_string) + 5);
}

void test_utf8bom_encoding() {
    using ET = utf8bom_encoding;
    using CT = ET::codec_type::character_type;
    using CUT = ET::codec_type::code_unit_type;
    using ST = ET::codec_type::state_type;
    using CUMS = code_unit_map_sequence<ET::codec_type>;

    // FIXME: code_unit_type for UTF-8 is char, but the values below require
    // FIXME: an unsigned (8-bit) char.  An initializer that allows narrowing
    // FIXME: conversions is used to support implementations with a signed
    // FIXME: 8-bit char.

    // Test an empty code unit sequence.
    CUMS code_unit_maps_empty{};
    test_bidirectional_encoding<ET>(code_unit_maps_empty);

    // Test a code unit sequence containing only a BOM.  This requires an
    // explicit state transition to force writing of the BOM when encoding.
    CUMS code_unit_maps_only_bom{
        { { ST::to_bom_written_state() },
              {}, { CUT(0xEF), CUT(0xBB), CUT(0xBF) } } };
    test_bidirectional_encoding<ET>(code_unit_maps_only_bom);

    // Test a code unit sequence not containing a BOM.
    CUMS code_unit_maps_no_bom{
        { { ST::to_assume_bom_written_state() },
              {},                    {} },
        { {}, { CT{U'\U00000041'} }, { CUT(0x41) } },
        { {}, { CT{U'\U00000141'} }, { CUT(0xC5), CUT(0x81) } },
        { {}, { CT{U'\U0000FEFF'} }, { CUT(0xEF), CUT(0xBB), CUT(0xBF) } }, // Not a BOM
        { {}, { CT{U'\U00011141'} }, { CUT(0xF0), CUT(0x91), CUT(0x85), CUT(0x81) } },
        { {}, { CT{U'\0'} },         { CUT(0x00) } } };
    test_bidirectional_encoding<ET>(code_unit_maps_no_bom);

    // Test a code unit sequence containing a BOM.
    CUMS code_unit_maps_bom{
        { {}, {},                    { CUT(0xEF), CUT(0xBB), CUT(0xBF) } }, // BOM
        { {}, { CT{U'\U00000041'} }, { CUT(0x41) } },
        { {}, { CT{U'\U00000141'} }, { CUT(0xC5), CUT(0x81) } },
        { {}, { CT{U'\U0000FEFF'} }, { CUT(0xEF), CUT(0xBB), CUT(0xBF) } }, // Not a BOM
        { {}, { CT{U'\U00011141'} }, { CUT(0xF0), CUT(0x91), CUT(0x85), CUT(0x81) } },
        { {}, { CT{U'\0'} },         { CUT(0x00) } } };
    test_bidirectional_encoding<ET>(code_unit_maps_bom);

    {
    string encoded_string_no_bom(u8"a\U00011141\U0000FEFF");
    auto tv = make_text_view<ET>(encoded_string_no_bom);
    static_assert(! origin::Iterator<decltype(end(tv))>(), "");
    auto tvend = text_detail::advance_to(begin(tv), end(tv));
    auto tvit = find(begin(tv), tvend, CT{U'a'});
    assert(begin(tvit.base_range()) == begin(encoded_string_no_bom) + 0);
    assert(end(tvit.base_range()) == begin(encoded_string_no_bom) + 1);
    tvit = find(begin(tv), tvend, CT{U'\U00011141'});
    assert(begin(tvit.base_range()) == begin(encoded_string_no_bom) + 1);
    assert(end(tvit.base_range()) == begin(encoded_string_no_bom) + 5);
    tvit = find(begin(tv), tvend, CT{U'\U0000FEFF'});
    assert(begin(tvit.base_range()) == begin(encoded_string_no_bom) + 5);
    assert(end(tvit.base_range()) == begin(encoded_string_no_bom) + 8);
    }

    {
    string encoded_string_bom(u8"\U0000FEFFa\U00011141\U0000FEFF");
    auto tv = make_text_view<ET>(encoded_string_bom);
    static_assert(! origin::Iterator<decltype(end(tv))>(), "");
    auto tvend = text_detail::advance_to(begin(tv), end(tv));
    auto tvit = find(begin(tv), tvend, CT{U'a'});
    assert(begin(tvit.base_range()) == begin(encoded_string_bom) + 3);
    assert(end(tvit.base_range()) == begin(encoded_string_bom) + 4);
    tvit = find(begin(tv), tvend, CT{U'\U00011141'});
    assert(begin(tvit.base_range()) == begin(encoded_string_bom) + 4);
    assert(end(tvit.base_range()) == begin(encoded_string_bom) + 8);
    tvit = find(begin(tv), tvend, CT{U'\U0000FEFF'});
    assert(begin(tvit.base_range()) == begin(encoded_string_bom) + 8);
    assert(end(tvit.base_range()) == begin(encoded_string_bom) + 11);
    }
}

void test_utf16_encoding() {
    using ET = utf16_encoding;
    using CT = ET::codec_type::character_type;
    using CUMS = code_unit_map_sequence<ET::codec_type>;

    // Test an empty code unit sequence.
    CUMS code_unit_maps_empty{};
    test_bidirectional_encoding<ET>(code_unit_maps_empty);

    // Test a non-empty code unit sequence.
    CUMS code_unit_maps{
        { {}, { CT{U'\U00000041'} }, { 0x0041 } },
        { {}, { CT{U'\U00000141'} }, { 0x0141 } },
        { {}, { CT{U'\U00001141'} }, { 0x1141 } },
        { {}, { CT{U'\U00011141'} }, { 0xD804, 0xDD41 } },
        { {}, { CT{U'\0'} },         { 0x0000 } } };
    test_bidirectional_encoding<ET>(code_unit_maps);

    u16string encoded_string(u"a\U00011141z");
    auto tv = make_text_view<ET>(encoded_string);
    static_assert(origin::Iterator<decltype(end(tv))>(), "");
    auto tvit = find(begin(tv), end(tv), CT{U'\U00011141'});
    assert(begin(tvit.base_range()) == begin(encoded_string) + 1);
    assert(end(tvit.base_range()) == begin(encoded_string) + 3);
}

void test_utf16be_encoding() {
    using ET = utf16be_encoding;
    using CT = ET::codec_type::character_type;
    using CUMS = code_unit_map_sequence<ET::codec_type>;

    // Test an empty code unit sequence.
    CUMS code_unit_maps_empty{};
    test_bidirectional_encoding<ET>(code_unit_maps_empty);

    // Test a non-empty code unit sequence.
    CUMS code_unit_maps{
        { {}, { CT{U'\U00000041'} }, { 0x00, 0x41 } },
        { {}, { CT{U'\U00000141'} }, { 0x01, 0x41 } },
        { {}, { CT{U'\U00001141'} }, { 0x11, 0x41 } },
        { {}, { CT{U'\U00011141'} }, { 0xD8, 0x04, 0xDD, 0x41 } },
        { {}, { CT{U'\0'} },         { 0x00, 0x00 } } };
    test_bidirectional_encoding<ET>(code_unit_maps);

    string encoded_string("\x00\x61\xD8\x04\xDD\x41\x00\x7A", 8);
    auto tv = make_text_view<ET>(encoded_string);
    static_assert(origin::Iterator<decltype(end(tv))>(), "");
    auto tvit = find(begin(tv), end(tv), CT{U'\U00011141'});
    assert(begin(tvit.base_range()) == begin(encoded_string) + 2);
    assert(end(tvit.base_range()) == begin(encoded_string) + 6);
}

void test_utf16le_encoding() {
    using ET = utf16le_encoding;
    using CT = ET::codec_type::character_type;
    using CUMS = code_unit_map_sequence<ET::codec_type>;

    // Test an empty code unit sequence.
    CUMS code_unit_maps_empty{};
    test_bidirectional_encoding<ET>(code_unit_maps_empty);

    // Test a non-empty code unit sequence.
    CUMS code_unit_maps{
        { {}, { CT{U'\U00000041'} }, { 0x41, 0x00 } },
        { {}, { CT{U'\U00000141'} }, { 0x41, 0x01 } },
        { {}, { CT{U'\U00001141'} }, { 0x41, 0x11 } },
        { {}, { CT{U'\U00011141'} }, { 0x04, 0xD8, 0x41, 0xDD } },
        { {}, { CT{U'\0'} },         { 0x00, 0x00 } } };
    test_bidirectional_encoding<ET>(code_unit_maps);

    string encoded_string("\x61\x00\x04\xD8\x41\xDD\x7A\x00", 8);
    auto tv = make_text_view<ET>(encoded_string);
    static_assert(origin::Iterator<decltype(end(tv))>(), "");
    auto tvit = find(begin(tv), end(tv), CT{U'\U00011141'});
    assert(begin(tvit.base_range()) == begin(encoded_string) + 2);
    assert(end(tvit.base_range()) == begin(encoded_string) + 6);
}

void test_utf16bom_encoding() {
    using ET = utf16bom_encoding;
    using CT = ET::codec_type::character_type;
    using ST = ET::codec_type::state_type;
    using CUMS = code_unit_map_sequence<ET::codec_type>;

    // Test an empty code unit sequence.
    CUMS code_unit_maps_empty{};
    test_bidirectional_encoding<ET>(code_unit_maps_empty);

    // Test a code unit sequence containing only a BE BOM.  This requires an
    // explicit state transition to force writing of the BOM when encoding.
    CUMS code_unit_maps_only_be_bom{
        { { ST::to_bom_written_state() },
              {}, { 0xFE, 0xFF } } }; // BE BOM
    test_bidirectional_encoding<ET>(code_unit_maps_only_be_bom);

    // Test a code unit sequence containing only a LE BOM.  This requires an
    // explicit state transition to force writing of the BOM when encoding.
    CUMS code_unit_maps_only_le_bom{
        { { ST::to_le_bom_written_state() },
              {}, { 0xFF, 0xFE } } }; // LE BOM
    test_bidirectional_encoding<ET>(code_unit_maps_only_le_bom);

    // Test a code unit sequence not containing a BOM.  Big endian is assumed.
    CUMS code_unit_maps_no_bom{
        { { ST::to_assume_bom_written_state() },
              {},                    {} },
        { {}, { CT{U'\U00000041'} }, { 0x00, 0x41 } },
        { {}, { CT{U'\U00000141'} }, { 0x01, 0x41 } },
        { {}, { CT{U'\U0000FEFF'} }, { 0xFE, 0xFF } }, // Not a BOM
        { {}, { CT{U'\U00011141'} }, { 0xD8, 0x04, 0xDD, 0x41 } },
        { {}, { CT{U'\0'} },         { 0x00, 0x00 } } };
    test_bidirectional_encoding<ET>(code_unit_maps_no_bom);

    // Test a code unit sequence containing a BE BOM.
    CUMS code_unit_maps_be_bom{
        { {}, {},                    { 0xFE, 0xFF } }, // BE BOM
        { {}, { CT{U'\U00000041'} }, { 0x00, 0x41 } },
        { {}, { CT{U'\U00000141'} }, { 0x01, 0x41 } },
        { {}, { CT{U'\U0000FEFF'} }, { 0xFE, 0xFF } }, // Not a BOM
        { {}, { CT{U'\U00011141'} }, { 0xD8, 0x04, 0xDD, 0x41 } },
        { {}, { CT{U'\0'} },         { 0x00, 0x00 } } };
    test_bidirectional_encoding<ET>(code_unit_maps_be_bom);

    // Test a code unit sequence containing a LE BOM.  This requires an explicit
    // state transition to force writing of a LE BOM.
    CUMS code_unit_maps_le_bom{
        { { ST::to_le_bom_written_state() },
              {},                    { 0xFF, 0xFE } }, // LE BOM
        { {}, { CT{U'\U00000041'} }, { 0x41, 0x00 } },
        { {}, { CT{U'\U00000141'} }, { 0x41, 0x01 } },
        { {}, { CT{U'\U0000FEFF'} }, { 0xFF, 0xFE } }, // Not a BOM
        { {}, { CT{U'\U00011141'} }, { 0x04, 0xD8, 0x41, 0xDD } },
        { {}, { CT{U'\0'} },         { 0x00, 0x00 } } };
    test_bidirectional_encoding<ET>(code_unit_maps_le_bom);

    {
    string encoded_string_no_bom("\x00\x61\xD8\x04\xDD\x41\xFE\xFF", 8);
    auto tv = make_text_view<ET>(encoded_string_no_bom);
    static_assert(! origin::Iterator<decltype(end(tv))>(), "");
    auto tvend = text_detail::advance_to(begin(tv), end(tv));
    auto tvit = find(begin(tv), tvend, CT{U'\U00000061'});
    assert(begin(tvit.base_range()) == begin(encoded_string_no_bom) + 0);
    assert(end(tvit.base_range()) == begin(encoded_string_no_bom) + 2);
    tvit = find(begin(tv), tvend, CT{U'\U00011141'});
    assert(begin(tvit.base_range()) == begin(encoded_string_no_bom) + 2);
    assert(end(tvit.base_range()) == begin(encoded_string_no_bom) + 6);
    tvit = find(begin(tv), tvend, CT{U'\U0000FEFF'});
    assert(begin(tvit.base_range()) == begin(encoded_string_no_bom) + 6);
    assert(end(tvit.base_range()) == begin(encoded_string_no_bom) + 8);
    }

    {
    string encoded_string_be_bom("\xFE\xFF\x00\x61\xD8\x04\xDD\x41\xFE\xFF", 10);
    auto tv = make_text_view<ET>(encoded_string_be_bom);
    static_assert(! origin::Iterator<decltype(end(tv))>(), "");
    auto tvend = text_detail::advance_to(begin(tv), end(tv));
    auto tvit = find(begin(tv), tvend, CT{U'\U00000061'});
    assert(begin(tvit.base_range()) == begin(encoded_string_be_bom) + 2);
    assert(end(tvit.base_range()) == begin(encoded_string_be_bom) + 4);
    tvit = find(begin(tv), tvend, CT{U'\U00011141'});
    assert(begin(tvit.base_range()) == begin(encoded_string_be_bom) + 4);
    assert(end(tvit.base_range()) == begin(encoded_string_be_bom) + 8);
    tvit = find(begin(tv), tvend, CT{U'\U0000FEFF'});
    assert(begin(tvit.base_range()) == begin(encoded_string_be_bom) + 8);
    assert(end(tvit.base_range()) == begin(encoded_string_be_bom) + 10);
    }

    {
    string encoded_string_le_bom("\xFF\xFE\x61\x00\x04\xD8\x41\xDD\xFF\xFE", 10);
    auto tv = make_text_view<ET>(encoded_string_le_bom);
    static_assert(! origin::Iterator<decltype(end(tv))>(), "");
    auto tvend = text_detail::advance_to(begin(tv), end(tv));
    auto tvit = find(begin(tv), tvend, CT{U'\U00000061'});
    assert(begin(tvit.base_range()) == begin(encoded_string_le_bom) + 2);
    assert(end(tvit.base_range()) == begin(encoded_string_le_bom) + 4);
    tvit = find(begin(tv), tvend, CT{U'\U00011141'});
    assert(begin(tvit.base_range()) == begin(encoded_string_le_bom) + 4);
    assert(end(tvit.base_range()) == begin(encoded_string_le_bom) + 8);
    tvit = find(begin(tv), tvend, CT{U'\U0000FEFF'});
    assert(begin(tvit.base_range()) == begin(encoded_string_le_bom) + 8);
    assert(end(tvit.base_range()) == begin(encoded_string_le_bom) + 10);
    }
}

void test_utf32_encoding() {
    using ET = utf32_encoding;
    using CT = ET::codec_type::character_type;
    using CUMS = code_unit_map_sequence<ET::codec_type>;

    // Test an empty code unit sequence.
    CUMS code_unit_maps_empty{};
    test_random_access_encoding<ET>(code_unit_maps_empty);

    // Test a non-empty code unit sequence.
    CUMS code_unit_maps{
        { {}, { CT{U'\U00000041'} }, { 0x00000041 } },
        { {}, { CT{U'\U00000141'} }, { 0x00000141 } },
        { {}, { CT{U'\U00001141'} }, { 0x00001141 } },
        { {}, { CT{U'\U00011141'} }, { 0x00011141 } },
        { {}, { CT{U'\0'} },         { 0x00000000 } } };
    test_random_access_encoding<ET>(code_unit_maps);

    u32string encoded_string(U"a\U00011141z");
    auto tv = make_text_view<ET>(encoded_string);
    static_assert(origin::Iterator<decltype(end(tv))>(), "");
    auto tvit = find(begin(tv), end(tv), CT{U'\U00011141'});
    assert(begin(tvit.base_range()) == begin(encoded_string) + 1);
    assert(end(tvit.base_range()) == begin(encoded_string) + 2);
}

void test_utf32be_encoding() {
    using ET = utf32be_encoding;
    using CT = ET::codec_type::character_type;
    using CUMS = code_unit_map_sequence<ET::codec_type>;

    // Test an empty code unit sequence.
    CUMS code_unit_maps_empty{};
    test_random_access_encoding<ET>(code_unit_maps_empty);

    // Test a non-empty code unit sequence.
    CUMS code_unit_maps{
        { {}, { CT{U'\U00000041'} }, { 0x00, 0x00, 0x00, 0x41 } },
        { {}, { CT{U'\U00000141'} }, { 0x00, 0x00, 0x01, 0x41 } },
        { {}, { CT{U'\U00001141'} }, { 0x00, 0x00, 0x11, 0x41 } },
        { {}, { CT{U'\U00011141'} }, { 0x00, 0x01, 0x11, 0x41 } },
        { {}, { CT{U'\0'} },         { 0x00, 0x00, 0x00, 0x00 } } };
    test_random_access_encoding<ET>(code_unit_maps);

    string encoded_string("\x00\x00\x00\x61\x00\x01\x11\x41\x00\x00\x00\x7A", 12);
    auto tv = make_text_view<ET>(encoded_string);
    static_assert(origin::Iterator<decltype(end(tv))>(), "");
    auto tvit = find(begin(tv), end(tv), CT{U'\U00011141'});
    assert(begin(tvit.base_range()) == begin(encoded_string) + 4);
    assert(end(tvit.base_range()) == begin(encoded_string) + 8);
}

void test_utf32le_encoding() {
    using ET = utf32le_encoding;
    using CT = ET::codec_type::character_type;
    using CUMS = code_unit_map_sequence<ET::codec_type>;

    // Test an empty code unit sequence.
    CUMS code_unit_maps_empty{};
    test_random_access_encoding<ET>(code_unit_maps_empty);

    // Test a non-empty code unit sequence.
    CUMS code_unit_maps{
        { {}, { CT{U'\U00000041'} }, { 0x41, 0x00, 0x00, 0x00 } },
        { {}, { CT{U'\U00000141'} }, { 0x41, 0x01, 0x00, 0x00 } },
        { {}, { CT{U'\U00001141'} }, { 0x41, 0x11, 0x00, 0x00 } },
        { {}, { CT{U'\U00011141'} }, { 0x41, 0x11, 0x01, 0x00 } },
        { {}, { CT{U'\0'} },         { 0x00, 0x00, 0x00, 0x00 } } };
    test_random_access_encoding<ET>(code_unit_maps);

    string encoded_string("\x61\x00\x00\x00\x41\x11\x01\x00\x7A\x00\x00\x00", 12);
    auto tv = make_text_view<ET>(encoded_string);
    static_assert(origin::Iterator<decltype(end(tv))>(), "");
    auto tvit = find(begin(tv), end(tv), CT{U'\U00011141'});
    assert(begin(tvit.base_range()) == begin(encoded_string) + 4);
    assert(end(tvit.base_range()) == begin(encoded_string) + 8);
}

void test_utf32bom_encoding() {
    using ET = utf32bom_encoding;
    using CT = ET::codec_type::character_type;
    using ST = ET::codec_type::state_type;
    using CUMS = code_unit_map_sequence<ET::codec_type>;

    // Test an empty code unit sequence.
    CUMS code_unit_maps_empty{};
    test_bidirectional_encoding<ET>(code_unit_maps_empty);

    // Test a code unit sequence containing only a BE BOM.  This requires an
    // explicit state transition to force writing of the BOM when encoding.
    CUMS code_unit_maps_only_be_bom{
        { { ST::to_bom_written_state() },
              {}, { 0x00, 0x00, 0xFE, 0xFF } } }; // BE BOM
    test_bidirectional_encoding<ET>(code_unit_maps_only_be_bom);

    // Test a code unit sequence containing only a LE BOM.  This requires an
    // explicit state transition to force writing of the BOM when encoding.
    CUMS code_unit_maps_only_le_bom{
        { { ST::to_le_bom_written_state() },
              {}, { 0xFF, 0xFE, 0x00, 0x00 } } }; // LE BOM
    test_bidirectional_encoding<ET>(code_unit_maps_only_le_bom);

    // Test a code unit sequence not containing a BOM.  Big endian is assumed.
    CUMS code_unit_maps_no_bom{
        { { ST::to_assume_bom_written_state() },
              {},                    {} },
        { {}, { CT{U'\U00000041'} }, { 0x00, 0x00, 0x00, 0x41 } },
        { {}, { CT{U'\U00000141'} }, { 0x00, 0x00, 0x01, 0x41 } },
        { {}, { CT{U'\U0000FEFF'} }, { 0x00, 0x00, 0xFE, 0xFF } }, // Not a BOM
        { {}, { CT{U'\U00011141'} }, { 0x00, 0x01, 0x11, 0x41 } },
        { {}, { CT{U'\0'} },         { 0x00, 0x00, 0x00, 0x00 } } };
    test_bidirectional_encoding<ET>(code_unit_maps_no_bom);

    // Test a code unit sequence containing a BE BOM.
    CUMS code_unit_maps_be_bom{
        { {}, {},                    { 0x00, 0x00, 0xFE, 0xFF } }, // BE BOM
        { {}, { CT{U'\U00000041'} }, { 0x00, 0x00, 0x00, 0x41 } },
        { {}, { CT{U'\U00000141'} }, { 0x00, 0x00, 0x01, 0x41 } },
        { {}, { CT{U'\U0000FEFF'} }, { 0x00, 0x00, 0xFE, 0xFF } }, // Not a BOM
        { {}, { CT{U'\U00011141'} }, { 0x00, 0x01, 0x11, 0x41 } },
        { {}, { CT{U'\0'} },         { 0x00, 0x00, 0x00, 0x00 } } };
    test_bidirectional_encoding<ET>(code_unit_maps_be_bom);

    // Test a code unit sequence containing a LE BOM.  This requires an explicit
    // state transition to force writing of a LE BOM.
    CUMS code_unit_maps_le_bom{
        { { ST::to_le_bom_written_state() },
              {},                    { 0xFF, 0xFE, 0x00, 0x00 } }, // LE BOM
        { {}, { CT{U'\U00000041'} }, { 0x41, 0x00, 0x00, 0x00 } },
        { {}, { CT{U'\U00000141'} }, { 0x41, 0x01, 0x00, 0x00 } },
        { {}, { CT{U'\U0000FEFF'} }, { 0xFF, 0xFE, 0x00, 0x00 } }, // Not a BOM
        { {}, { CT{U'\U00011141'} }, { 0x41, 0x11, 0x01, 0x00 } },
        { {}, { CT{U'\0'} },         { 0x00, 0x00, 0x00, 0x00 } } };
    test_bidirectional_encoding<ET>(code_unit_maps_le_bom);

    {
    string encoded_string_no_bom("\x00\x00\x00\x61\x00\x01\x11\x41\x00\x00\xFE\xFF", 12);
    auto tv = make_text_view<ET>(encoded_string_no_bom);
    static_assert(! origin::Iterator<decltype(end(tv))>(), "");
    auto tvend = text_detail::advance_to(begin(tv), end(tv));
    auto tvit = find(begin(tv), tvend, CT{U'\U00000061'});
    assert(begin(tvit.base_range()) == begin(encoded_string_no_bom) + 0);
    assert(end(tvit.base_range()) == begin(encoded_string_no_bom) + 4);
    tvit = find(begin(tv), tvend, CT{U'\U00011141'});
    assert(begin(tvit.base_range()) == begin(encoded_string_no_bom) + 4);
    assert(end(tvit.base_range()) == begin(encoded_string_no_bom) + 8);
    tvit = find(begin(tv), tvend, CT{U'\U0000FEFF'});
    assert(begin(tvit.base_range()) == begin(encoded_string_no_bom) + 8);
    assert(end(tvit.base_range()) == begin(encoded_string_no_bom) + 12);
    }

    {
    string encoded_string_be_bom("\x00\x00\xFE\xFF\x00\x00\x00\x61\x00\x01\x11\x41\x00\x00\xFE\xFF", 16);
    auto tv = make_text_view<ET>(encoded_string_be_bom);
    static_assert(! origin::Iterator<decltype(end(tv))>(), "");
    auto tvend = text_detail::advance_to(begin(tv), end(tv));
    auto tvit = find(begin(tv), tvend, CT{U'\U00000061'});
    assert(begin(tvit.base_range()) == begin(encoded_string_be_bom) + 4);
    assert(end(tvit.base_range()) == begin(encoded_string_be_bom) + 8);
    tvit = find(begin(tv), tvend, CT{U'\U00011141'});
    assert(begin(tvit.base_range()) == begin(encoded_string_be_bom) + 8);
    assert(end(tvit.base_range()) == begin(encoded_string_be_bom) + 12);
    tvit = find(begin(tv), tvend, CT{U'\U0000FEFF'});
    assert(begin(tvit.base_range()) == begin(encoded_string_be_bom) + 12);
    assert(end(tvit.base_range()) == begin(encoded_string_be_bom) + 16);
    }

    {
    string encoded_string_le_bom("\xFF\xFE\x00\x00\x61\x00\x00\x00\x41\x11\x01\x00\xFF\xFE\x00\x00", 16);
    auto tv = make_text_view<ET>(encoded_string_le_bom);
    static_assert(! origin::Iterator<decltype(end(tv))>(), "");
    auto tvend = text_detail::advance_to(begin(tv), end(tv));
    auto tvit = find(begin(tv), tvend, CT{U'\U00000061'});
    assert(begin(tvit.base_range()) == begin(encoded_string_le_bom) + 4);
    assert(end(tvit.base_range()) == begin(encoded_string_le_bom) + 8);
    tvit = find(begin(tv), tvend, CT{U'\U00011141'});
    assert(begin(tvit.base_range()) == begin(encoded_string_le_bom) + 8);
    assert(end(tvit.base_range()) == begin(encoded_string_le_bom) + 12);
    tvit = find(begin(tv), tvend, CT{U'\U0000FEFF'});
    assert(begin(tvit.base_range()) == begin(encoded_string_le_bom) + 12);
    assert(end(tvit.base_range()) == begin(encoded_string_le_bom) + 16);
    }
}

int main() {
    test_code_unit_models();
    test_code_point_models();
    test_character_set_models();
    test_character_models();
    test_code_unit_iterator_models();
    test_codec_state_models();
    test_codec_models();
    test_encoding_models();
    test_text_iterator_models();
    test_text_view_models();

    test_any_character_set();

    test_text_view();
    test_wtext_view();
    test_u8text_view();
    test_u16text_view();
    test_u32text_view();

    test_utf8_encoding();
    test_utf8bom_encoding();
    test_utf16_encoding();
    test_utf16be_encoding();
    test_utf16le_encoding();
    test_utf16bom_encoding();
    test_utf32_encoding();
    test_utf32be_encoding();
    test_utf32le_encoding();
    test_utf32bom_encoding();

    return 0;
}
