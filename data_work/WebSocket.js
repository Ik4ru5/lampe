var colorPicker = new iro.ColorPicker("#colorPicker", {
	width: 300,
	height: 300,
	markerRadius: 8,
	borderWidth: 2,
	padding: 0,
	anticlockwise: true,
});

var rainbowEnable = false;
var receivedResonse = true;
var connectionEstablished = false;
var received = "";

var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);

connection.onopen = function () {
	enableSilders();
	connectionEstablished = true;
};
connection.onerror = function (error) {
	disabledSliders();
	alert('WebSocket Error ' + error); // maybe need to be removed
};
connection.onmessage = function (e) {
	received = e.data;
	if(e.data[0] == "#") {
		console.log("received: " + e.data + "");
		
		// convert hey values to decimal
		var r = parseInt(e.data[1] + e.data[2], 16);
		var g = parseInt(e.data[3] + e.data[4], 16);
		var b = parseInt(e.data[5] + e.data[6], 16);
		var w = parseInt(e.data[7] + e.data[8], 16);
		
		//convert from rgbw to rgb (just add white amount to rgb)
		r += w;
		g += w;
		b += w;
		colorPicker.color.set("rgb(" + r + ", " + g + ", " + b + ")");
		/*var fader = setInterval(fadeIn, 10);
		
		function fadeIn() {
			if(document.getElementById('r').value < r)
				document.getElementById('r').value += Math.ceil(r / 1000);
			else
				document.getElementById('r').value = r
			
			if(document.getElementById('g').value < g)
				document.getElementById('g').value += Math.ceil(g / 1000);
			else
				document.getElementById('g').value = g
			
			if(document.getElementById('b').value < b)
				document.getElementById('b').value += Math.ceil(b / 1000);
			else
				document.getElementById('b').value = b
			
			if(document.getElementById('w').value < w)
				document.getElementById('w').value += Math.ceil(w / 1000);
			else 
				document.getElementById('w').value = w
			
			if(document.getElementById('r').value >= r &&
			document.getElementById('g').value >= g &&
			document.getElementById('b').value >= b &&
			document.getElementById('w').value >= w) {
				clearInterval(fader)
				document.getElementById('r').value = r
				document.getElementById('g').value = g
				document.getElementById('b').value = b
				document.getElementById('w').value = w
				console.log("Stopping")
			}
		}*/
		receivedResonse = true;
	}
};
connection.onclose = function(){
	disabledSliders();
	connectionEstablished = false;
};

function sendRGB(color) {
	//convert rgb to rgbw (substract the white part from all colors)
	var w = Math.min(color.rgb.r, color.rgb.g, color.rgb.b);
	var r = color.rgb.r - w;
	var g = color.rgb.g - w;
	var b = color.rgb.b - w;
	
	//create hex string
	var rgbwstr = '#'+ r.toString(16).padStart(2, 0) + g.toString(16).padStart(2, 0) + b.toString(16).padStart(2, 0) + w.toString(16).padStart(2, 0); 
	console.log('Sending: ' + rgbwstr);
	if(receivedResonse && rgbwstr != received) {
		connection.send(rgbwstr);
		receivedResonse = false;
	}
}

function sendPattern(pattern) {
	console.log("sending " + pattern);
	connection.send(pattern);
}

function enableSilders() {
	document.getElementById("overlay").style.display = "none";
	document.getElementById("colorPicker").style.pointerEvents = "auto";
	colorPicker.on("color:change", function(color, changes) {
		sendRGB(color);
	});
}

function disabledSliders() {
	document.getElementById("overlay").style.display = "block";
	document.getElementById("colorPicker").style.pointerEvents = "none";
}

/*function rainbowEffect(){
	rainbowEnable = ! rainbowEnable;
	if(rainbowEnable){
		connection.send("R");
		document.getElementById('rainbow').style.backgroundColor = '#00878F';
		disabledSliders();
	} else {
		connection.send("N");
		document.getElementById('rainbow').style.backgroundColor = '#999';
		enableSilders();
		sendRGB();
	}  
}*/
