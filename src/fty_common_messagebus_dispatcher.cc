/*  =========================================================================
    fty_common_messagebus_dispatcher - class description

    Copyright (C) 2014 - 2020 Eaton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
*/

/*
@header
    fty_common_messagebus_dispatcher -
@discuss
@end
*/

#include "fty_common_messagebus_classes.h"

//  --------------------------------------------------------------------------
//  Self test of this class

// If your selftest reads SCMed fixture data, please keep it in
// src/selftest-ro; if your test creates filesystem objects, please
// do so under src/selftest-rw.
// The following pattern is suggested for C selftest code:
//    char *filename = NULL;
//    filename = zsys_sprintf ("%s/%s", SELFTEST_DIR_RO, "mytemplate.file");
//    assert (filename);
//    ... use the "filename" for I/O ...
//    zstr_free (&filename);
// This way the same "filename" variable can be reused for many subtests.
#define SELFTEST_DIR_RO "src/selftest-ro"
#define SELFTEST_DIR_RW "src/selftest-rw"

#include <cassert>
#include <iostream>
#include <set>

void fty_common_messagebus_dispatcher_test(bool verbose)
{
    std::cerr << " * fty_common_messagebus_dispatcher: " << std::endl;

    using namespace messagebus;
    {
        // Four-function calculator test.
        std::cerr << "  - calculator: ";

        using CalculatorDispatcher = Dispatcher<std::string, std::function<int(int, int)>, std::function<int(const std::string&, int, int)>>;
        CalculatorDispatcher::Map calculatorMap {
            { "+", [](int a, int b) -> int { return a + b; }},
            { "-", [](int a, int b) -> int { return a - b; }},
            { "*", [](int a, int b) -> int { return a * b; }},
            { "/", [](int a, int b) -> int { return a / b; }},
        } ;

        CalculatorDispatcher calculator(calculatorMap);

        for (int b = 1; b < 10; b++) {
            for (int a = 1; a < 10; a++) {
                assert(calculator("+", a, b) == (a+b));
                assert(calculator("-", a, b) == (a-b));
                assert(calculator("*", a, b) == (a*b));
                assert(calculator("/", a, b) == (a/b));
            }
        }

        // Check what happens on unknown operator.
        bool caught = false;
        try {
            assert(calculator("A", 2, 3) == 'A');
        }
        catch (std::bad_function_call &e) {
            caught = true;
        }
        assert(caught);

        std::cerr << "OK" << std::endl;
    }

    {
        // Translator test.
        std::cerr << "  - translator: ";

        using TranslatorDispatcher = Dispatcher<std::string, std::function<std::string()>, std::function<std::string(const std::string&)>>;
        TranslatorDispatcher::Map translatorMap {
            { "hello", []() -> std::string { return "bonjour"; }},
            { "goodbye", []() -> std::string { return "au revoir"; }},
        } ;

        TranslatorDispatcher translator(translatorMap,
            [](const std::string& word) { return "unknown word " + word; }
        );

        assert(translator("hello") == "bonjour");
        assert(translator("goodbye") == "au revoir");
        assert(translator("candy") == "unknown word candy");

        std::cerr << "OK" << std::endl;
    }
}
