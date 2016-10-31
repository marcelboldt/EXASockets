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

#include "exasockets.h"

RSA *createRSA(unsigned char *key, bool publ, int len = -1) {
// http://hayageek.com/rsa-encryption-decryption-openssl-c/
    RSA *rsa = NULL;
    BIO *keybio;
    keybio = BIO_new_mem_buf(key, len);
    if (keybio == NULL) {
        //printf( "Failed to create key BIO");
        throw "Failed to create key BIO";
    }
    if (publ) {
        rsa = PEM_read_bio_RSAPublicKey(keybio, &rsa, NULL, NULL);
    } else {
        rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);
    }
    if (rsa == NULL) {
        //printf("Failed to create RSA");
        throw "Failed to create RSA";
    }

    return rsa;
}

/*
int private_decrypt(unsigned char * enc_data,int data_len,unsigned char * key, unsigned char *decrypted)
{    // http://hayageek.com/rsa-encryption-decryption-openssl-c/
    RSA * rsa = createRSA(key,false);
    int  result = RSA_private_decrypt(data_len,enc_data,decrypted,rsa,RSA_PKCS1_PADDING);
    return result;
}
 */

void printLastError(char *msg) {    // http://hayageek.com/rsa-encryption-decryption-openssl-c/
    char *err = (char *) malloc(130);
    ERR_load_crypto_strings();
    ERR_error_string(ERR_get_error(), err);
    printf("%s ERROR: %s\n", msg, err);
    free(err);
}

exasockets_connection::exasockets_connection(const char *server, uint16_t port, const char *clientName,
                                             const char *username,
                                             const char *password, int pwd_len, bool autocommit, bool use_compression,
                                             uint64_t sessionId) {

    this->status = false;
    this->logfile = "exaloginjson.txt";

    char *host = new char[HOST_NAME_MAX];
    if (gethostname(host, HOST_NAME_MAX) != 0) *host = *clientName;
    const char *driverName = DRV_NAME;
    const char *clientOS = OS_NAME;
    const char *clientOsUsername = getlogin();
    if (clientOsUsername == nullptr) {
        clientOsUsername = "unknown";
    }
    tfile.open(logfile, std::ios::in);
    ws_con = new Websockets_connection(server, port, clientName);

    if (!ws_con->connected()) {
        throw "EXASOCKETS: Websockets connection couldn't be established.";
    }

    rapidjson::StringBuffer s;
    rapidjson::Document d;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);


// Compose LOGIN REQUEST:

    writer.StartObject();
    writer.Key("command");
    writer.String("login");
    writer.Key("protocolVersion");
    writer.Int(1);
    writer.EndObject();

    ws_send_data(s.GetString(), s.GetSize(), 1);


// get LOGIN RESPONSE

    d.Parse(ws_receive_data());
    if (!d.IsObject()) {
        throw "Response parsing failed.";
    } else if (d.HasMember("exception")) {
        throw d["exception"]["text"].GetString();
    }


// printf("PublicKeyExp: %s\n", d["responseData"]["publicKeyPem"].GetString());

    unsigned char *pubkey = new unsigned char[d["responseData"]["publicKeyPem"].GetStringLength() + 1];
    memcpy(pubkey, d["responseData"]["publicKeyPem"].GetString(), d["responseData"]["publicKeyPem"].GetStringLength());
    pubkey[d["responseData"]["publicKeyPem"].GetStringLength() + 1] = 0;

    unsigned char *pwd = new unsigned char[pwd_len];
    memcpy(pwd, password, pwd_len);

// printf("pwd: %s\npubkey: %s", pwd, pubkey);
// RSA encrypt user password


    RSA *rsa = createRSA(pubkey, true);
// RSA * rsa = RSA_generate_key( 1024, 3, 0 , 0 );
// PEM_write_RSAPublicKey(stdout, rsa);
    int enc_len = RSA_size(rsa);

    unsigned char *encrypted = new unsigned char[enc_len];

    srand(10); /* initialize random seed: */ // TODO: another seed, e.g. std::srand(std::time(0));
    if (RSA_public_encrypt(pwd_len, pwd, encrypted, rsa, RSA_PKCS1_PADDING) == -1) {
        throw "RSA_public_encrypt failed";
    }
