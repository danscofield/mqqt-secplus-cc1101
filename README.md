# mqqt-secplus-cc1101: A project for transmitting Security+ 1.0 signals via a CC1101 on ESP32-S3 devices (with an AWS IoT MQTT integration for good measure).

This is the microcontroller portion of a larger IoT-enabled garage door project I recently completed. The code connects to your local WiFi network, connects to an MQTT endpoint, and subscribes to a garage door opener topic. When messages are receieved by the device, it transmits the supplied fixed and rolling codes via a CC1101 radio. As a result, hopefully your garage door opens. I have only tested this with AWS IoT core MQTT, but ostensibly it should work with any mTLS MQTT server. I'm using a QT Py ESP32-S3 from Adafruit and a 8-pin CC1101 breakout board from ebyte that I got on eBay.

***Note:*** As written this code relies on the MQQT server to supply a fixed and rolling code. This means that you will need extra infrastructure to keep track of the fixed code and the previous rolling code and to push values of the rolling code that are greater than the previously observed value. 

In my setup I trigger the garage door opener with an AWS Lambda function that writes to an AWS IoT MQQT endpont. The rolling codes are stored in DynamoDB and I basically use an atomic counter like [this](https://docs.aws.amazon.com/amazondynamodb/latest/developerguide/WorkingWithItems.html) to ensure the rolling code is always increasing. My approach is pretty heavy handed, but garage door rolling code security is pretty terrible and I wanted to add some other features. So, in my setup, I plug my garage door opener into a TP-Link smart plug. My lambda uses [this](https://github.com/piekstra/tplink-cloud-api) library to power on my garage door opener, waits a few seconds, grabs the new rolling code from dynamo and then publishes the MQTT message. I've got a second lambda that periodically polls the plug's power status and powers it off if it's been on for more than ten minutes. I've got a wider AWS setup for invoking that lambda from an cognito-authenticated Vue SPA via API Gateway. 

## Usage


1. Open `garagedooropener.ino` in the Arduino IDE
2. Make sure that you have the following libraries installed:
    1. [SmartRC-CC1101-Driver-Lib](https://github.com/LSatan/SmartRC-CC1101-Driver-Lib)
    2. [ArduinoMqttClient](https://github.com/arduino-libraries/ArduinoMqttClient)
    3. [ArduinoJson](https://arduinojson.org/?utm_source=meta&utm_medium=library.properties)
3. Hard code `WIFI_SSID` and `WIFI_PASSWORD` in `secrets.h`.
4. Add CA, public and private keys to `secrets.h` to allow mTLS.
    1. For AWS IoT core this can be done in the AWS IoT core console by clicking on "Manage" then "Things", then downloading Certificates for your "thing".  
4. In `constants.h`, configure your SPI pins, MQTT endpoint information, and queue names:
    1. `AWS_IOT_SUBSCRIBE_TOPIC` this is the topic we subscribe to to listen for requests to transmit a code.
    2. `AWS_IOT_PUBLISH_TOPIC` is the topic confirmation messages are published to (i.e., acknowledgements of transmission requests).
    3. `AWS_IOT_HEALTHCHECK_TOPIC` is the topic periodic heart beats are published to. 
    4. `HEALTHCHECK_DEVICE_NAME` is the device name to transmit in the heart beat messages.
    5. The `pCSN, pMOSI, pSCK, pMISO, pGDO0` constants are just the GPIO pin numbers for the SPI pins connected to the CC1101.
5. Compile and upload the project

***How do I know if it's working?*** Your ESP32 should output some basic debugging information via the Serial port. After it's connected to WiFi and MQQT, you should be able to subscribe to the MQTT server's `AWS_IOT_HEALTHCHECK_TOPIC` and see healtcheck mesages every few seconds.

## Opening the garage door

Messages to the `AWS_IOT_SUBSCRIBE_TOPIC` queue are expected to be in the below JSON format:

```
{"request_id": string, "fixed": int, "rolling": int, "version": 1}
```

To open the garage door, you just need to send a message of the above format to the `AWS_IOT_SUBSCRIBE_TOPIC` topic. There are some size restrictions on values for `fixed` and `rolling` described [here](https://github.com/argilo/secplus). 

To train your gargage door opener, you can usually just push the physical learn button on the opener, and send the above message. Once trained, you should be able to send the previous `rolling+3` and have things open.

***What if I want to clone an existing opener?*** This is probably not the greatest idea because eventually your original opener and this one will grow out of sync. But if you want to do it, all you need to do is use an SDR (e.g., an RTL-SDR) and rtl-433 to obtain the fixed code and current rolling code value for your existing opener. Then you can just transmit that same fixed code, and a bigger rolling code. 



## Credits

Big thanks to [Clayton Smith](https://github.com/argilo/) for reverse engineering the Security+ protocol and building an excellent [reference implementation](https://github.com/argilo/secplus/). Equally many thanks to [LSatan](https://github.com/LSatan/) for making it easy to use the CC101 on microcontoller projects via this [excellent library](https://github.com/LSatan/SmartRC-CC1101-Driver-Lib).
