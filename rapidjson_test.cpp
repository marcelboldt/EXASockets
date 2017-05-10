//
// Created by marcel on 09.05.17.
//

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include<vector>
#include <iostream>
#include <chrono>


int main(int argc, char **argv) {

    std::vector<int> ints_A(10000, 1111111);
    std::vector<int> ints_B(10000, 222220);
    std::vector<int> ints_C(10000, 333333330);
    std::vector<int> ints_D(10000, 4444440);

    std::vector<std::string> str_A(10000, "my_string_A");
    std::vector<std::string> str_B(10000, "my_string_B");
    std::vector<std::string> str_C(10000, "my_string_C");
    std::vector<std::string> str_D(10000, "my_string_D_blablablablablablablablablablablablablablablablabla");

    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);


    writer.StartObject();
    writer.Key("Int_A");
    writer.StartArray();
    for (int i = 0; i < ints_A.size(); i++) writer.Uint(ints_A[i]);
    writer.EndArray();
    writer.Key("Int_B");
    writer.StartArray();
    for (int i = 0; i < ints_B.size(); i++) writer.Uint(ints_B[i]);
    writer.EndArray();
    writer.Key("Int_C");
    writer.StartArray();
    for (int i = 0; i < ints_C.size(); i++) writer.Uint(ints_C[i]);
    writer.EndArray();
    writer.Key("Int_D");
    writer.StartArray();
    for (int i = 0; i < ints_D.size(); i++) writer.Uint(ints_D[i]);
    writer.EndArray();

    writer.Key("Str_A");
    writer.StartArray();
    for (int i = 0; i < str_A.size(); i++) writer.String(str_A[i].c_str());
    writer.EndArray();

    writer.Key("Str_B");
    writer.StartArray();
    for (int i = 0; i < str_B.size(); i++) writer.String(str_B[i].c_str());
    writer.EndArray();

    writer.Key("Str_C");
    writer.StartArray();
    for (int i = 0; i < str_C.size(); i++) writer.String(str_C[i].c_str());
    writer.EndArray();
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    writer.Key("Str_D");
    writer.StartArray();
    for (int i = 0; i < str_D.size(); i++) writer.String(str_D[i].c_str());
    writer.EndArray();
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    writer.EndObject();


    std::cout << "JSON build duration " << " : "
              << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000
              << " ms." << std::endl;

}
