//
// Created by 牛志强 on 16/5/18.
//

#include <gtest/gtest.h>
#include "../url.h"

TEST(URLTest, ParseNormalUrl){
    std::string url = "http://www.google.com/reader/";
    zq::URL u;

    ASSERT_TRUE(u.parse(url.c_str()));

    EXPECT_EQ("http", u.get_schema());
    EXPECT_EQ("www.google.com", u.get_host());
    EXPECT_EQ("/reader/", u.get_path());
    EXPECT_EQ(80, u.get_port());
    EXPECT_EQ("", u.get_query());
    EXPECT_EQ("", u.get_fragment());
}

TEST(URLTest, ParseWrongUrl){
    std::string url = "https:///www.google.com/&wrong/reader/";
    zq::URL u;

    ASSERT_FALSE(u.parse(url.c_str()));
}

TEST(URLTest, ParseFullUrl){
    std::string url = "http://test:pass@www.google.com:6666/reader/p?key1=value1#frag";
    zq::URL u;

    ASSERT_TRUE(u.parse(url.c_str()));

    EXPECT_EQ("http", u.get_schema());
    EXPECT_EQ("www.google.com", u.get_host());
    EXPECT_EQ("/reader/p", u.get_path());
    EXPECT_EQ(6666, u.get_port());
    EXPECT_EQ("key1=value1", u.get_query());
    EXPECT_EQ("frag", u.get_fragment());
    EXPECT_EQ("test", u.get_username());
    EXPECT_EQ("pass", u.get_password());
}

TEST(URLTest, ParseMultiQuery){
    std::string url = "http://www.google.com/reader?key1=value1&key2=value2";
    zq::URL u;

    ASSERT_TRUE(u.parse(url.c_str()));

    EXPECT_EQ("http", u.get_schema());
    EXPECT_EQ("www.google.com", u.get_host());
    EXPECT_EQ("/reader", u.get_path());
    EXPECT_EQ(80, u.get_port());
    EXPECT_EQ("key1=value1&key2=value2", u.get_query());
    EXPECT_EQ("", u.get_fragment());
}

TEST(URLTest, ParseWrongPort){
    std::string url = "http://www.google.com:ab34/";
    zq::URL u;

    ASSERT_FALSE(u.parse(url.c_str()));
}

TEST(URLTest, ParseUserInfo){
    std::string url = "http://test:pass@www.google.com";
    zq::URL u;

    ASSERT_TRUE(u.parse(url.c_str()));

    EXPECT_EQ("test", u.get_username());
    EXPECT_EQ("pass", u.get_password());
}

TEST(URLTest, ParseLocalFileScheme){
    std::string url = "file:///Users/test/t.cpp";
    zq::URL u;

    ASSERT_FALSE(u.parse(url.c_str()));
}

TEST(URLTest, ParseRemoteFileScheme){
    std::string url = "file://root@www.google.com/Users/test/t.cpp";
    zq::URL u;

    ASSERT_TRUE(u.parse(url.c_str()));

    EXPECT_EQ("file", u.get_schema());
    EXPECT_EQ("/Users/test/t.cpp", u.get_path());
    EXPECT_EQ("root", u.get_username());
    EXPECT_EQ("www.google.com", u.get_host());
    EXPECT_EQ(80, u.get_port());
}

TEST(URLTest, DumpNormalUrl){
    zq::URL u;
    u.set_schema("http");
    u.set_host("www.google.com");
    u.set_port(4567);
    u.set_path("/reader");
    u.set_query("key1=value1&key2=value2");
    u.set_fragment("frag");

    std::string url = "http://www.google.com:4567/reader?key1=value1&key2=value2#frag";
    EXPECT_EQ(url, u.dump());
}