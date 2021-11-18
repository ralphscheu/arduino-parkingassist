let parking_spots_data = [
    { "spot_id": "01", "car_distance": 35, "status": "EINPARKEN" },
    { "spot_id": "02", "car_distance": 20, "status": "BELEGT" },
    { "spot_id": "03", "car_distance": 199, "status": "FREI" },
]

// Called after form input is processed
function startConnect(host, port) {
    // Generate a random client ID
    clientID = "clientID-" + parseInt(Math.random() * 100);

    // Fetch the hostname/IP address and port number from the form
    // host = document.getElementById("host").value;
    // port = document.getElementById("port").value;

    // Print output for the user in the messages div
    // document.getElementById("messages").innerHTML += '<span>Connecting to: ' + host + ' on port: ' + port + '</span><br/>';
    // document.getElementById("messages").innerHTML += '<span>Using the following client value: ' + clientID + '</span><br/>';

    // Initialize new Paho client connection
    client = new Paho.MQTT.Client(host, Number(port), clientID);

    // Set callback handlers
    client.onConnectionLost = onConnectionLost;
    client.onMessageArrived = onMessageArrived;

    // Connect the client, if successful, call onConnect function
    client.connect({ 
        onSuccess: onConnect,
    });
}

// Called when the client connects
function onConnect() {
    // Subscribe to the wildcard topic
    client.subscribe("parkingassist/#");
}

// Called when the client loses its connection
function onConnectionLost(responseObject) {
    document.getElementById("messages").innerHTML += '<span>ERROR: Connection lost</span><br/>';
    if (responseObject.errorCode !== 0) {
        document.getElementById("messages").innerHTML += '<span>ERROR: ' + + responseObject.errorMessage + '</span><br/>';
    }
}

// Called when a message arrives
function onMessageArrived(message) {
    const spot_id = message.destinationName.split('/')[1];
    const measurement = message.destinationName.split('/')[2];

    const car_distance = parseInt( message.payloadString.split('.')[0] );
    
    
    spot_arr_index = parseInt( spot_id.substring(4)) - 1;
    parking_spots_data[spot_arr_index]['car_distance'] = car_distance;
}

// Called when the disconnection button is pressed
function startDisconnect() {
    client.disconnect();
    document.getElementById("messages").innerHTML += '<span>Disconnected</span><br/>';
}

function updateDisplay() {
    // console.log(parking_spots_data);
    let count_free_parking_spots = 0;

    for (i=1; i<=3; i++) {
        const spot_data = parking_spots_data[i-1];
        // console.log(spot_data);

        // console.log("spot0" + i + "_" + "car_distance");
        // console.log(parking_spots_data[i-1]['car_distance']);

        // set box color according to current status
        document.getElementById("spot0" + i).className = "spot " + spot_data['status'];

        // show distance in cm if status is EINPARKEN
        
        if (spot_data['status'] === 'EINPARKEN') {
            document.getElementById("spot0" + i + "_car_distance").innerText = '⬌ ' + spot_data['car_distance'] + 'cm';
        }



        // increment free parking spot counter if this spot is free
        if (spot_data['status'] === 'FREI')
        {
            count_free_parking_spots = count_free_parking_spots + 1;
            document.getElementById("spot0" + i + "_car_distance").innerText = 'FREI';
        }
        
        if (spot_data['status'] === 'BELEGT') {
            document.getElementById("spot0" + i + "_car_distance").innerText = 'BELEGT';
        }
    }

    document.getElementById('num_free_parking_spots').innerText = count_free_parking_spots;
}




/////////////////////////////////////////////////////////////

window.onload = function() {
    startConnect("192.168.0.119", 9001);

    const interval = setInterval(function() {
        updateDisplay();
      }, 500);
};



