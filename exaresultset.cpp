//
// Created by mbo on 03.11.2016.
//

#include <iostream>
#include <cstring>
#include "exasockets/exasockets.h"

#include <gperftools/profiler.h>

template<typename T>
void printValue(exaResultSetHandler *rs, int col, int row) {

    std::cout << "- Column: " << col << " (" << (*rs)[col].getName() << ") | Row: " << row << std::endl;
    std::cout << "    + Type: " << exasockets_connection::ExaDatatypeToString((*rs)[col].type()) << std::endl;
    std::cout << "    + Value: " << *std::static_pointer_cast<T>((*rs)[col][row]) << std::endl;

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
        sql_stmt = "select * from pub1092.flights";
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
        exaws = new exasockets_connection(ip, port, "EXASockets", user, pw, 6, true);
    } catch (const char *msg) {
        std::cerr << msg << std::endl;
        return -1;
    }

    std::cout << "Got session ID: " << exaws->session_id() << std::endl;

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    // ProfilerStart("/tmp/exaresultset.prof");
    // exaws->json_debug_output = true;
    exaResultSetHandler *rs = exaws->exec_sql(sql_stmt);
    exaws->json_debug_output = false;
    // ProfilerStop();
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();


    std::cout << "rows received: " << rs->rows() << " - duration " << " : "
              << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000
              << " ms." << std::endl;

    printValue<std::string>(rs, 0, 0);



    delete (rs);
    delete (exaws);

    return 0;
}
