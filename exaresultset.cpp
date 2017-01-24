//
// Created by mbo on 03.11.2016.
//

#include<iostream>
#include <cstring>
#include "exasockets/exasockets.h"

template<typename T>
void printValue(exaResultSetHandler *rs, int col, int row) {
    std::cout << "- Column: " << col << " (" << (*rs)[col].getName() << ") | Row: " << row << std::endl;
    std::cout << "    + Type: " << exasockets_connection::ExaDatatypeToString((*rs)[col].type()) << std::endl;
    std::cout << "    + Value: " << *static_cast<T *>((*rs)[col][row]) << std::endl;
}


int main(int argc, char **argv) {

    char *sql_stmt;

    if (argc > 1) {
        sql_stmt = argv[1];
    } else {
        sql_stmt = "select * from pub1092.flights limit 200000";
    }

    exasockets_connection *exaws;
    try {
        exaws = new exasockets_connection("192.168.137.10", 8563, "MBO", "sys", "exasol", 6, false);
    } catch (const char *msg) {
        std::cerr << msg << std::endl;
        return -1;
    }

    std::cout << exaws->session_id() << std::endl;

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    exaResultSetHandler *rs = exaws->exec_sql(sql_stmt);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    /*
    printValue<std::string>(rs, 0, 0);
    printValue<std::string>(rs, 1, 0);
    printValue<std::string>(rs, 2, 0);
    printValue<std::string>(rs, 3, 0);
    printValue<std::string>(rs, 4, 0);
    printValue<std::string>(rs, 5, 0);
    printValue<std::string>(rs, 6, 0);
    printValue<std::string>(rs, 7, 0);

     */

    std::cout << "Rows received: " << rs->rows() << std::endl;
    printValue<std::string>(rs, 9, 0);

    delete (rs);
    delete (exaws);

    return 0;
}
