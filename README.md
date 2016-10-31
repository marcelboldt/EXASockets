# EXASockets
Interface to the EXASOL DB via JSON over Websockets protocol.

## Status

Work in progress... what basically works:

- Connect & Disconnect
- Sending a query
- Fetching a result set

## Build

These libs are intended to work on Windows and POSIX systems. For building the linker needs:

- the websockets libs, see https://github.com/marcelboldt/websockets/
- RapidJSON 1.10, see https://github.com/miloyip/rapidjson/
- OpenSSL (tested with v1.1.0b 26 Sep 2016), see https://www.openssl.org/


## See also

https://github.com/EXASOL/websocket-api/blob/master/WebsocketAPI.md