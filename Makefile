CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra
LDFLAGS = -lssl -lcrypto -lpthread
TEST_LDFLAGS = -lUnitTest++ -lssl -lcrypto -lpthread

SOURCES = main.cpp server.cpp
TARGET = server
TEST_SOURCE = test.cpp
TEST_TARGET = test_server

# Сборка основного сервера
$(TARGET): $(SOURCES) server.h
	$(CXX) $(SOURCES) -o $(TARGET) $(CXXFLAGS) $(LDFLAGS)

# Сборка тестов с UnitTest++ и созданием тестовых файлов
$(TEST_TARGET): $(TEST_SOURCE) server.cpp server.h
	@echo "Создание тестовых файлов..."
	@echo "testuser:testpass123" > test_auth_db.txt
	@echo "alice:password456" >> test_auth_db.txt
	@echo "bob:secret789" >> test_auth_db.txt
	@touch empty_test.txt
	@echo "user1pass1" > invalid_format.txt
	@echo "user2:pass2" >> invalid_format.txt
	@echo ":pass3" >> invalid_format.txt
	@echo "user4:" >> invalid_format.txt
	@echo "user5:pass5" >> invalid_format.txt
	$(CXX) $(TEST_SOURCE) server.cpp -o $(TEST_TARGET) $(CXXFLAGS) $(TEST_LDFLAGS)

# Запуск тестов
test: $(TEST_TARGET)
	@echo "=== ЗАПУСК МОДУЛЬНЫХ ТЕСТОВ С UNITTEST++ ==="
	./$(TEST_TARGET)

# Очистка
clean:
	rm -f $(TARGET) $(TEST_TARGET)
	rm -f test_auth_db.txt empty_test.txt invalid_format.txt

.PHONY: clean test