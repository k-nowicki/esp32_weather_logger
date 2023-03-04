// Use ip address under development
// Use "/" string when using from controller web site
if(window.location.pathname.includes("/home") || window.location.pathname.includes("C:")){
  var myIPaddress = "https://192.168.0.20/";
}
else{
  var myIPaddress = "/";
}

var command_prefix = "set/";
var data_prefix = "data/";
var measurements = "current_measurements.json";
var digitalAlljson = "digitalAlljson.json";

var extern_t = 0;
var intern_t = 0;
var humidity = 0;
var illuminance  = 0;
var pressure = 0;
var wind     = 0;
var timestamp = 0;

//Every 5sek get measurements
setInterval(function () { GetMeasurements() ; }, 5000);
setInterval(function () { updateImage() ; }, 10000);


function DoCommand(command, targetElement, value) {
  var xmlhttp;
  var path;
  var type;
  type = (targetElement == "reset") ? ".html" : ".json";
  if (window.XMLHttpRequest) {xmlhttp = new XMLHttpRequest();}
  else {xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");}
  xmlhttp.onreadystatechange = function () {
    if (xmlhttp.readyState == 4 && xmlhttp.status == 200 && xmlhttp.responseText.length){  
	  var jsonObj = JSON.parse(xmlhttp.responseText);
	  if(jsonObj.value){
			document.getElementById(targetElement).innerHTML = convertMiliseconds(jsonObj.value / 1000);
			console.log(convertMiliseconds(jsonObj.value));
		}
    }else if(xmlhttp.status == 203){
	  document.open();
	  document.write(xmlhttp.responseText);
	  document.close();
    }
  }
  if(value.length > 0){path = myIPaddress + command_prefix + targetElement + type + "?value=" + value}
  else {path = myIPaddress + data_prefix + targetElement + type}
  
  xmlhttp.open("GET", path, true);
  xmlhttp.send();
}


function requestReset(){
  var r = confirm("Czy na pewno zrestartować sterownik?");
  if(r){
	DoCommand('set', 'reset', '1');  
  }
}

function updateMeasurements(xmlhttp){
	if (xmlhttp.readyState == 4 && xmlhttp.status == 200){
		var jsonObj = JSON.parse(xmlhttp.responseText);
		//   {"time":"1671120839","int_t":24.27, "ext_t":23.00, "humi":34, "sun":182.50, "press":998.54}
		extern_t = jsonObj.ext_t;
		intern_t = jsonObj.int_t;
		humidity = jsonObj.humi;
		illuminance = jsonObj.sun;
		pressure = jsonObj.press;
		wind = jsonObj.wind ? jsonObj.wind : 0;
		timestamp = Date(jsonObj.time * 1000);
		document.getElementById("current_datetime").innerHTML = timestamp;
		document.getElementById("ext_t_display").innerHTML = extern_t + " °C";
		document.getElementById("int_t_display").innerHTML = intern_t + " °C";
		document.getElementById("humi_display").innerHTML = humidity + " %";
		document.getElementById("press_display").innerHTML = pressure + " hPa";
		document.getElementById("sun_display").innerHTML = illuminance + " lx";
		document.getElementById("wind_display").innerHTML = wind + " km/h";
	}
}

function GetMeasurements(){
	var xmlhttp;
	if (window.XMLHttpRequest){ 
		xmlhttp = new XMLHttpRequest(); 
	}else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	xmlhttp.onreadystatechange = function(){updateMeasurements(xmlhttp);};
	xmlhttp.open("GET", myIPaddress + data_prefix + measurements, true);
	xmlhttp.send();
}


function convertMiliseconds(miliseconds, format) {
    var days, hours, minutes, seconds, total_hours, total_minutes, total_seconds;

    total_seconds = parseInt(Math.floor(miliseconds / 1000));
    total_minutes = parseInt(Math.floor(total_seconds / 60));
    total_hours = parseInt(Math.floor(total_minutes / 60));
    days = parseInt(Math.floor(total_hours / 24));

    seconds = parseInt(total_seconds % 60);
    minutes = parseInt(total_minutes % 60);
    hours = parseInt(total_hours % 24);

    switch (format) {
      case 's':
        return total_seconds;
      case 'm':
        return total_minutes;
      case 'h':
        return total_hours;
      case 'd':
        return days;
      default:
        return '' + days +'days '+ hours + ':'+ minutes + ':'+ seconds;
    }
}
 
function updateImage() {
	var newImage = new Image();
	newImage.src = myIPaddress + "dcim/0/current.jpg?t=" + Date.now();
	document.getElementById("cam-current-image").src = newImage.src;
}