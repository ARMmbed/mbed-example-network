#include "mbed.h"
#include "test_env.h"
#include "EthernetInterface.h"
#include "HTTPClient.h"

#define BUFFER_SIZE 512

int main()
{
    char http_request_buffer[BUFFER_SIZE+1] = {0};
    HTTPClient http;
    EthernetInterface eth;
    eth.init(); //Use DHCP
    eth.connect();

    //GET data
    {
        bool result = true;
        const char *url_hello_txt = "http://mbed.org/media/uploads/donatien/hello.txt";
        printf("HTTP_GET: Trying to fetch page '%s'...\r\n", url_hello_txt);
        const int ret = http.get(url_hello_txt, http_request_buffer, BUFFER_SIZE);
        if (ret == 0)
        {
          printf("HTTP_GET: Read %d chars: '%s' ... [OK]\r\n", strlen(http_request_buffer), http_request_buffer);
        }
        else
        {
          printf("HTTP_GET: Error(%d). HTTP error(%d) ... [FAIL]\r\n", ret, http.getHTTPResponseCode());
          result = false;
        }

        if (result == false) {
            notify_completion(false);
            exit(ret);
        }
    }

    //POST data
    {
        bool result = true;
        const char *url_httpbin_post = "http://httpbin.org/post";
        HTTPMap map;
        HTTPText text(http_request_buffer, BUFFER_SIZE);
        map.put("Hello", "World");
        map.put("test", "1234");
        printf("HTTP_POST: Trying to post data to '%s' ...\r\n", url_httpbin_post);
        const int ret = http.post(url_httpbin_post, map, &text);
        if (ret == 0) {
          printf("HTTP_POST: Read %d chars ... [OK]\n", strlen(http_request_buffer));
          printf("HTTP_POST: %s\r\n", http_request_buffer);
        }
        else {
          printf("HTTP_GET: Error(%d). HTTP error(%d) ... [FAIL]\r\n", ret, http.getHTTPResponseCode());
          result = false;
        }

        if (result == false) {
            notify_completion(false);
            exit(ret);
        }
    }
    eth.disconnect();
    notify_completion(true);
    return 0;
}
