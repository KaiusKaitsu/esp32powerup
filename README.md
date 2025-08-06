# ESP32 powerup

Software for ESP32 Wroom microcontroller to wake up my computer via a webinterface.
Some basic security measures are SSL & HTTP authentication. Don't try pentesting ty. 
Use Espressif esp-idf. 

### Contents of secrets.h
Your secrets.h should look something like this
```
#pragma once

#define WIFI_SSID "FREEWIFI"
#define WIFI_PASS "ANYWHEREYOUGO"

// Login for username: yourusername 
// and password: yourpassword
#define AUTH_HEADER "Basic eW91cnVzZXJuYW1lOnlvdXJwYXNzd29yZA=="


extern const uint8_t _binary_esp32_cert_pem_start[];
extern const uint8_t _binary_esp32_cert_pem_end[];
extern const uint8_t _binary_esp32_key_pem_start[];
extern const uint8_t _binary_esp32_key_pem_end[];
```
### Creating AUTH_HEADER
I'm lazy and base 64 works fine. In terminal run
```
echo -n 'yourusername:yourpassword' | base64
eW91cnVzZXJuYW1lOnlvdXJwYXNzd29yZA==
```
Remember to change 'yourusername:yourpassword' to something

### Creating SSL self signed certificates
To create certificates use the below command in terminal. 
```
openssl req -x509 -newkey rsa:2048 -keyout esp32_key.pem -out esp32_key.pem -days 365 -nodes
```
Make sure the key and cert are in main folder

```
main
├── CMakeLists.txt
├── esp32_cert.pem
├── esp32_key.pem
├── main.c
└── secrets.h
```