/*
websockets.cpp and websockets.h
http://www.github.com/marcelboldt/EXASockets

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
        throw exas_rsa_bio_create();
    }
    if (publ) {
        rsa = PEM_read_bio_RSAPublicKey(keybio, &rsa, NULL, NULL);
    } else {
        rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);
    }
    if (rsa == NULL) {
        throw exas_rsa_create();
    }

    return rsa;
}


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
                                             uint64_t sessionId, bool debug) {
    this->json_debug_output = debug;

//    Websockets_connection::write_msg_to_file("exasock init");
//    Websockets_connection::write_msg_to_file(OS_NAME);


    this->status = false;
    if (this->logfile == nullptr) {
        this->logfile = new char[128];
        time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        strftime(this->logfile, 128, "EXASockets %Y-%m-%d %X", std::localtime(&tt));
        sprintf(this->logfile, "%s %i.tmp", logfile, rand() % 65536);
        std::stringstream outfile(this->logfile); //ostream
        // outfile.close();
    }


    char *host = new char[HOST_NAME_MAX];
    if (gethostname(host, HOST_NAME_MAX) != 0) *host = *clientName;
    const char *driverName = DRV_NAME;
    const char *clientOS = OS_NAME;
    const char *clientOsUsername =nullptr; // = OSTools::getlogin(); TODO
    if (clientOsUsername == nullptr) {
        clientOsUsername = "unknown";
    }

    // tfile.open(this->logfile, std::ios::in);
    //  if(!tfile.is_open()) {
    //      throw std::runtime_error("EXASockets: Couldn't open tempfile.");
    //  }
    //  Websockets_connection::write_msg_to_file("ws init");
    ws_con = new Websockets_connection(server, port, clientName);
    if (ws_con == nullptr) throw std::runtime_error("Error: failed to initialise websockets connection");
    if (!ws_con->connected()) throw std::runtime_error("EXASOCKETS: Websockets connection couldn't be established.");


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

    //   std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    d.Parse(ws_receive_data().c_str());
    if (!d.IsObject()) {
        throw std::runtime_error("Login stage 1: Response parsing failed.");
    } else if (d.HasMember("exception")) {
        throw std::runtime_error(d["exception"]["text"].GetString());
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
    delete (pubkey);
// RSA * rsa = RSA_generate_key( 1024, 3, 0 , 0 );
// PEM_write_RSAPublicKey(stdout, rsa);
    int enc_len = RSA_size(rsa);

    unsigned char *encrypted = new unsigned char[enc_len];

    srand(10); /* initialize random seed: */ // TODO: another seed, e.g. std::srand(std::time(0));
    if (RSA_public_encrypt(pwd_len, pwd, encrypted, rsa, RSA_PKCS1_PADDING) == -1) {
        throw std::runtime_error("RSA_public_encrypt failed");
    }
    delete (pwd);
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
    delete (encrypted);


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
    writer.String(host);
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
    writer.String(clientName);
    writer.Key("attributes");
    writer.StartObject();
    writer.Key("autocommit");
    writer.Bool(autocommit);
    writer.Key("compressionEnabled");
    writer.Bool(use_compression);
    writer.EndObject();
    writer.EndObject();

    delete (host);
    delete (encrypted_b64);

    ws_send_data(s.GetString(), s.GetSize(), 1);


// get LOGIN RESPONSE

    d.Parse(ws_receive_data().c_str());
    if (!d.IsObject()) {
        throw std::runtime_error("Login stage 2: Response parsing failed.");
    } else if (d.HasMember("exception")) {
        throw std::runtime_error(d["exception"]["text"].GetString());
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

int exasockets_connection::disconnect(bool throw_message) {
    if (this->status) {

        rapidjson::StringBuffer s;
        rapidjson::Document d;
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);

// Compose disconnect REQUEST:

        writer.StartObject();
        writer.Key("command");
        writer.String("disconnect");
        writer.EndObject();

// std::cout << s.GetString() << std::endl;
        //ws_con->send_data(s.GetString(),s.GetSize(), 1);
        ws_send_data(s.GetString(), s.GetSize(), 1);
        this->status = false;

// get LOGIN RESPONSE

        d.Parse(ws_receive_data().c_str());
        if (!d.IsObject()) {
            throw std::runtime_error("Disconnect: Response parsing failed.");
        } else if (d.HasMember("exception")) {
            throw std::runtime_error(d["exception"]["text"].GetString());
            return 1; // some error
        } else {
            if (throw_message) std::cout << "Disconnected from EXASOL." << std::endl;
            return 0; // all fine
        }
    } else {
        return -1; // already disconnected
    }
}

