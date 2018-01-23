#include <algorithm>
#include "mbed.h"
#include "ESP8266Interface.h"
#include "TCPSocket.h"
#include "greentea-client/test_env.h"
#include "unity/unity.h"
#include "utest.h"

using namespace utest::v1;

#ifndef MBED_CFG_ESP8266_TX
#define MBED_CFG_ESP8266_TX D1
#endif

#ifndef MBED_CFG_ESP8266_RX
#define MBED_CFG_ESP8266_RX D0
#endif

#ifndef MBED_CFG_ESP8266_DEBUG
#define MBED_CFG_ESP8266_DEBUG false
#endif


namespace {
    // Test connection information
    const char *HTTP_SERVER_NAME = "developer.mbed.org";
    const char *HTTP_SERVER_FILE_PATH = "/media/uploads/mbed_official/hello.txt";
    const int HTTP_SERVER_PORT = 80;
#if defined(TARGET_VK_RZ_A1H)
    const int RECV_BUFFER_SIZE = 300;
#else
    const int RECV_BUFFER_SIZE = 512;
#endif
    // Test related data
    const char *HTTP_OK_STR = "200 OK";
    const char *HTTP_HELLO_STR = "Hello world!";

    // Test buffers
    char buffer[RECV_BUFFER_SIZE] = {0};
}

bool find_substring(const char *first, const char *last, const char *s_first, const char *s_last) {
    const char *f = std::search(first, last, s_first, s_last);
    return (f != last);
}

void test_tcp_hello_world() {
    bool result = false;
    ESP8266Interface net(MBED_CFG_ESP8266_TX, MBED_CFG_ESP8266_RX, MBED_CFG_ESP8266_DEBUG);
    net.connect(MBED_CFG_ESP8266_SSID, MBED_CFG_ESP8266_PASS);
    printf("TCP client IP Address is %s\r\n", net.get_ip_address());

    TCPSocket sock(&net);
    printf("HTTP: Connection to %s:%d\r\n", HTTP_SERVER_NAME, HTTP_SERVER_PORT);
    if (sock.connect(HTTP_SERVER_NAME, HTTP_SERVER_PORT) == 0) {
        printf("HTTP: OK\r\n");

        // We are constructing GET command like this:
        // GET http://developer.mbed.org/media/uploads/mbed_official/hello.txt HTTP/1.0\n\n
        strcpy(buffer, "GET http://");
        strcat(buffer, HTTP_SERVER_NAME);
        strcat(buffer, HTTP_SERVER_FILE_PATH);
        strcat(buffer, " HTTP/1.0\n\n");
        // Send GET command
        sock.send(buffer, strlen(buffer));

        // Server will respond with HTTP GET's success code
        const int ret = sock.recv(buffer, sizeof(buffer) - 1);
        buffer[ret] = '\0';

        // Find 200 OK HTTP status in reply
        bool found_200_ok = find_substring(buffer, buffer + ret, HTTP_OK_STR, HTTP_OK_STR + strlen(HTTP_OK_STR));
        // Find "Hello World!" string in reply
        bool found_hello = find_substring(buffer, buffer + ret, HTTP_HELLO_STR, HTTP_HELLO_STR + strlen(HTTP_HELLO_STR));

        TEST_ASSERT_TRUE(found_200_ok);
        TEST_ASSERT_TRUE(found_hello);

        if (found_200_ok && found_hello) result = true;

        printf("HTTP: Received %d chars from server\r\n", ret);
        printf("HTTP: Received 200 OK status ... %s\r\n", found_200_ok ? "[OK]" : "[FAIL]");
        printf("HTTP: Received '%s' status ... %s\r\n", HTTP_HELLO_STR, found_hello ? "[OK]" : "[FAIL]");
        printf("HTTP: Received message:\r\n");
        printf("%s", buffer);
        sock.close();
    } else {
        printf("HTTP: ERROR\r\n");
    }

    net.disconnect();
    TEST_ASSERT_EQUAL(true, result);
}


// Test setup
utest::v1::status_t test_setup(const size_t number_of_cases) {
    char uuid[48] = {0};
    GREENTEA_SETUP_UUID(120, "default_auto", uuid, 48);

    // create mac address based on uuid
    uint64_t mac = 0;
    for (int i = 0; i < sizeof(uuid); i++) {
        mac += uuid[i];
    }
    mbed_set_mac_address((const char*)mac, /*coerce control bits*/ 1);

    return verbose_test_setup_handler(number_of_cases);
}

Case cases[] = {
    Case("TCP hello world", test_tcp_hello_world),
};

Specification specification(test_setup, cases);

int main() {
    return !Harness::run(specification);
}

