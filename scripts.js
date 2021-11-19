///////////////
// CONSTANTS //
///////////////

// Some translation strings for the UI
const LOCALIZATIONSTRINGS = {
    'FREE': 'Frei',
    'RESERVED': 'Reserviert',
    'OCCUPIED': 'Belegt',
    'PARKING_IN': 'Parkt ein',
    'PARKING_OUT': 'Parkt aus',
}

// Timespan after which unused reservation will expire
const RESERVATIONS_EXPIRATION_MINUTES = 0.25;

// Total number of available parking spots
const NUM_PARKING_SPOTS = 9;

// Length of parking spots in cm. Set to a reasonable value for demonstration purposes.
const PARKING_SPOT_LENGTH_CM = 100;

// Interval (in ms) between updates to the GUI
const UPDATE_LOOP_INTERVAL_MS = 50;



///////////////
// VARIABLES //
///////////////

// This will hold the current parking data based on the most recent corresponding MQTT messages
// We initialize some default data so we don't run into null errors while waiting for the first messages.
let parking_spots_data = [
    { "car_distance": 999,  "status": "FREE", "prev_status": "FREE", "reservation_expires_at": null },
    { "car_distance": 999,  "status": "FREE", "prev_status": "FREE", "reservation_expires_at": null },
    { "car_distance": 999, "status": "FREE", "prev_status": "FREE", "reservation_expires_at": null },
    { "car_distance": 999, "status": "FREE", "prev_status": "FREE", "reservation_expires_at": null },
    { "car_distance": 12, "status": "OCCUPIED", "prev_status": "OCCUPIED", "reservation_expires_at": null },
    { "car_distance": 14, "status": "OCCUPIED", "prev_status": "OCCUPIED", "reservation_expires_at": null },
    { "car_distance": 11, "status": "OCCUPIED", "prev_status": "OCCUPIED", "reservation_expires_at": null },
    { "car_distance": 999, "status": "FREE", "prev_status": "FREE", "reservation_expires_at": null },
    { "car_distance": 999, "status": "FREE", "prev_status": "FREE", "reservation_expires_at": null },
]


///////////////////////////////
// MQTT CONNECTION FUNCTIONS //
///////////////////////////////

// Called after page finished loading
// Establish connection to MQTT broker using paho-mqtt (https://www.eclipse.org/paho/clients/js/)
function startConnect(host, port) {
    // Generate a random client ID
    clientID = "clientID-" + parseInt(Math.random() * 100);

    console.log('Connecting to: ' + host + ' on port: ' + port);
    console.log('Using clientID = ' + clientID);

    // Initialize new client connection
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
    console.log('ERROR: Connection lost');
    if (responseObject.errorCode !== 0) {
        console.log('ERROR: ' + + responseObject.errorMessage);
    }
}


///////////////////////
// APPLICATION LOGIC //
///////////////////////

// Called when a message arrives
function onMessageArrived(message) {

    // Topics look like parkingassist/spot03/status
    // We extract the middle and the last part of the topic "path"
    const spot_id = message.destinationName.split('/')[1];
    const measurement = message.destinationName.split('/')[2];

    // Compute zero-based parking spot index for accessing the parking_spots_data array
    // e.g. spot02 -> 1
    spot_arr_index = parseInt( spot_id.substring(4)) - 1;

    // Handle the payload depending on what kind of value is being received
    if (measurement === 'car_distance') {
        const car_distance = parseInt( message.payloadString.split('.')[0] );  // dirty way to make sure the float value is safely converted to int
        parking_spots_data[spot_arr_index]['car_distance'] = car_distance;
    }
    else if (measurement === 'status') {
        parking_spots_data[spot_arr_index]['status'] = message.payloadString;
    }
    else if (measurement === 'reservation_expires_at') {
        parking_spots_data[spot_arr_index]['reservation_expires_at'] = message.payloadString;
    }
}

// Submit a reservation by publishing the RESERVED status and the future timestamp at which the reservation will expire
function makeReservation(spot_id) {
    message = new Paho.MQTT.Message("RESERVED");
    message.destinationName = "parkingassist/" + spot_id + "/status";
    client.send(message);

    // calculate UNIX timestamp at which the reservation will expire
    reservation_expires_at = new Date().getTime() + RESERVATIONS_EXPIRATION_MINUTES * 60000;

    message = new Paho.MQTT.Message(reservation_expires_at.toString());
    message.destinationName = "parkingassist/" + spot_id + "/reservation_expires_at";
    client.send(message);
}