exasockets_connection::~exasockets_connection() {

    disconnect();
    delete (this->ws_con);
    delete (this->logfile);

    //  if (remove(this->logfile) != 0) throw std::runtime_error("EXASockets tempfile couldn't be deleted. Look for garbage in the application's working directory.");
}


std::string exasockets_connection::ws_receive_data() {
    /* reads from websocket and returns the data */


    std::string buf;
    int len = ws_con->receive_data(&buf);

    if (this->json_debug_output) {
        std::cout << "RECV:" << buf << std::endl;
    }


    return buf;
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

    d.Parse(ws_receive_data().c_str());

    if (!d.IsObject()) {
        throw std::runtime_error("update_session_attributes: Response parsing failed.");
    } else if (d.HasMember("exception")) {
        throw std::runtime_error(d["exception"]["text"].GetString());
    } else {

        // TODO: parse parameters
        throw std::runtime_error("update_session_attributes not implemented");

    }
    return 0;
}

int exasockets_connection::StringToExaDatatype(const char *str) {
    if (strcmp(str, "BOOLEAN") == 0) return EXA_BOOLEAN;
    if (strcmp(str, "CHAR") == 0) return EXA_CHAR;
    if (strcmp(str, "DATE") == 0) return EXA_DATE;
    if (strcmp(str, "DECIMAL") == 0) return EXA_DECIMAL;
    if (strcmp(str, "DOUBLE") == 0) return EXA_DOUBLE;
    if (strcmp(str, "GEOMETRY") == 0) return EXA_GEOMETRY;
    if (strcmp(str, "INTERVAL_DS") == 0) return EXA_INTERVAL_DS;
    if (strcmp(str, "INTERVAL_YM") == 0) return EXA_INTERVAL_YM;
    if (strcmp(str, "TIMESTAMP") == 0) return EXA_TIMESTAMP;
    if (strcmp(str, "TIMESTAMP_TZ") == 0) return EXA_TIMESTAMP_TZ;
    if (strcmp(str, "VARCHAR") == 0) return EXA_VARCHAR;
    return 0;
}

char *exasockets_connection::ExaDatatypeToString(const int type) {
    if (type == EXA_BOOLEAN) return "BOOLEAN";
    if (type == EXA_CHAR) return "CHAR";
    if (type == EXA_DATE) return "DATE";
    if (type == EXA_DECIMAL) return "DECIMAL";
    if (type == EXA_DOUBLE) return "DOUBLE";
    if (type == EXA_GEOMETRY) return "GEOMETRY";
    if (type == EXA_INTERVAL_DS) return "INTERVAL_DS";
    if (type == EXA_INTERVAL_YM) return "INTERVAL_YM";
    if (type == EXA_TIMESTAMP) return "TIMESTAMP";
    if (type == EXA_TIMESTAMP_TZ) return "TIMESTAMP_TZ";
    if (type == EXA_VARCHAR) return "VARCHAR";
    return 0;
}


void exasockets_connection::append_data_from_Rapid_JSON_Document(exaResultSetHandler *rs, const rapidjson::Value &JSONdata) {

    // and insert the data
    // int i = 0;


#pragma omp parallel for
    //  for (auto exa_col : rs->getColumns()) {// all columns
    for (int i = 0; i < rs->cols(); i++) {
        auto exa_col = rs->getColumns()[i];

        for (auto &item : JSONdata[i].GetArray()) {     //  iterate through the column and insert the data one by one in the exaresultset

            if (item.IsNull()) {
                exa_col->appendData(nullptr, true);
            } else {
                switch (exa_col->type()) {
                    case EXA_BOOLEAN : {
                        auto bool1 = item.GetBool();
                        exa_col->appendData(&bool1);
                        break;
                    }
                    case EXA_CHAR : {
                        std::string str1 = item.GetString();
                        exa_col->appendData(&str1);
                        break;
                    }
                    case EXA_DATE : {
                        std::string s = item.GetString();
                        exa_col->appendData(&s);
                        break;
                    }
                    case EXA_DECIMAL : {// TODO: determine if int or float...
                        std::string str1;
                        if (item.IsString()) { // a decimal with scale > 0
                            str1 = item.GetString();

                        } else {
                            str1 = std::to_string(item.GetInt());
                        }
                        exa_col->appendData(&str1);
                        break;
                    }
                    case EXA_DOUBLE : {
                        double d1 = item.GetDouble();
                        exa_col->appendData(&d1);
                        break;
                    }
                    case EXA_TIMESTAMP : {
                        std::string s = item.GetString();
                        exa_col->appendData(&s);
                        break;
                    }
                    case EXA_VARCHAR : {
                        std::string str1 = item.GetString();
                        exa_col->appendData(&str1);
                        break;
                    }

                }
            }
        }
        //   i++;
    }

}


