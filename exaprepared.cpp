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
    std::cout << "    + Value: " << *std::static_pointer_cast<T>((*rs)[col][row]) << std::endl;

}

void printMetadata(exaResultSetHandler *rs) {

    for (int col = 0; col < rs->cols(); col++) {
        std::cout << "- Column: " << col << " (" << (*rs)[col].getName() << ") " << std::endl;
        std::cout << "    + Type: " << exasockets_connection::ExaDatatypeToString((*rs)[col].type()) << std::endl;
    }

}

int my_str2int(const char *s) { // http://stackoverflow.com/questions/17478793/converting-multi-digit-char-number-to-int
    int res = 0;
    while (*s) {
        res *= 10;
        res += *s++ - '0';
    }

    return res;
}

int main(int argc, char **argv) {

    char *ip;
    uint16_t port;
    char *user;
    char *pw;
    char *sql_stmt;

    if (argc > 1) {
        ip = argv[1];
        port = my_str2int(argv[2]);
        user = argv[3];
        pw = argv[4];
        sql_stmt = argv[5];
    } else {
        ip = "192.168.137.8";
        port = 8563;
        user = "sys";
        pw = "exasol";
        sql_stmt = "insert into test.tbl1 values (?,?);";
        //sql_stmt = "select * from pub1092.flights limit 1";
    }

    std::cout << "connecting to:"
              << "\n  IP   : " << ip
              << "\n  port : " << port
              << "\n  user : " << user
              << "\n  pw   : " << pw
              << "\n  SQL  : " << sql_stmt
              << std::endl;

    exasockets_connection *exaws;


    try {
        exaws = new exasockets_connection("192.168.137.8", 8563, "MBO", "sys", "exasol", 6, true, false, 0, false);
    } catch (const char *msg) {
        std::cerr << msg << std::endl;
        return -1;
    }

    std::cout << exaws->session_id() << std::endl;

    exaws->json_debug_output = true;

    exaResultSetHandler *rs = exaws->create_prepared_insert(sql_stmt);
    std::cout << "create_prepared OK" << std::endl;
    printMetadata(rs);

    std::string s = "1";
    for (int i = 0; i < 10000; i++) (*rs)[0].appendData(&s);

    double d = 3.123;
    for (int i = 0; i < 10000; i++) (*rs)[1].appendData(&d);

    for (int i = 0; i < 1; i++) {
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        exaws->exec_prepared_insert(*rs);
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

        std::cout << "Insert " << i << " duration " << " : "
                  << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000
                  << " ms." << std::endl;
    }

    exaws->close_prepared(*rs);

    exaws->json_debug_output = false;

    //  std::cout << "Rows received: " << rs->rows() << std::endl;
    //  printValue<std::string>(rs, 9, 0);

    delete (rs);
    delete (exaws);

    return 0;
}
