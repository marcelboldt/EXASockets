//
// Created by marcel on 06.03.17.
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
        sql_stmt = "select * from pub1092.flights limit 10;";
    }

    exasockets_connection *exaws;
    try {
        exaws = new exasockets_connection("192.168.137.8", 8563, "MBO", "sys", "exasol", 6, false);
    } catch (const char *msg) {
        std::cerr << msg << std::endl;
        return -1;
    }

    std::cout << exaws->session_id() << std::endl;

    // test

    exaResultSetHandler *rs = exaws->create_prepared(sql_stmt);
    std::cout << "create_prepared OK" << std::endl;

    exaws->exec_prepared(*rs);

    std::cout << "Rows received: " << rs->rows() << std::endl;
    printValue<std::string>(rs, 9, 0);

    delete (rs);
    delete (exaws);

    return 0;
}
