//
// Created by mbo on 03.11.2016.
//

#include<iostream>
#include <cstring>
#include "exasockets/exasockets.h"
#include "exasockets/exaResultSet.h"


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

    int h = exaws->exec_sql(sql_stmt);
    if (h > 0) { // a result set handle received (result set >= 1000 rows))
        std::cout << "rows fetched: " << exaws->fetch(h, 0, 1, (10485760 * 30)) << std::endl;

    }


    exaTblColumn *c = exaTblColumn::create("col1", EXA_BOOLEAN);
    // now iterate through the bool column and insert the data one by one in the exaresultset

    bool b;
    for (auto &item : exaws->data[1].GetArray()) {

        if (item.IsNull()) {
            // c->appendData(nullptr); // how to store NULL values? this causes a segfault
        } else {
            b = item.GetBool();
            // bool b = true;
            c->appendData(&b);
        }
    }
    //
    //  std::cout << c[1] << std::endl;

    // exaResultSetHandler r = new exaResultSetHandler();
    // r.addColumn(c);

    return 0;
}
