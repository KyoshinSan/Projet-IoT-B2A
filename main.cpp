/*
 * Copyright (c) 2017, CATIE, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "mbed.h"
#include "zest-radio-atzbrf233.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"

static DigitalOut led1 (LED1);

I2C i2c(I2C1_SDA, I2C1_SCL);
uint8_t lm75_adress = 0x48 << 1;

AnalogIn   capteurHumidity(ADC_IN1);

//function getTemperature()
float getTemperature() {
	char cmd[2];
    cmd[0] = 0x00; // adresse registre temperature
   	i2c.write(lm75_adress, cmd, 1);
   	i2c.read(lm75_adress, cmd, 2);

   	float temperature = ((cmd[0] << 8 | cmd[1] ) >> 7) * 0.5;
   	return temperature;
}

float getHumidity() {

	// mesure du capteur dans l'eau
	float humidity = capteurHumidity.read();
	return humidity;

	//appliqué la formule
	// float measure_percent = (measure - air_value) * 100.0 / (water_value - air_value);
}

void flip_led() {
	led1 !=led1;
}

// Network interface
NetworkInterface *net;

int arrivedcount = 0;
const char* topic = "TheoJonathan/feeds/projet-iot";

/* Printf the message received and its configuration */

void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    if (strcmp((const char *)md.message.payload, "ON") == 0) {
    	led1 = 1;
    	printf("ON\n");
    } else if (strcmp((const char *)md.message.payload, "OFF") == 0) {
    	led1 = 0;
    	printf("OFF\n");
    }
    printf("Message arrived: qos %d, retained %d, dup %d, packetid %d\r\n", message.qos, message.retained, message.dup, message.id);
    printf("Payload %.*s\r\n", message.payloadlen, (char*)message.payload);
    ++arrivedcount;
}

// MQTT demo
int main() {
	int result;

    // Add the border router DNS to the DNS table
    nsapi_addr_t new_dns = {
        NSAPI_IPv6,
        { 0xfd, 0x9f, 0x59, 0x0a, 0xb1, 0x58, 0, 0, 0, 0, 0, 0, 0, 0, 0x00, 0x01 }
    };
    nsapi_dns_add_server(new_dns);

    printf("Starting MQTT demo\n");

    // Get default Network interface (6LowPAN)
    net = NetworkInterface::get_default_instance();
    if (!net) {
        printf("Error! No network inteface found.\n");
        return 0;
    }

    // Connect 6LowPAN interface
    result = net->connect();
    if (result != 0) {
        printf("Error! net->connect() returned: %d\n", result);
        return result;
    }

    // Build the socket that will be used for MQTT
    MQTTNetwork mqttNetwork(net);

    // Declare a MQTT Client
    MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);

    // Connect the socket to the MQTT Broker
    const char* hostname = "io.adafruit.com";
    uint16_t port = 1883;
    printf("Connecting to %s:%d\r\n", hostname, port);
    int rc = mqttNetwork.connect(hostname, port);
    if (rc != 0)
        printf("rc from TCP connect is %d\r\n", rc);

    // Connect the MQTT Client
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = "mbed-sample";
    data.username.cstring = "TheoJonathan";
    data.password.cstring = "f9382dde1e4f4c20b219f70fce53ae1f";
    if ((rc = client.connect(data)) != 0)
        printf("rc from MQTT connect is %d\r\n", rc);

    // Subscribe to the same topic we will publish in
    if ((rc = client.subscribe(topic, MQTT::QOS2, messageArrived)) != 0)
        printf("rc from MQTT subscribe is %d\r\n", rc);

    // Send a message with QoS 0
    MQTT::Message message;

    // QoS 0
    char buf[100];
    //sprintf(buf, "La Commune de France ! Temperature : %f, Humidite : %f \r\n", getTemperature(), getHumidity());
    sprintf(buf, "%f\r\n", getTemperature());

    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf)+1;

    rc = client.publish("TheoJonathan/feeds/temperature", message);

    sprintf(buf, "%f\r\n", getHumidity());

    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf)+1;

    rc = client.publish("TheoJonathan/feeds/humidite", message);


    // yield function is used to refresh the connexion
    // Here we yield until we receive the message we sent
    while (true) {
        client.yield(100);
        wait_ms(1000);
    }

    // Disconnect client and socket
    client.disconnect();
    mqttNetwork.disconnect();

    // Bring down the 6LowPAN interface
    net->disconnect();
    printf("Done\n");
}
