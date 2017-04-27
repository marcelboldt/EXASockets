# EXASockets
Interface to the EXASOL DB via JSON over Websockets protocol.

## Status

Work in progress... but probably already useful.

### Functionality: 

What basically works:

- Connect & Disconnect
- Sending a query
- Fetching a result set

The result set is stored in an exaResultSet that stores data column-wise in C++ std::vectors 
of an appropriate data type, depending on the column data type. However, the data type mapping is not yet perfect and
may be improved in the future.

### Performance:

The library is C++ with a custom implementation of the websocket protocol and relying on rapidJSON, 
which is pretty fast and works well for huge JSONs. Promising so far...

Tested with EXASOL 6.0.0: Measured is sending a query and fetching a 
result set. Testing with the flights data set with a VM on the same machine, 
 a simple query (e.g. metadata request) returns within 10-20 ms; an uncompressed fetch of 10k rows / 970 KB
  RAW data took me ~ 900 - 1300 ms. Fetching 100k rows / 9.8 MB RAW data typically took about 8.5 sec.

### Stability:

Regarding the interface, nothing is final.
There is certainly still some error-handling left to do. I appreciate any bug report!


## Use:

```C++

#include <iostream>
#include "exasockets/exasockets.h"

int main() {
  
    exasockets_connection *exaws;
    
    // connecting to EXASOL (IP, port, client descriptor, username, password, password length, autocommit)
    try {
        exaws = new exasockets_connection("192.168.137.10", 8563, "user", "sys", "exasol", 6, false);
    } catch (const char *msg) {
        std::cerr << msg << std::endl;
    }
    
    // returning the EXASOL session ID
    std::cout << exaws->session_id() << std::endl;

    // sending a query to EXASOL and receiving a result set
    exaResultSetHandler *rs = exaws->exec_sql("select * from test.flights;");
    

    // accessing the data 
    // data is stored in an exaResultSet. Values can be addressed two-dimensionally [column][row];
    // returned is a void pointer that may be casted into the appropriate C datatype (see section "Data type mapping").
    
    int col = 0; int row = 1; // first column, second row
    
    std::string * value = static_cast<std::string *>((*rs)[col][row]);
    std::cout << "Column name: " << (*rs)[col].getName() << std::endl;
    std::cout << "Column type: " << exasockets_connection::ExaDatatypeToString((*rs)[col].type()) << std::endl;
    std::cout << "Value: " << *value << std::endl;
    delete(value);

    // a clean disconnect is done on destruction of the connection object.
    delete (exaws);
    
    return 0;
}

```

### Data type mapping

| EXASOL data type 	| client data type (C++ type) 	|
|------------------	|-----------------------------	|
| BOOLEAN          	| bool                        	|
| CHAR             	| std::string                 	|
| DATE             	| std::string                 	|
| DECIMAL          	| std::string                 	|
| DOUBLE           	| double                      	|
| GEOMETRY         	| not yet implemented         	|
| INTERVAL_DS      	| not yet implemented         	|
| INTERVAL_YM      	| not yet implemented         	|
| TIMESTAMP        	| std::string                 	|
| VARCHAR          	| std::string                 	|


## Build

These libs are intended to work on Windows and POSIX systems. For building the linker needs:

- the websockets libs, see https://github.com/marcelboldt/websockets/
- RapidJSON 1.10, see https://github.com/miloyip/rapidjson/
- OpenSSL (tested with v1.1.0b 26 Sep 2016), see https://www.openssl.org/
- OpenMP

No other non C++ standard libraries (e.g. boost) are needed.


## Documentation

Also in progress...

See https://marcelboldt.github.io/EXASockets/


## See also

https://github.com/EXASOL/websocket-api/blob/master/WebsocketAPI.md

https://www.exasol.com/portal
