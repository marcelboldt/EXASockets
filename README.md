# EXASockets
Interface to the EXASOL DB via JSON over Websockets protocol.

## Status

Work in progress... 

### Functionality: 

What basically works:

- Connect & Disconnect
- Sending a query
- Fetching a result set

### Performance:

Tested with EXASOL 6 Beta 1: The application connects, send a query and fetches a 
result set (which needs to be done separately using 'fetch()' if the result set is
 1000 rows or greater). Testing with the flights data set with an EXASolo on the same machine, 
 a simple query (e.g. metadata request) returns within 10-20 ms; an uncompressed fetch of 10k rows / 970 KB
  RAW data took me ~ 900 - 1300 ms. Fetching 100k rows / 9.8 MB RAW data typically took about 8.5 sec.

### Stability:

Regarding the interface, nothing is final.
There is certainly still some error-handling left to do. I appreciate any bug report!

## Example code:

```C++

#include <iostream>
#include "exasockets/exasockets.h"

int main() {
  
    exasockets_connection *exaws;
    
    // connecting to EXASOL (IP, port, client descriptor, username, password, password length, autocommit)
    try {
        exaws = new exasockets_connection("192.168.137.10", 8563, "MBO", "sys", "exasol", 6, false);
    } catch (const char *msg) {
        std::cerr << msg << std::endl;
    }
    
    // returning the EXASOL session ID
    std::cout << exaws->session_id() << std::endl;

    // sending a query to EXASOL
    int h = exaws->exec_sql("select * from pub1092.flights limit 100000");
    
    // if exec_sql() returns a handle, use it to fetch the result set
    // results sets < 1000 rows are immediately fetched by exec_sql()
    // fetch(handle, row offset / first row to fetch, Bytes to fetch - here 30 MB)
    if (h > 0) { // a result set handle received (result set >= 1000 rows))
        std::cout << "rows fetched: " << exaws->fetch(h, 0, (10485760 * 3)) << std::endl;
    }

    // accessing the data 
    // data is a rapidJSON Value object. For details see the rapidJSON documentation (link below).
    std::cout << "First data field: " << exaws->data[0][0].GetInt() << std::endl; // first col, first row

    // a clean disconnect is done on destruction of the connection object.
    delete (exaws);
    
    return 0;
}

```


## Build

These libs are intended to work on Windows and POSIX systems. For building the linker needs:

- the websockets libs, see https://github.com/marcelboldt/websockets/
- RapidJSON 1.10, see https://github.com/miloyip/rapidjson/
- OpenSSL (tested with v1.1.0b 26 Sep 2016), see https://www.openssl.org/


## Documentation

Also in progress...

See https://marcelboldt.github.io/EXASockets/

A tutorial for rapidJSON: http://rapidjson.org/md_doc_tutorial.html

## See also

https://github.com/EXASOL/websocket-api/blob/master/WebsocketAPI.md

https://www.exasol.com/portal
