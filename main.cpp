/*
http://www.github.com/marcelboldt/EXASockets

 The MIT License (MIT)

Copyright (C) 2016 Marcel Boldt / EXASOL

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Marcel Boldt <marcel.boldt@exasol.com>

*/


#include <iostream>
#include "exasockets/exasockets.h"

int main() {

    exasockets_connection *exaws;
    try {
        exaws = new exasockets_connection("192.168.137.10", 8563, "MBO", "sys", "exasol", 6, false);
    } catch (const char *msg) {
        std::cerr << msg << std::endl;
        exit(-1);
    }

    std::cout << exaws->session_id() << std::endl;

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    int h = exaws->exec_sql("select * from pub1092.flights limit 100000");
    if (h > 0) { // a result set handle received (result set >= 1000 rows))
        std::cout << "rows fetched: " << exaws->fetch(h, 0, (10485760 * 30)) << std::endl;
    }
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    std::cout << "Duration " << " : "
              << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000
              << " ms." << std::endl;

    std::cout << "numResults: " << exaws->resultSet["responseData"]["numResults"].GetInt() << std::endl;
    std::cout << "First data field: " << exaws->data[0][0].GetInt() << std::endl; // first col, first row

    delete (exaws);
    return 0;
}