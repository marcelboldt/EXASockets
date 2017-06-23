# EXASockets
Interface to the EXASOL DB via JSON over Websockets protocol.

## Build

These libs are intended to work on Windows and POSIX systems. For building the linker needs:

- the websockets libs (included as a git submodule; see below), see https://github.com/marcelboldt/websockets/
- RapidJSON 1.10, see https://github.com/miloyip/rapidjson/
- OpenSSL (tested with v1.1.0b 26 Sep 2016), see https://www.openssl.org/
- OpenMP (optional but recommended)

No other non C++ standard libraries (e.g. boost) are needed.

As the websockets library is included as a submodule, clone the repository recursively by
 ```git clone --recursive <project_url>```, or pull the submodule before building 
 by ```git submodule update --init```. For details see e.g. https://github.com/blog/2104-working-with-submodules


## Status

Work in progress... but probably already useful.


### Functionality: 

What basically works:

- Connect & Disconnect
- Sending a query
- Fetching & closing a result set
- Writing into DB via prepared insert statement

The result set is stored in an exaResultSet that stores data column-wise in C++ std::vectors 
of an appropriate data type, depending on the column data type. However, the data type mapping is not yet perfect and
may be improved in the future.

### Performance:

The library is C++ with a custom implementation of the websocket protocol and relying on RapidJSON, 
which is pretty fast and works well for huge JSONs.

Tested with EXASOL 6.0.0: Measured is sending a query and fetching a 
result set. Testing with the flights data set with a VM on the same machine, 
 a simple query (e.g. metadata request) returns within 10-20 ms; an uncompressed fetch of 10k rows / 970 KB
  RAW data took me ~ 900 - 1300 ms. Fetching 100k rows / 9.8 MB RAW data typically took about 8.5 sec.

### Stability:

There is still some error-handling left to be done. Bug reports and feedback are highly appreciated.


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
    
    
    
    // writing into DB (prepared insert)
    
    exaResultSetHandler *prep_rs = exaws->create_prepared_insert("insert into test.tbl1 values (?,?);");

    std::string s = "1";
    for (int i = 0; i < 1000000; i++) (*prep_rs)[0].appendData(&s);
    
    double d = 3.123;
    for (int i = 0; i < 1000000; i++) (*prep_rs)[1].appendData(&d);
    
    exaws->exec_prepared_insert(*prep_rs);   
    exaws->close_prepared(*prep_rs);

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


## Documentation

See https://marcelboldt.github.io/EXASockets/


## See also

https://github.com/EXASOL/websocket-api/blob/master/WebsocketAPI.md

https://www.exasol.com/portal
