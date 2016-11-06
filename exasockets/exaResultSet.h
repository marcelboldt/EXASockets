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


#ifndef EXASOCKETS_EXARESULTSET_H
#define EXASOCKETS_EXARESULTSET_H

#define EXA_BOOLEAN 301
#define EXA_CHAR 302
#define EXA_DATE 303
#define EXA_DECIMAL 304
#define EXA_DOUBLE 305
#define EXA_GEOMETRY 306
#define EXA_INTERVAL_DS 307
#define EXA_INTERVAL_YM 308
#define EXA_TIMESTAMP 309
#define EXA_TIMESTAMP_TZ 310
#define EXA_VARCHAR 311

#include <cstdint>
#include <vector>
#include <memory>
#include <cstddef>
#include <cstring>



class exaTblColumn {
public:
    static exaTblColumn* create(char *name, int datatype, int precision = 0, int scale = 0);

    exaTblColumn(char *name, int datatype, int precision = 0, int scale = 0);
    virtual ~exaTblColumn() {};

  //  virtual void* operator[](size_t position) = 0;
  //  virtual void appendData(char *data, size_t start) = 0;
    virtual size_t count() const = 0;

protected:
    char* name;
    int datatype;
    int precision;
    int scale;

};

template <typename T>
class exaColumn : public exaTblColumn {
public:
    exaColumn(char *name, int datatype, int precision = 0, int scale = 0) : exaTblColumn(name, datatype, precision, scale) {
       this->name = name;
       this->datatype = datatype;
       this->precision = precision;
       this->scale = scale;

        this->data = new std::vector<T>();
   } ;
    // exaColumn(std::vector<T> *data);
    // exaColumn(char *data);
    // ~exaColumn();

    // T operator[](size_t position);
  //  void appendData(char *data, size_t start = 0);
    size_t count() const;

protected:
    std::vector<T>* data;
};


class exaResultSetHandler {
public:
    //exaTblColumn* c = new exaColumn<bool>("col1", EXA_BOOLEAN);
    exaResultSetHandler();
    ~exaResultSetHandler();
    int addColumn();
    int removeColumn();
    int getRows(int start_row_no, int stop_row_no = -1);
    int fetchRows(size_t limit, size_t offset);

protected:
    std::vector<std::shared_ptr<exaTblColumn>> columns;
    size_t num_rows;
    int num_cols;
    int handle;
};


#endif //EXASOCKETS_EXARESULTSET_H