// Cancel a reservation by setting the status back to FREE.
// This is called in the update loop after a reservation has expired.
function cancelReservation(spot_id) {
    message = new Paho.MQTT.Message("FREE");
    message.destinationName = "parkingassist/" + spot_id + "/status";
    client.send(message);
}

// Main update loop
function update() {
    // init free parking space counter for message at the top.
    let count_free_parking_spots = 0;

    // loop through all parking spots
    for (i=1; i<=NUM_PARKING_SPOTS; i++) {
        const spot_data = parking_spots_data[i-1];  // fetch currently saved info about this parking spot

        const spot_id_verbose = "spot0" + i;  // converts 1 to spot01 etc. for referencing in MQTT topics and HTML object id's (This will fail if there are more than 9 spots)

        // set box color according to current status using status-specific styling classes
        document.getElementById(spot_id_verbose).className = "spot " + spot_data['status'];

        // show distance in cm while status is PARKING_IN
        if (['PARKING_IN','PARKING_OUT'].includes(spot_data['status'])) {
            // document.getElementById(spot_id_verbose + "_label").innerText = '⬌ ' + spot_data['car_distance'] + 'cm';
            document.getElementById(spot_id_verbose + "_label").innerText = '';
            document.getElementById(spot_id_verbose).style.backgroundPositionY = "calc(-1rem + 14rem * (" + spot_data['car_distance'] + " / " + PARKING_SPOT_LENGTH_CM + ")";
        }



        // increment free parking spot counter if this spot is free
        if (spot_data['status'] === 'FREE')
        {
            count_free_parking_spots = count_free_parking_spots + 1;

            // show the "Reserve" button for free parking spots only
            document.getElementById("btn_reserve_parking_spot0" + i).style.display = 'block';
        }
        else {
            // hide "Reserve" button
            document.getElementById("btn_reserve_parking_spot0" + i).style.display = 'none';
        }
        
        // display translated status label for occupied, reserved and free parking spots
        if (['RESERVED', 'FREE'].includes(spot_data['status'])) {
            document.getElementById(spot_id_verbose + "_label").innerText = LOCALIZATIONSTRINGS[spot_data['status']];
        }

        // this spot is reserved -> show remaining time until expiration
        if (spot_data['status'] === 'RESERVED') {
            // compute the remaining time until expiration in seconds using Unix timestamps
            const remaining = Math.floor( ( spot_data['reservation_expires_at'] - new Date() ) / 60000  * 60 );

            // convert remaining seconds to min:sec format for easier reading
            // this method is silly but it works and does not require external libraries
            const minutesRemaining = Math.floor( remaining/60 ).toString().padStart(2, '0');
            const secondsRemaining = (remaining - minutesRemaining * 60).toString().padStart(2, '0');
            
            // check if the reservation has expired
            if (remaining <= 0)
                cancelReservation(spot_id_verbose);
            
            // print status and remaining time until the reservation expires
            document.getElementById(spot_id_verbose + "_label").innerText = LOCALIZATIONSTRINGS['RESERVED'] + " (" + minutesRemaining + ":" + secondsRemaining + ")";
        }
    }

    // print number of free parking spots on top of the page
    document.getElementById('num_free_parking_spots').innerText = count_free_parking_spots;
}

// Actions on startup
window.onload = function() {
    startConnect("127.0.0.1", 9001);  // 192.168.0.119 in mico-b-wifi

    // register event listeners to Reserve buttons
    document.getElementById('btn_reserve_parking_spot01').addEventListener('click', function(){
        makeReservation("spot01");
    });
    document.getElementById('btn_reserve_parking_spot02').addEventListener('click', function(){
        makeReservation("spot02");
    });
    document.getElementById('btn_reserve_parking_spot03').addEventListener('click', function(){
        makeReservation("spot03");
    });
    
    // start timer that calls the update() function repeatedly
    setInterval(function() {
        update();
      }, UPDATE_LOOP_INTERVAL_MS);
};



