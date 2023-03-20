var dataSet;
var myIPaddress;
var chart1;
var chart2;
var chart3;
var chart4;
var chart5;
const a_day = 1000*60*60*24;

if(window.location.pathname.includes("/home") || window.location.pathname.includes("C:")){
  var myIPaddress = "http://192.168.0.23/";
}
else{
  var myIPaddress = "/";
}
CanvasJS.addCultureInfo("pl", {});


document.cookie = 'SameSite=None; ; path=/; Secure'
 

$('#datepicker').datepicker({
    uiLibrary: 'bootstrap4',
    footer: true,
    format: 'dd-mm-yy',
    modal: true,
    change: function (e) {
		fetch_logs_date_callback();
		fetch_picture_list_callback();
	}
});


function fetch_logs_date_callback(){
	var date, filename;
	date = $('#datepicker').val();
	filename= date.split('-').join('')+'.CSV';
	FetchCSVLog(filename);
	addLoader();
	closeError();
}

function fetch_picture_list_callback(){
	var date;
	date = $('#datepicker').val();
	parts = date.split("-");
	var day = parts[0];
	var month = parts[1];
	var year = parts[2] < 100 ? 2000 + parseInt(parts[2]) : parseInt(parts[2]); // ustaw rok XXI wieku, jeśli jest mniejszy niż 100
	date = new Date(year, month - 1, day);
	fetch_picture_list_for(date)
}

function fetch_picture_list_for(date){
	FetchPicList('pic_list/?'+ format_date(date));
}



