#!/usr/bin/env node
var http = require('http');

var server = http.createServer(function(request, response) {
    console.log((new Date()) + ' Received request for ' + request.url);
    response.writeHead(404);
    response.end();
});

server.listen(8080, function() {
    console.log((new Date()) + ' Server is listening on port 8080');
});

var WebSocketServer = require('websocket').server;
wsServer = new WebSocketServer({
    httpServer: server,
    // You should not use autoAcceptConnections for production
    // applications, as it defeats all standard cross-origin protection
    // facilities built into the protocol and the browser.  You should
    // *always* verify the connection's origin and decide whether or not
    // to accept it.
    autoAcceptConnections: false
});

function originIsAllowed(origin) {
  // put logic here to detect whether the specified origin is allowed.
  return true;
}

var fw_mode = "station";
var gpio_en = [ 4, 5, 9 ];
var gpio_states = {};

wsServer.on('request', function(request) {
    if (!originIsAllowed(request.origin)) {
      // Make sure we only accept requests from an allowed origin
      request.reject();
      console.log((new Date()) + ' Connection from origin ' + request.origin + ' rejected.');
      return;
    }

    var connection = request.accept('wifiswitch', request.origin);
    console.log((new Date()) + ' Connection accepted.');
    
    connection.on('message', function(message) {
        if (message.type === 'utf8') {
            console.log('Received Message: ' + message.utf8Data);
            data = JSON.parse(message.utf8Data);
            if (data.type === "fw") {
				console.log("Firmware message");
				if (data.hasOwnProperty("mode")) {
					fw_mode = data.mode;
				}
				
				var message = JSON.stringify({ type : "fw", mode : fw_mode, ver : "1.0.1" });
				console.log("Sending: " + message);
				connection.sendUTF(message);
			}
			if (data.type === "networks") {
				console.log("Networks message");
		
				var message = JSON.stringify({ type : "networks", ssids : [ "testAP", "PrettyFlyForAWIFI", "NewAdventuresInWIFI" ] });
				console.log("Sending: " + message);
				connection.sendUTF(message);
			}
			else if (data.type === "station") {
				console.log("Station message");
				if (!data.hasOwnProperty("ssid")) {
					data.ssid = "OhMyWIFI";
				}
				if (!data.hasOwnProperty("hostname")) {
					data.hostname = "testswitch";
				}
				if (!data.hasOwnProperty("ip")) {
					data.ip = "500.500.500.500";
				}

				var message = JSON.stringify({ type : "station", ssid : data.ssid, hostname :  data.hostname, ip : data.ip});
				console.log("Sending: " + message);
				connection.sendUTF(message);				
			}
			else if (data.type === "ap") {
				console.log("AP message");
				if (!data.hasOwnProperty("ssid")) {
					data.ssid = "OhMyWIFI";
				}
				if (!data.hasOwnProperty("hostname")) {
					data.hostname = "testswitch";
				}
				if (!data.hasOwnProperty("channel")) {
					data.channel = 9;
				}
				
				var message = JSON.stringify({ type : "ap", ssid : data.ssid, hostname : data.hostname, channel : data.channel});
				console.log("Sending: " + message);
				connection.sendUTF(message);
			}
			else if (data.type === "gpio") {
				console.log("GPIO message");
				
				for (var key in data) {
					if (data.hasOwnProperty(key)) {
						if (!isNAN(key)) {
							gpio_states[key] = data[key];
						}
					}
				}
				
				var message = JSON.stringify({ type : "gpio", gpios : gpio_en} + gpio_states);
				console.log("Sending: " + message);
				connection.sendUTF(message);				
			}
			else {
				console.log("Unknown message");
			}            
        }
        else {
            console.log('Received unusable message type.');
        }
    });
    connection.on('close', function(reasonCode, description) {
        console.log((new Date()) + ' Peer ' + connection.remoteAddress + ' disconnected.');
    });
    
    setInterval(function() {
		gpio_states[5] = Math.floor((Math.random() * 2));
		var message = JSON.stringify({ type : "gpio", 5 : gpio_states[5]});
		console.log("Sending: " + message);
		connection.sendUTF(message);
	}, 3333)
});
