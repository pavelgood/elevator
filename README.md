# Elevator

Simple lift simulator.

Build:

    mkdir Release
    cd Release
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make


Usage: 

    ./elevator STOREY_COUNT HIHG SPEED INTERVAL ELEVATOR_COUNT

		STOREY_COUNT      number of storeys in the buiding (5 - 20),
		HIGHT             storey's hight (in meters, 10 maximum),
		SPEED             elevator's speed (m/sec, 10 maximum),
		INTERVAL          opening/closing time interval (sec, 20 maximum),
		ELEVATOR_COUNT    number of elevators in the building (8 maximum)

