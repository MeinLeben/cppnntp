#pragma once

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include <boost/regex.hpp>

#include <nntp.hpp>

std::string NNTP_USER = "";
std::string NNTP_PASS = "";
std::string NNTP_PORT = "";
std::string NNTP_HOST = "";
bool        NNTP_SSL = false;

void start();
void help();
bool readconf();
int main();