// Inits and returns an exaResultSet from a rapidJSON result set
exaResultSetHandler *
exasockets_connection::create_exaResultSetHandler_from_RapidJSON_Document(const rapidjson::Value &JSONresultSet,
                                                                          int handle) {

    exaResultSetHandler *exa_rs = new exaResultSetHandler();

    size_t num_rows = 0;
    if (JSONresultSet.HasMember("numRows") && JSONresultSet["numRows"].IsInt64()) {
        num_rows = JSONresultSet["numRows"].GetInt64();
    }

    int precision;
    int scale;
    int size;
    char *charset;
    bool w_local_tz;
    int fraction;
    int srid;


    for (auto i = 0; i < JSONresultSet["numColumns"].GetInt64(); i++) { //for each column

        precision = JSONresultSet["columns"][i]["dataType"].HasMember("precision")
                    ? JSONresultSet["columns"][i]["dataType"]["precision"].GetInt() : -1;

        scale = JSONresultSet["columns"][i]["dataType"].HasMember("scale")
                ? JSONresultSet["columns"][i]["dataType"]["scale"].GetInt() : -1;

        size = JSONresultSet["columns"][i]["dataType"].HasMember("size")
               ? JSONresultSet["columns"][i]["dataType"]["size"].GetInt() : -1;

        charset = (char *) (JSONresultSet["columns"][i]["dataType"].HasMember("characterSet")
                            ? JSONresultSet["columns"][i]["dataType"]["characterSet"].GetString() : nullptr);

        w_local_tz = JSONresultSet["columns"][i]["dataType"].HasMember("withLocalTimeZone")
                     ? JSONresultSet["columns"][i]["dataType"]["withLocalTimeZone"].GetBool() : false;

        fraction = JSONresultSet["columns"][i]["dataType"].HasMember("fraction")
                   ? JSONresultSet["columns"][i]["dataType"]["fraction"].GetInt() : -1;

        srid = JSONresultSet["columns"][i]["dataType"].HasMember("srid")
               ? JSONresultSet["columns"][i]["dataType"]["srid"].GetInt() : -1;



        std::shared_ptr<exaTblColumn> exa_col(
                exaTblColumn::create((char *) JSONresultSet["columns"][i]["name"].GetString(),
                                     StringToExaDatatype(
                                             JSONresultSet["columns"][i]["dataType"]["type"].GetString()),
                                     num_rows, precision, scale, size, charset, w_local_tz, fraction, srid
                ));

        exa_rs->addColumn(exa_col);
    }
    exa_rs->setHandle(handle);
    return exa_rs;
}


exaResultSetHandler *exasockets_connection::exec_sql(char *sql) {

    exaResultSetHandler *exa_rs = nullptr;

    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);

    writer.StartObject();
    writer.Key("command");
    writer.String("execute");
    writer.Key("sqlText");
    writer.String(sql);
    writer.EndObject();

    ws_send_data(s.GetString(), s.GetSize(), 1);

    // std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    this->resultSet.Parse(ws_receive_data().c_str());

    if (!this->resultSet.IsObject()) { // nonsense received
        throw std::runtime_error("exec_sql: Response parsing failed.");
    } else if (this->resultSet.HasMember("exception")) { // DB had a problem
        throw std::runtime_error(this->resultSet["exception"]["text"].GetString());

    } else { // something useful received
        if (this->resultSet["responseData"].HasMember("results")) { // a result received
            const rapidjson::Value &jrs = this->resultSet["responseData"]["results"][0]["resultSet"];
            exa_rs = create_exaResultSetHandler_from_RapidJSON_Document(jrs);


            if (this->resultSet["responseData"]["results"][0]["resultSet"].HasMember("resultSetHandle")) {
                // result contains a handle... separate fetching needed
                //  call fetch(), then return the exaResultSet composed
                int h = this->resultSet["responseData"]["results"][0]["resultSet"]["resultSetHandle"].GetInt();
                exa_rs->setHandle(h);
                this->fetch(exa_rs, 0, 1, FETCH_BYTES);
                close_result_set(*exa_rs);

            } else {// resultset without a handle received
                // add data to exaResultSet and return it. TODO: Dispose the JSON resultSet afterwards.
                this->data = &this->resultSet["responseData"]["results"][0]["resultSet"]["data"];
                append_data_from_Rapid_JSON_Document(exa_rs, *this->data);
            }
        }
    }
    return exa_rs;
}