//    printf("encrypted: %s\n", encrypted);

    BIO *b64 = BIO_new((BIO_METHOD *) BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO *bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, encrypted, enc_len);
    BIO_flush(b64);
    BUF_MEM *bptr = NULL;
    BIO_get_mem_ptr(b64, &bptr);

    unsigned char *encrypted_b64 = new unsigned char[bptr->length];
    memcpy(encrypted_b64, bptr->data, bptr->length);
    BIO_free(b64);
    BIO_free(bmem);
    RSA_free(rsa);


// Build and send the request JSON:
/*
{
    "username": <string>,
    "password": <string>,
    "useCompression": <boolean>,
    "sessionId": <number>,
    "clientName": <string>,
    "driverName": <string>,
    "clientOs": <string>,
    "clientOsUsername": <string>,
    "clientLanguage": <string>,
    "clientVersion": <string>,
    "clientRuntime": <string>,
    "attributes": {
        // as defined separately
    }
}
*/
    s.Clear();
    writer.Reset(s);

    writer.StartObject();
    writer.Key("username");
    writer.String(username);
    writer.Key("password");
    writer.String((const char *) encrypted_b64);
//writer.String("test");
    writer.Key("useCompression");
    writer.Bool(use_compression);
    if (sessionId > 0) {
        writer.Key("sessionId");
        writer.Int(sessionId);
    }
    writer.Key("clientName");
    writer.String(clientName);
    writer.Key("driverName");
    writer.String(driverName);
    writer.Key("clientOs");
    writer.String(clientOS);
    writer.Key("clientOsUsername");
    writer.String(clientOsUsername);
    writer.Key("clientLanguage");
    writer.String("English"); // TODO
    writer.Key("clientVersion");
    writer.String("0.1"); // TODO
    writer.Key("clientRuntime");
    writer.String("runtime"); // TODO
    writer.Key("attributes");
    writer.StartObject();
    writer.Key("autocommit");
    writer.Bool(autocommit);
    writer.Key("compressionEnabled");
    writer.Bool(use_compression);
    writer.EndObject();
    writer.EndObject();


    ws_send_data(s.GetString(), s.GetSize(), 1);


// get LOGIN RESPONSE

    d.Parse(ws_receive_data());
    if (!d.IsObject()) {
        throw "Response parsing failed.";
    } else if (d.HasMember("exception")) {
        throw d["exception"]["text"].GetString();
    }

    assert(d.HasMember("responseData"));
    assert(d["responseData"].HasMember("databaseName"));
    assert(d["responseData"].HasMember("identifierQuoteString"));
    assert(d["responseData"].HasMember("maxDataMessageSize"));
    assert(d["responseData"].HasMember("maxIdentifierLength"));
    assert(d["responseData"].HasMember("maxVarcharLength"));
    assert(d["responseData"].HasMember("productName"));
    assert(d["responseData"].HasMember("protocolVersion"));
    assert(d["responseData"].HasMember("releaseVersion"));
    assert(d["responseData"].HasMember("sessionId"));
    assert(d["responseData"].HasMember("timeZone"));
    assert(d["responseData"].HasMember("timeZoneBehavior"));

    this->c_databaseName = d["responseData"]["databaseName"].GetString();
    this->c_identifierQuoteString = d["responseData"]["identifierQuoteString"].GetString();
    this->c_maxDataMessageSize = d["responseData"]["maxDataMessageSize"].GetInt();
    this->c_maxIdentifierLength = d["responseData"]["maxIdentifierLength"].GetInt();
    this->c_maxVarcharLength = d["responseData"]["maxVarcharLength"].GetInt();
    this->c_productName = d["responseData"]["productName"].GetString();
    this->c_protocolVersion = d["responseData"]["protocolVersion"].GetInt();
    this->c_releaseVersion = d["responseData"]["releaseVersion"].GetString();
    this->c_session_id = d["responseData"]["sessionId"].GetUint64();
    this->c_timezone = d["responseData"]["timeZone"].GetString();
    this->c_timeZoneBehaviour = d["responseData"]["timeZoneBehavior"].GetString();

    this->status = true;
}

