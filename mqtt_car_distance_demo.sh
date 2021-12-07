#!/usr/bin/env bash

MQTT_HOST=127.0.0.1
MQTT_PORT=11883

PARKING_SPOT_LENGTH_CM=200


while :
do

        mosquitto_pub -h $MQTT_HOST -p $MQTT_PORT -t parkingassist/spot04/status -m "PARKING_IN"
        for divisor in 0.5 1 2 3 4 5 6 10 20
        do
                distance=$(echo $PARKING_SPOT_LENGTH_CM / $divisor | bc)
                mosquitto_pub -h $MQTT_HOST -p $MQTT_PORT -t parkingassist/spot04/car_distance -m $distance
                sleep 0.5
        done
        mosquitto_pub -h $MQTT_HOST -p $MQTT_PORT -t parkingassist/spot04/status -m "OCCUPIED"

        mosquitto_pub -h $MQTT_HOST -p $MQTT_PORT -t parkingassist/spot08/status -m "PARKING_IN"
        for divisor in 0.5 1 2 3 4 5 6 10 20
        do
                distance=$(echo $PARKING_SPOT_LENGTH_CM / $divisor | bc)
                mosquitto_pub -h $MQTT_HOST -p $MQTT_PORT -t parkingassist/spot08/car_distance -m $distance
                sleep 0.5
        done
        mosquitto_pub -h $MQTT_HOST -p $MQTT_PORT -t parkingassist/spot08/status -m "OCCUPIED"

        sleep 1

        mosquitto_pub -h $MQTT_HOST -p $MQTT_PORT -t parkingassist/spot04/status -m "PARKING_OUT"
        for divisor in 20 10 6 5 4 3 2 1 0.5
        do
                distance=$(echo $PARKING_SPOT_LENGTH_CM / $divisor | bc)
                mosquitto_pub -h $MQTT_HOST -p $MQTT_PORT -t parkingassist/spot04/car_distance -m $distance
                sleep 0.5
        done
        mosquitto_pub -h $MQTT_HOST -p $MQTT_PORT -t parkingassist/spot04/status -m "FREE"

        sleep 3

        mosquitto_pub -h $MQTT_HOST -p $MQTT_PORT -t parkingassist/spot08/status -m "PARKING_OUT"
        for divisor in 20 10  6 5 4 3 2 1 0.5
        do
                distance=$(echo $PARKING_SPOT_LENGTH_CM / $divisor | bc)
                mosquitto_pub -h $MQTT_HOST -p $MQTT_PORT -t parkingassist/spot08/car_distance -m $distance
                sleep 0.5
        done
        mosquitto_pub -h $MQTT_HOST -p $MQTT_PORT -t parkingassist/spot08/status -m "FREE"

        sleep 4

done
