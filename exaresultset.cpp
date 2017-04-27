//
// Created by mbo on 03.11.2016.
//

#include<iostream>
#include <cstring>
#include "exasockets/exasockets.h"

#include <gperftools/profiler.h>

template<typename T>
void printValue(exaResultSetHandler *rs, int col, int row) {

    T *p = static_cast<T *>((*rs)[col][row]);
    std::cout << "- Column: " << col << " (" << (*rs)[col].getName() << ") | Row: " << row << std::endl;
    std::cout << "    + Type: " << exasockets_connection::ExaDatatypeToString((*rs)[col].type()) << std::endl;
    std::cout << "    + Value: " << *p << std::endl;

    delete (p);
}


int main(int argc, char **argv) {

    char *sql_stmt;

    if (argc > 1) {
        sql_stmt = argv[1];
    } else {
        sql_stmt = "select * from pub1092.flights;";
    }

    exasockets_connection *exaws;
    try {
        exaws = new exasockets_connection("192.168.137.8", 8563, "MBO", "sys", "exasol", 6, false);
    } catch (const char *msg) {
        std::cerr << msg << std::endl;
        return -1;
    }

    std::cout << exaws->session_id() << std::endl;

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    ProfilerStart("/tmp/exaresultset.prof");
    exaResultSetHandler *rs = exaws->exec_sql(sql_stmt);
    ProfilerStop();

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();


    std::cout << "Rows received: " << rs->rows() << std::endl;
    printValue<std::string>(rs, 9, 0);

    std::cout << "Duration " << " : "
              << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000
              << " ms." << std::endl;

    delete (rs);
    delete (exaws);

    return 0;
}
