#pragma once
#include<iostream>

class hello
{
private:
    /* data */
public:
    hello(/* args */);
    ~hello();
};

hello::hello(/* args */)
{
    std::cout << "nihao";
}

hello::~hello()
{
}
