//
// Created by mbo on 03.11.2016.
//

#include<iostream>
#include <cstring>
#include "exasockets/exasockets.h"


int main(int argc, char **argv) {

    char *sql_stmt;

    if (argc > 1) {
        sql_stmt = argv[1];
    } else {
        sql_stmt = "select * from test.bools;";
    }

    exasockets_connection *exaws;
    try {
        exaws = new exasockets_connection("192.168.137.10", 8563, "MBO", "sys", "exasol", 6, false);
    } catch (const char *msg) {
        std::cerr << msg << std::endl;
        return -1;
    }

    std::cout << exaws->session_id() << std::endl;

    exaResultSetHandler *rs = exaws->exec_sql("select * from test.bools;");

    std::cout << "Data Value from result set: " << *static_cast<int32_t *>((*rs)[0][1]) << std::endl;

    delete (rs);
    delete (exaws);

    return 0;
}
