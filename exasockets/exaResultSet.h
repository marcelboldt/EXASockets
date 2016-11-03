/*
exaResultSet.h and exaResultSet.cpp
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

//
// Created by mbo on 02.11.2016.
//

#ifndef EXASOCKETS_EXARESULTSET_H
#define EXASOCKETS_EXARESULTSET_H

#define BOOLEAN 1
#define CHAR 2
#define DATE 3
#define DECIMAL 4
#define DOUBLE 5
#define GEOMETRY 6
#define INTERVAL_DS 7
#define INTERVAL_YM 8
#define TIMESTAMP 9
#define TIMESTAMP_TZ 10
#define VARCHAR 11

#include <cstdint>
#include <vector>
#include <bits/shared_ptr.h>
#include <cstddef>

class exaTblColumn {
public:
    //e.g. new exaTblColumn("col1", BOOLEAN);
    virtual exaTblColumn(char name, int datatype, int precision = 0, int scale = 0);

    virtual ~exaTblColumn();

    virtual void operator[](size_t position);

    virtual void appendData(char *data, size_t start = 1;

    size_t stop = 0
    );
    int type;

    int count();

private:
    exaColData data;
    char name;
    int datatype;
    int precision;
    int scale;
};

template<typename T>
class exaColData : public exaTblColumn {
public:
    exaColData();

    exaColData(std::vector<T> *data);

    exaColData(char *data);

    T operator[](size_t position);

private:
    std::vector<T> *data;
};


class exaResultSet {
public:
    exaResultSet();

    exaResultSet getRows(int start_row_no, int stop_row_no = -1);


private:
    std::vector<std::shared_ptr<exaTblColumn>> columns;
    size_t num_rows;
    int num_cols;
    int handle;
};


#endif //EXASOCKETS_EXARESULTSET_H
