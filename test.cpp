#include <UnitTest++/UnitTest++.h>
#include "server.h"
#include <fstream>
#include <cstdio>
#include <cstdint>
#include <vector>

using namespace std;

// ==================== ФИКСТУРЫ ДЛЯ ТЕСТИРОВАНИЯ ФАЙЛОВ ====================

struct TestAuthDB {
    string filename;
    
    TestAuthDB() : filename("test_auth_db.txt") {
        ofstream file(filename);
        file << "testuser:testpass123\n";
        file << "alice:password456\n";
        file << "bob:secret789\n";
        file.close();
    }
    
    ~TestAuthDB() {
        remove(filename.c_str());
    }
};

struct EmptyFile {
    string filename;
    
    EmptyFile() : filename("empty_test.txt") {
        ofstream file(filename);
        file.close();
    }
    
    ~EmptyFile() {
        remove(filename.c_str());
    }
};

struct InvalidFormatFile {
    string filename;
    
    InvalidFormatFile() : filename("invalid_format.txt") {
        ofstream file(filename);
        file << "user1pass1\n";
        file << "user2:pass2\n";
        file << ":pass3\n";
        file << "user4:\n";
        file << "user5:pass5\n";
        file.close();
    }
    
    ~InvalidFormatFile() {
        remove(filename.c_str());
    }
};

// ==================== ТЕСТИРОВАНИЕ ВЫЧИСЛИТЕЛЬНОГО МОДУЛЯ ====================

SUITE(CalculationTest)
{
    TEST(CalculatePositive) {
        Server server(33333, "./vcalc.conf", "./log/vcalc.log");  // ИСПРАВЛЕНО: правильные имена по умолчанию
        vector<int16_t> vector = {2, 3, 4};
        int16_t result = server.calculateProduct(vector);
        CHECK_EQUAL(24, result);
    }
    
    TEST(CalculateNegative) {
        Server server(33333, "./vcalc.conf", "./log/vcalc.log");
        vector<int16_t> vector = {-2, 3, -4};
        int16_t result = server.calculateProduct(vector);
        CHECK_EQUAL(24, result);
    }
    
    TEST(CalculateSingle) {
        Server server(33333, "./vcalc.conf", "./log/vcalc.log");
        vector<int16_t> vector = {42};
        int16_t result = server.calculateProduct(vector);
        CHECK_EQUAL(42, result);
    }
    
    TEST(CalculateEmpty) {
        Server server(33333, "./vcalc.conf", "./log/vcalc.log");
        vector<int16_t> vector;
        int16_t result = server.calculateProduct(vector);
        CHECK_EQUAL(0, result);
    }
    
    TEST(CalculateWithZero) {
        Server server(33333, "./vcalc.conf", "./log/vcalc.log");
        vector<int16_t> vector = {1, 0, 5};
        int16_t result = server.calculateProduct(vector);
        CHECK_EQUAL(0, result);
    }
    
    TEST(CalculateOverflowPositive) {
        Server server(33333, "./vcalc.conf", "./log/vcalc.log");
        vector<int16_t> vector = {200, 200};
        int16_t result = server.calculateProduct(vector);
        CHECK_EQUAL(32767, result);
    }
    
    TEST(CalculateOverflowNegative) {
        Server server(33333, "./vcalc.conf", "./log/vcalc.log");
        vector<int16_t> vector = {-200, 200};
        int16_t result = server.calculateProduct(vector);
        CHECK_EQUAL(-32768, result);
    }
}

// ==================== ТЕСТИРОВАНИЕ ФУНКЦИЙ АУТЕНТИФИКАЦИИ ====================

SUITE(AuthenticationTest)
{
    TEST(MD5ValidInput) {
        Server server(33333, "./vcalc.conf", "./log/vcalc.log");
        string input = "test123";
        string result = server.md5Hash(input);
        CHECK_EQUAL("CC03E747A6AFBBCBF8BE7668ACFEBEE5", result);
    }
    
    TEST(MD5EmptyInput) {
        Server server(33333, "./vcalc.conf", "./log/vcalc.log");
        string input = "";
        string result = server.md5Hash(input);
        CHECK_EQUAL("D41D8CD98F00B204E9800998ECF8427E", result);
    }
    
    TEST(MD5FixedInput) {
        Server server(33333, "./vcalc.conf", "./log/vcalc.log");
        string input = "hello";
        string result = server.md5Hash(input);
        CHECK_EQUAL("5D41402ABC4B2A76B9719D911017C592", result);
    }
    
    TEST(HashVerification) {
        Server server(33333, "./vcalc.conf", "./log/vcalc.log");
        string testString = "hello world";
        string hash1 = server.md5Hash(testString);
        string hash2 = server.md5Hash(testString);
        CHECK_EQUAL(hash1, hash2);
    }
}

// ==================== ТЕСТИРОВАНИЕ РАБОТЫ С ФАЙЛАМИ ====================

SUITE(FileOperationsTest)
{
    TEST_FIXTURE(TestAuthDB, LoadValidFile) {
        Server server(33333, filename, "./log/vcalc.log");
        server.loadUserDatabase();
        CHECK_EQUAL(3, server.getUsersCount());
    }
    
    TEST_FIXTURE(EmptyFile, LoadEmptyFile) {
        Server server(33333, filename, "./log/vcalc.log");
        server.loadUserDatabase();
        CHECK_EQUAL(0, server.getUsersCount());
    }
    
    TEST_FIXTURE(InvalidFormatFile, LoadInvalidFormat) {
        Server server(33333, filename, "./log/vcalc.log");
        server.loadUserDatabase();
        CHECK_EQUAL(2, server.getUsersCount());
    }
    
    TEST(LoadNonExistentFile) {
        Server server(33333, "nonexistent.db", "./log/vcalc.log");
        server.loadUserDatabase();
        CHECK_EQUAL(0, server.getUsersCount());
    }
    
}

// ==================== ТЕСТИРОВАНИЕ СЕРВЕРНЫХ МОДУЛЕЙ ====================

SUITE(ServerModulesTest)
{
    TEST(ServerConstructor) {
        Server server1(33333, "./vcalc.conf", "./log/vcalc.log");
        Server server2(44444, "./vcalc.conf", "./log/vcalc.log");
        Server server3(55555, "./vcalc.conf", "./log/vcalc.log");
        CHECK(true);
    }
    
    TEST_FIXTURE(TestAuthDB, UserManagement) {
        Server server(33333, filename, "./log/vcalc.log");
        server.loadUserDatabase();
        size_t userCount = server.getUsersCount();
        CHECK(userCount > 0);
    }
}

int main()
{
    return UnitTest::RunAllTests();
}