function instantiateCharts(){
  chart1 = new CanvasJS.Chart("chartContainer1", {
    theme: "light1", //, "light1", "light2", "dark1", "dark2"
    animationEnabled: true,
    animationDuration: 1500,
    zoomEnabled: true,
    //backgroundColor: "#6096BA",
    title: {
      text: "Temperature [°C]"
    },
    toolTip:{   
      content: "{name}: {y}"      
    },
    axisX: {
      valueFormatString: " DD-MM-YYYY  HH:mm:ss",
      labelAngle: -20,
      crosshair: {
        enabled: true,
        snapToDataPoint: true
      }
    },
    axisY: {
      title: "Temperature (°C)",
      gridColor: "black",
      lineColor: "red",
      interval: 10,
      gridThickness: 1,
      minimum: -50,
      maximum: 50,
      crosshair: {
        enabled: true,
        snapToDataPoint: true,
        labelFormatter: function(e) {
          return CanvasJS.formatNumber(e.value, "##0 °C");
        }
      }
    },
    legend:{
      cursor: "pointer",
      fontSize: 16,
      itemclick: toggleDataSeries
    },
    data: [{
      name: "Internal temperature",
      type: "spline",
      xValueType: "dateTime",
      yValueFormatString: "#0.## °C",
      showInLegend: true,
      dataPoints: []
    },{
      name: "External temperature",
      type: "spline",
      xValueType: "dateTime",
      yValueFormatString: "#0.## °C",
      showInLegend: true,
      dataPoints: []
    }]
  });

  chart2 = new CanvasJS.Chart("chartContainer2", {
    theme: "light1", //, "light1", "light2", "dark1", "dark2"
    animationEnabled: true,
    animationDuration: 1500,
    zoomEnabled: true,
    title: {
      text: "Sun Illuminance [lx]"
    },
    toolTip:{   
      content: "{name}: {y}"      
    },
    axisY: {
      lineColor: "green",
      title: "Sun Illuminance (lx)",
      includeZero: true,
      logarithmic: true, //change it to false
      suffix: " lx",
      crosshair: {
        enabled: true,
        snapToDataPoint: true,
        labelFormatter: function(e) {
          return CanvasJS.formatNumber(e.value, "##0.00 lx");
        }
      },
      //minimum: -10,
      //maximum: 65000,
    },
    axisX: {
      valueFormatString: " DD-MM-YYYY  HH:mm:ss",
      crosshair: {
        enabled: true,
        snapToDataPoint: true
      }
    },
    legend:{
      cursor: "pointer",
      fontSize: 16,
      itemclick: toggleDataSeries
    },
    data: [{
      name: "Illuminance",
      type: "spline",
      xValueType: "dateTime",
      yValueFormatString: "#0.## lx",
      showInLegend: true,
      dataPoints: []
    }]
  });

  chart3 = new CanvasJS.Chart("chartContainer3", {
    theme: "light1", //, "light1", "light2", "dark1", "dark2"
    animationEnabled: true,
    animationDuration: 1500,
    zoomEnabled: true,
    title: {
      text: "Humidity [%]"
    },
    toolTip:{   
      content: "{name}: {y}"      
    },
    axisY: {
      lineColor: "lightgreen",
      title: "Humidity (%)",
      interval: 5,
      includeZero: true,
      suffix: "%",
      minimum: 0,
      maximum: 100,
      crosshair: {
        enabled: true,
        snapToDataPoint: true,
        labelFormatter: function(e) {
          return CanvasJS.formatNumber(e.value/100, "##0 %");
        }
      }
    },
    axisX: {
      valueFormatString: " DD-MM-YYYY  HH:mm:ss",
      crosshair: {
        enabled: true,
        snapToDataPoint: true
      }
    },
    legend:{
      cursor: "pointer",
      fontSize: 16,
      itemclick: toggleDataSeries
    },
    data: [{
      name: "Humidity",
      type: "splineArea",
      xValueType: "dateTime",        
      dataPoints: []
    }]
  });

  chart4 = new CanvasJS.Chart("chartContainer4", {
    theme: "light1", //, "light1", "light2", "dark1", "dark2"
    animationEnabled: true,
    animationDuration: 1500,
    zoomEnabled: true,
    title: {
      text: "Pressure [hPa]"
    },
    toolTip:{   
      content: "{name}: {y}"      
    },
    axisX: {
      valueFormatString: " DD-MM-YYYY  HH:mm:ss",
      crosshair: {
        enabled: true,
        snapToDataPoint: true
      }
    },
    axisY: {
      title: "Atm Pressure (hPa)",
      minimum: 950,
      maximum: 1050,
      crosshair: {
        enabled: true,
        snapToDataPoint: true,
        labelFormatter: function(e) {
          return CanvasJS.formatNumber(e.value, "##0");
        }
      }
    },
    legend:{
      cursor: "pointer",
      fontSize: 16,
      itemclick: toggleDataSeries
    },
    data: [{
      name: "Pressure",
      type: "splineArea",
      xValueType: "dateTime",        
      dataPoints: []
    }]
  });
  
  
  chart5 = new CanvasJS.Chart("chartContainer5", {
    theme: "light1", //, "light1", "light2", "dark1", "dark2"
    animationEnabled: true,
    animationDuration: 1500,
    zoomEnabled: true,
    title: {
      text: "Wind [m/s]"
    },
    toolTip:{   
      content: "{name}: {y}"      
    },
    axisX: {
      valueFormatString: " DD-MM-YYYY  HH:mm:ss",
      crosshair: {
        enabled: true,
        snapToDataPoint: true
      }
    },
    axisY: {
      title: "Wind speed (m/s)",
      minimum: 0,
      maximum: 200,
      crosshair: {
        enabled: true,
        snapToDataPoint: true,
        labelFormatter: function(e) {
          return CanvasJS.formatNumber(e.value, "##0.000 m/s");
        }
      }
    },
    legend:{
      cursor: "pointer",
      fontSize: 16,
      itemclick: toggleDataSeries
    },
    data: [{
      name: "WindSpeed",
      type: "splineArea",
      xValueType: "dateTime",        
      dataPoints: []
    }]
  });

}
  

//index is no of days back (ex: 1 will produce yesterday's date)
function resolve_date_by_idx(log_index){
	date = new Date((new Date()).valueOf() - a_day * log_index);
	dd = date.getDate().toLocaleString('en-US', {minimumIntegerDigits: 2, useGrouping:false});
	mm = (date.getMonth() +1).toLocaleString('en-US', {minimumIntegerDigits: 2, useGrouping:false});
	yy = date.getYear() - 100;
	return (''+dd + mm + yy);
}

//returns log filename based on index (no of days back) and extension
function resolve_log_name_by(log_index, extension){
	var log_name;
	if(log_index > 0){
		log_name = resolve_date_by_idx(log_index) + '.' + extension
	}else{
		log_name = 'CURRENT.' + extension
	}
	return log_name;
}

//base on high precision checkbox
function resolve_path(){
	return $('#precise_logs').is(":checked") ? "logs/" : "logs/avg/";
}