int64_t
exasockets_connection::fetch(exaResultSetHandler *rs, uint64_t numRows, uint64_t startPosition,
                             uint64_t numBytes) {

    // recursively fetches until the number of rows are all transferred
    // stores the json containing the dataset in d, while resultSet keeps the initial JSON containing the handle.
    // also sets the pointer data to the dataset contained in d.

    // std::cout << "Fetch start from row" << startPosition << std::endl;

    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
    rapidjson::Document d0, d1;  // TODO: remove d0, it is not used.

    uint64_t nr2, nr_here;

    if (numRows == 0) {
        numRows = this->resultSet["responseData"]["results"][0]["resultSet"]["numRows"].GetInt();
    }

    writer.StartObject();
    writer.Key("command");
    writer.String("fetch");
    writer.Key("resultSetHandle");
    writer.Int(rs->getHandle());
    writer.Key("startPosition");
    writer.Int64(startPosition - 1);
    writer.Key("numBytes");
    writer.Int64(numBytes);
    writer.EndObject();


    ws_send_data(s.GetString(), s.GetSize(), 1);
    this->d.Parse(ws_receive_data().c_str());

    if (!this->d.IsObject()) { // nonsense received
        throw std::runtime_error("fetch: Response parsing failed.");
        return -2;
    } else if (this->d.HasMember("exception")) { // DB had a problem
        throw std::runtime_error(this->d["exception"]["text"].GetString());
        return -1;
    } else { // something useful received
        if (this->d.HasMember("responseData")) { // a resultset received
            assert(this->d["responseData"].HasMember("data"));
            this->data = &this->d["responseData"]["data"];

            // append the data to rs
            append_data_from_Rapid_JSON_Document(rs, *this->data);

            nr_here = this->d["responseData"]["numRows"].GetUint64();

            if (nr_here < numRows) { // if not completely fetched...
                nr2 = fetch(rs, numRows - nr_here, startPosition + nr_here,
                            numBytes); // fetch again, overwriting d
                nr_here += nr2;
            }
            return nr_here;
        }
    }
    return -3;
}

exaResultSetHandler *exasockets_connection::create_prepared_insert(char *sql) {
    // creates a prepared statement

    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);

    writer.StartObject();
    writer.Key("command");
    writer.String("createPreparedStatement");
    writer.Key("sqlText");
    writer.String(sql);
    writer.EndObject();

    ws_send_data(s.GetString(), s.GetSize(), 1);
    rapidjson::Document d;
    d.Parse(ws_receive_data().c_str());


    if (!d.IsObject()) { // nonsense received
        throw std::runtime_error("create_prepared: Response parsing failed.");
    } else if (d.HasMember("exception")) { // DB had a problem
        throw std::runtime_error(d["exception"]["text"].GetString());
    } else { // something useful received
        const rapidjson::Value &jrs = d["responseData"]["parameterData"];
        int h = d["responseData"]["statementHandle"].GetInt();
        return this->create_exaResultSetHandler_from_RapidJSON_Document(jrs, h);
    }
}

