/*
websockets.cpp and websockets.h
http://www.github.com/marcelboldt/websockets

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

#ifndef EXASOCKETS_EXASOCKETS_H
#define EXASOCKETS_EXASOCKETS_H

#include "../../sockets/websockets/websockets.h"
#include "osname.h"
#include "../rapidjson/document.h"
#include "../rapidjson/writer.h"
#include "../rapidjson/stringbuffer.h"
#include<iostream>
#include <openssl/pem.h>
//#include <openssl/ssl.h>
#include <openssl/rsa.h>
//#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/err.h>


#include <unistd.h> // gethostname, getlogin

#define __USE_POSIX

#include <limits.h> // HOST_NAME_MAX

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 16
#endif

#define DRV_NAME "EXASOCKETS 0.1"

class exasockets_connection {
public:
    exasockets_connection(const char *server,
                          uint16_t port,
                          const char *clientName,
                          const char *username,
                          const char *password,
                          int pwd_len,
                          bool autocommit = true,
                          bool use_compression = false,
                          uint64_t sessionId = 0 // 0 -> create randomly
    );

    ~exasockets_connection();

    int update_session_attributes();

    int exec_sql(char *sql);

    const char *databaseName() const;

    const char *identifierQuoteString() const;

    int maxDataMessageSize() const;

    int maxIdentifierLength() const;

    int maxVarcharLength() const;

    const char *productName() const;

    int protocolVersion() const;

    const char *releaseVersion() const;

    uint64_t session_id() const;

    const char *timezone() const;

    const char *timeZoneBehaviour() const;

    bool isStatus() const;

    rapidjson::Document resultSet;
    bool json_debug_output = true;


private:
    Websockets_connection *ws_con;
    std::ifstream tfile;
    char *logfile;

    const char *c_databaseName;
    const char *c_identifierQuoteString;
    int c_maxDataMessageSize;
    int c_maxIdentifierLength;
    int c_maxVarcharLength;
    const char *c_productName;
    int c_protocolVersion;
    const char *c_releaseVersion;
    uint64_t c_session_id;
    const char *c_timezone;
    const char *c_timeZoneBehaviour; // change to predef
    bool status;


    int ws_send_data(const char *data, int len = -1, int type = 1);

    char *ws_receive_data();


};

#endif //EXASOCKETS_EXASOCKETS_H