//Fething CSV Log by index
function SwitchCSVLog(log_index){
	log_name = resolve_log_name_by(log_index, 'CSV')
	FetchCSVLog(log_name);
	$(".btn").attr("disabled", false);
	$("#button"+log_index).attr("disabled", true);
	var date = log_index == 0 ? new Date : yesterdayDate();
	fetch_picture_list_for(date);
	addLoader();
	closeError();
}

function yesterdayDate(){
	var date = new Date();
	return new Date(date.setDate(date.getDate()-1));
}

//Fething JS Log by index
function SwitchJSLog(log_index){
	log_name = resolve_log_name_by(log_index, 'JSO')
	FetchJSLog(log_name);
	$(".btn").attr("disabled", false);
	$("#button"+log_index).attr("disabled", true);
	addLoader();
	closeError();
}

var dataSet;
var response;

var config = {
	quotes: false, //or array of booleans
	quoteChar: '"',
	escapeChar: '"',
	delimiter: ",",
	header: true,
	newline: "\n",
	skipEmptyLines: 'greedy',
	columns: null, //or array of strings
	dynamicTyping: true
}

//fetch log from weather station
function FetchPicList(path){
  var xmlhttp, path;
  if (window.XMLHttpRequest){ xmlhttp = new XMLHttpRequest();  }
  else { xmlhttp = new ActiveXObject("Microsoft.XMLHTTP"); }
  xmlhttp.onreadystatechange = function() {
    if (xmlhttp.readyState == 4 && xmlhttp.status == 200){
		newPicList = document.createElement('ol');
		newPicList.innerHTML = xmlhttp.responseText;
		oldPicList = document.getElementById("pic_list");
		oldPicList.parentNode.replaceChild(newPicList, oldPicList);
		removeLoader();
		displaySuccess("Picture list loaded successfully!");
		displayLogErrors();
		setTimeout('closeBar()',5000);
    }else if(xmlhttp.readyState == 4){
      removeLoader();
      displayError("Can not find any pictures.");
    }
  }
  xmlhttp.open("GET", myIPaddress + path, true);
  xmlhttp.send();
}
var result;
//fetch log from weather station
function FetchCSVLog(log_fname){
  var xmlhttp, path;
  if (window.XMLHttpRequest){ xmlhttp = new XMLHttpRequest();  }
  else { xmlhttp = new ActiveXObject("Microsoft.XMLHTTP"); }
  xmlhttp.onreadystatechange = function() {
    if (xmlhttp.readyState == 4 && xmlhttp.status == 200){
		response = xmlhttp.responseText;
		dataSet = Papa.parse(response, config).data;
		addDataPoints(dataSet);
		chart1.render();
		chart2.render();
		chart3.render();
		chart4.render();
		chart5.render();
		removeLoader();
		displaySuccess("Data loaded successfully!");
		displayLogErrors();
		setTimeout('closeBar()',5000);
		setTimeout('closeError()',5000);
    }else if(xmlhttp.readyState == 4){
      removeLoader();
      displayError("Can not find that log file. Choose another one.");
    }
  }
  path = resolve_path();
  xmlhttp.open("GET", myIPaddress + path + log_fname, true);
  xmlhttp.send();
}

//fetch log from weather station
function FetchJSLog(log_fname){
  var xmlhttp;
  if (window.XMLHttpRequest){ xmlhttp = new XMLHttpRequest();  }
  else { xmlhttp = new ActiveXObject("Microsoft.XMLHTTP"); }
  xmlhttp.onreadystatechange = function() {
    if (xmlhttp.readyState == 4 && xmlhttp.status == 200){
		response = xmlhttp.responseText;
		if(response[response.length -2] == ','){ //repair log content if it is unended
			response = response.slice(0, -2) + ']';
		}
		dataSet = JSON.parse(response);
		addDataPoints(dataSet);
		chart1.render();
		chart2.render();
		chart3.render();
		chart4.render();
		chart5.render();
		removeLoader();
		displaySuccess("Data loaded successfully!");
		displayLogErrors();
		setTimeout('closeBar()',5000);
    }else if(xmlhttp.readyState == 4){
      removeLoader();
      displayError("Can not find that log file. Choose another one.");
    }
  }
  xmlhttp.open("GET", myIPaddress + "logs/" + log_fname, true);
  xmlhttp.send();
}