void exasockets_connection::exec_prepared_insert(exaResultSetHandler &rs) {


    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);

    writer.StartObject();
    writer.Key("command");
    writer.String("executePreparedStatement");
    writer.Key("statementHandle");
    writer.Int(rs.getHandle());
    writer.Key("numColumns");
    writer.Int(rs.cols());
    writer.Key("numRows");
    writer.Int(rs.rows());
    writer.Key("columns");
    writer.StartArray();

    for (int i = 0; i < rs.cols(); i++) { // writing the parameter metadata
        writer.StartObject();
        writer.Key("name");
        writer.String(rs[i].getName());
        writer.Key("dataType");
        writer.StartObject();
        writer.Key("type");
        writer.String(this->ExaDatatypeToString(rs[i].type()));
        if (rs[i].getPrecision() > -1) {
            writer.Key("precision");
            writer.Int(rs[i].getPrecision());
            if (rs[i].getScale() > -1) {
                writer.Key("scale");
                writer.Int(rs[i].getScale());
            }
            if (rs[i].getFraction() > -1) {
                writer.Key("fraction");
                writer.Int(rs[i].getFraction());
            }
        }
        if (rs[i].getSize() > -1) {
            writer.Key("size");
            writer.Int(rs[i].getSize());
        }
        writer.EndObject();
        writer.EndObject();
    }

    writer.EndArray();
    writer.Key("data");

    writer.StartArray(); // column array
    for (int i = 0; i < rs.cols(); i++) { // for each column
        writer.StartArray();

        switch (rs[i].type()) {
            case EXA_BOOLEAN : {

                std::vector<bool> &v = *static_cast<std::vector<bool> *>(rs[i].as_std_vector());
                std::vector<bool> &b = rs[i].nulls;
                for (int j = 0; j < v.size(); j++) {

                    if (!b[j]) {
                        writer.Bool(v[j]);
                    } else {
                        writer.Null();
                    }
                }
                break;
            }
            case EXA_DOUBLE : {

                std::vector<double> &v = *static_cast<std::vector<double> *>(rs[i].as_std_vector());
                std::vector<bool> &b = rs[i].nulls;
                for (int j = 0; j < v.size(); j++) {

                    if (!b[j]) {
                        writer.Double(v[j]);
                    } else {
                        writer.Null();
                    }
                }
                break;
            }
            default : {
                std::vector<std::string> &v = *static_cast<std::vector<std::string> *>(rs[i].as_std_vector());
                std::vector<bool> &b = rs[i].nulls;
                for (int j = 0; j < v.size(); j++) {
                    if (!b[j]) {
                        writer.String(v[j].c_str());
                    } else {
                        writer.Null();
                    }
                }
            }
        }
        writer.EndArray();
    }

    writer.EndArray();
    writer.EndObject();


    ws_send_data(s.GetString(), s.GetSize(), 1);
    rapidjson::Document d;
    d.Parse(ws_receive_data().c_str());


    if (!d.IsObject()) { // nonsense received
        throw std::runtime_error("create_prepared: Response parsing failed.");
    } else if (d.HasMember("exception")) { // DB had a problem
        throw std::runtime_error(d["exception"]["text"].GetString());
    } else { // something useful received
        //std::cout << "ok" << std::endl;
    }
}

int exasockets_connection::close_prepared(exaResultSetHandler &rs) {

    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);

    writer.StartObject();
    writer.Key("command");
    writer.String("closePreparedStatement");
    writer.Key("statementHandle");
    writer.StartArray();
    writer.Int(rs.getHandle());
    writer.EndArray();
    writer.EndObject();

    ws_send_data(s.GetString(), s.GetSize(), 1);
    rapidjson::Document d;
    d.Parse(ws_receive_data().c_str());


    if (!this->d.IsObject()) { // nonsense received
        throw std::runtime_error("close_result_set: Response parsing failed.");
        return -2;
    } else if (this->d.HasMember("exception")) { // DB had a problem
        throw std::runtime_error(this->d["exception"]["text"].GetString());
        return -1;
    } else { // something useful received
        rs.setHandle(rs.getHandle() * (-1));
        return 0;
    }

}

int exasockets_connection::close_result_set(exaResultSetHandler &rs) {

    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);

    writer.StartObject();
    writer.Key("command");
    writer.String("closeResultSet");
    writer.Key("resultSetHandles");
    writer.StartArray();
    writer.Int(rs.getHandle());
    writer.EndArray();
    writer.EndObject();

    ws_send_data(s.GetString(), s.GetSize(), 1);
    rapidjson::Document d;
    d.Parse(ws_receive_data().c_str());


    if (!this->d.IsObject()) { // nonsense received
        throw std::runtime_error("close_result_set: Response parsing failed.");
        return -2;
    } else if (this->d.HasMember("exception")) { // DB had a problem
        throw std::runtime_error(this->d["exception"]["text"].GetString());
        return -1;
    } else { // something useful received
        rs.setHandle(rs.getHandle() * (-1));
        return 0;
    }

}