exasockets_connection::~exasockets_connection() {
    if (this->status) {

        rapidjson::StringBuffer s;
        rapidjson::Document d;
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);

// Compose LOGIN REQUEST:

        writer.StartObject();
        writer.Key("command");
        writer.String("disconnect");
        writer.EndObject();

// std::cout << s.GetString() << std::endl;
        //ws_con->send_data(s.GetString(),s.GetSize(), 1);
        ws_send_data(s.GetString(), s.GetSize(), 1);
        this->status = false;

// get LOGIN RESPONSE

        d.Parse(ws_receive_data());
        if (!d.IsObject()) {
            throw "Disconnect: Response parsing failed.";
            // std::cout << "Disconnect: Response parsing failed." << std::endl;
        } else if (d.HasMember("exception")) {
            std::cout << d["exception"]["text"].GetString() << std::endl;
        } else {
            std::cout << d["status"].GetString() << std::endl;
        }
    }
}


char *exasockets_connection::ws_receive_data() {
    /* reads from websocket and returns the data */

    int len = ws_con->receive_data(logfile);
    char *recv_line = new char[len];

    tfile.seekg(0, std::ios::beg);
    tfile.getline(recv_line, len);

    if (this->json_debug_output) {
        std::cout << "RECV:" << recv_line << std::endl;
    }

    return recv_line;
}

int exasockets_connection::ws_send_data(const char *data, int len, int type) {

    if (this->json_debug_output) {
        if (type == 1) { std::cout << "SEND: " << data << std::endl; }
        else if (type == 2) {
            std::cout << "SEND: " << "binary length: " << len << std::endl;
        }
    }

    if (len < 0) {
        return ws_con->send_data(data, strlen(data), type);
    } else {
        return ws_con->send_data(data, len, type);
    }
}


const char *exasockets_connection::databaseName() const {
    return c_databaseName;
}

const char *exasockets_connection::identifierQuoteString() const {
    return c_identifierQuoteString;
}

int exasockets_connection::maxDataMessageSize() const {
    return c_maxDataMessageSize;
}

int exasockets_connection::maxIdentifierLength() const {
    return c_maxIdentifierLength;
}

int exasockets_connection::maxVarcharLength() const {
    return c_maxVarcharLength;
}

const char *exasockets_connection::productName() const {
    return c_productName;
}

int exasockets_connection::protocolVersion() const {
    return c_protocolVersion;
}

const char *exasockets_connection::releaseVersion() const {
    return c_releaseVersion;
}

uint64_t exasockets_connection::session_id() const {
    return c_session_id;
}

const char *exasockets_connection::timezone() const {
    return c_timezone;
}

const char *exasockets_connection::timeZoneBehaviour() const {
    return c_timeZoneBehaviour;
}

bool exasockets_connection::isStatus() const {
    return status;
}

int exasockets_connection::update_session_attributes() {
    /* Retrieves the session attribute values from DB and updates the object's metadata
     * */

    /*
     *  {
     "command": "getAttributes",
     "attributes": {
             // as defined separately
     },
     }
     */

    rapidjson::StringBuffer s;
    rapidjson::Document d;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);

    writer.StartObject();
    writer.Key("command");
    writer.String("getAttributes");
    writer.EndObject();

    ws_send_data(s.GetString(), s.GetSize(), 1);

    d.Parse(ws_receive_data());

    if (!d.IsObject()) {
        throw "update_session_attributes: Response parsing failed.";
        // std::cout << "update_session_attribute: Response parsing failed." << std::endl;
    } else if (d.HasMember("exception")) {
        std::cout << d["exception"]["text"].GetString() << std::endl;
    } else {

        // TODO: parse parameters

    }
    return 0;
}

int exasockets_connection::exec_sql(char *sql) {
    /* Sends an SQL stmt and fetches the result set */

    /*
     *  {
     "command": "execute",
     "attributes": {
             // as defined separately
     },
     "sqlText": <string>
 }
     */

    rapidjson::StringBuffer s;
//rapidjson::Document d;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);

    writer.StartObject();
    writer.Key("command");
    writer.String("execute");
    writer.Key("sqlText");
    writer.String(sql);
    writer.EndObject();

    ws_send_data(s.GetString(), s.GetSize(), 1);

   // std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    this->resultSet.Parse(ws_receive_data());

    if (!this->resultSet.IsObject()) {
        throw "update_session_attributes: Response parsing failed.";
// std::cout << "update_session_attribute: Response parsing failed." << std::endl;
    } else if (this->resultSet.HasMember("exception")) {
        std::cout << this->resultSet["exception"]["text"].GetString() << std::endl;
    } else {

// TODO: parse parameters

    }

    return 0;
}