function addDataPoints(dataSet) {
  var xVal =0, yVal=0,  timestamp;
  cleanCharts ();
  for(var i = 0; i < dataSet.length; i++) {
    timestamp = new Date(dataSet[i].time * 1000); //dataSet[i].timestamp * 1000;//
    timestamp.setHours(timestamp.getHours() - 2); //set as local time
    if((dataSet[i].time)&&(timestamp.getFullYear() >= 2020)){  //display logged data only if this entry has valid date and timestamp is valid
      chart1.options.data[0].dataPoints.push({x: timestamp,y: dataSet[i].int_t});
      chart1.options.data[1].dataPoints.push({x: timestamp,y: dataSet[i].ext_t});
      
      if(dataSet[i].sun <= 0 ){ dataSet[i].sun = 0.1; }
	  if(dataSet[i].sun > 60000){						//if value is grater than real values range
		if(i > 0) dataSet[i].sun = dataSet[i-1].sun;	//set to previous row value
		else dataSet[i].sun = 0.1;
  	  }
      chart2.options.data[0].dataPoints.push({x: timestamp,y: dataSet[i].sun});
      
      chart3.options.data[0].dataPoints.push({x: timestamp,y: (dataSet[i].humi < 100 ? dataSet[i].humi : 0)});
      chart4.options.data[0].dataPoints.push({x: timestamp,y: ((dataSet[i].press >= 900) ? dataSet[i].press : 991)});
      chart5.options.data[0].dataPoints.push({x: timestamp,y: (dataSet[i].wind < 0 ? 0 : dataSet[i].wind)});
    }
  }
  result = chart1.options.data;
}

function cleanCharts(){
  //remove all charts and reinstantiate them
  instantiateCharts();
}

function toggleDataSeries(e){
  if (typeof(e.dataSeries.visible) === "undefined" || e.dataSeries.visible) {
    e.dataSeries.visible = false;
  }
  else{
    e.dataSeries.visible = true;
  }
  e.chart.render();
}
// Display error list

function displayLogErrors(){
  var timestamp, error;
  $("#log_errors_list").empty();
  $("#log_errors").hide();
  for(var i = 0; i < dataSet.length; i++) {
    timestamp = new Date(dataSet[i].time * 1000); //dataSet[i].timestamp * 1000;//
    timestamp.setHours(timestamp.getHours() - 2); //set as local time
    if(dataSet[i].Error){
      $("#log_errors_list").append('<li>'+timestamp.toLocaleDateString('pl-pl')+' '+timestamp.toLocaleTimeString('pl-pl')+': '+dataSet[i].Error+'</li>')
    }
  }
  if($("#log_errors_list").html().length) $("#log_errors").show();
}


//  loader
function removeLoader(){
  $( "#loadingDiv" ).fadeOut(800, function() {
    $( "#loadingDiv" ).remove();
  });  
}

function addLoader(){
  $('body').append('<div style="" id="loadingDiv"><div class="loader">Loading...</div></div>');
}

//  error msgs: from here: https://code-boxx.com/simple-css-error-messages/
//function displayError(error_msg){
//  $("#errors").append('<div class="bar error"><span class="close" onclick="closeError()">X</span> &#9747;'+error_msg+'</div>');
//}

function displayError(error_msg){
  $("#errors").append('<div class="alert alert-danger alert-dismissible fade show" role="alert">'+error_msg+'<button type="button" class="close" data-dismiss="alert" aria-label="Close"><span aria-hidden="true">&times;</span></button></div>');
}

function displaySuccess(success_msg){
  $("#errors").append('<div class="alert alert-success alert-dismissible fade show" role="alert">'+success_msg+'<button type="button" class="close" data-dismiss="alert" aria-label="Close"><span aria-hidden="true">&times;</span></button></div>');
}

//function displaySuccess(success_msg){
//  $("#errors").append('<div class="bar success"><span class="close" onclick="closeError()">X</span> &#10004;'+success_msg+'</div>');
//}

function closeError(){
  var el = $('.alert');
  el.fadeOut(600, function() { el.remove(); });  
}

function closeBar(){
  var el = $('.bar');
  el.fadeOut(800, function() { el.remove(); });  
}

function format_date(date){
	const day = ("0" + date.getDate()).slice(-2);
	const month = ("0" + (date.getMonth() + 1)).slice(-2);
	const year = date.getFullYear();
	const formattedDate = year + '/' + month + '/' + day;
	return formattedDate;
}
