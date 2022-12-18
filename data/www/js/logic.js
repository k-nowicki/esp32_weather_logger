
// disableAdvanced();
// DoCommand('get', 'current_ms', '');
//GetArduinoInputs();


var gauge0 = new LinearGauge({
    renderTo: 'Canvas0',
    width: 170,
    height: 600,
    // units: "°C",
    title: "Extern T [°C]",
    minValue: -50,
    maxValue: 50,
    majorTicks: [ -50, -40, -30, -20, -10, 0, 10, 20, 30, 40, 50 ],
    minorTicks: 10,
    strokeTicks: false,
    ticksWidth: 15,
    ticksWidthMinor: 7.5,
    highlights: [   {"from": -50, "to": 0, "color": "rgba(0,120, 255, .4)" },
                    {"from": 0, "to": 50, "color": "rgba(255, 0, 0, .4)" } ],
    colorMajorTicks: "#555",
    colorMinorTicks: "#222",
    colorTitle: "#222",
    borderShadowWidth: 0,
    borders: false,
    needle: true,
    needleType: "line",
    needleWidth: 0.5,
    animation: false,
    colorNeedle: "#000",
    colorNeedleEnd: "#fff",
    colorBarProgress: "#9b1717", //"#9b1717",
    colorBar: "#f5f5f5",
    barStroke: 0,
    barWidth: 8,
    valueBox: false,
    barBeginCircle: false
})
gauge0.draw();
setInterval(function () { gauge0.value = extern_t; }, 200);

var gauge1 = new LinearGauge({
    renderTo: 'Canvas1',
    width: 170,
    height: 600,
    // units: "°C",
    title: "Intern T [°C]",
    minValue: -50,
    maxValue: 50,
    majorTicks: [ -50, -40, -30, -20, -10, 0, 10, 20, 30, 40, 50 ],
    minorTicks: 10,
    strokeTicks: false,
    ticksWidth: 15,
    ticksWidthMinor: 7.5,
    highlights: [   {"from": -50, "to": 0, "color": "rgba(0,120, 255, .4)" },
                    {"from": 0, "to": 50, "color": "rgba(255, 0, 0, .4)" } ],
    colorMajorTicks: "#555",
    colorMinorTicks: "#222",
    colorTitle: "#222",
    borderShadowWidth: 0,
    borders: false,
    needle: true,
    needleType: "line",
    needleWidth: 1,
    animation: false,
    colorNeedle: "#000",
    colorNeedleEnd: "#fff",
    colorBarProgress: "#9b1717", //"#9b1717",
    colorBar: "#f5f5f5",
    barStroke: 0,
    barWidth: 8,
    valueBox: false,
    barBeginCircle: false
})
gauge1.draw();
setInterval(function () { gauge1.value = intern_t; }, 500);

var gauge2 = new RadialGauge({
    renderTo: 'Canvas2',
    title: "Atm Pressure [hPa]",
    borders: false,
    width: 250, height: 250,
    // valueFormat: { int: 4, dec: 2 },
    // glow: true,
    minValue: 950,
    maxValue: 1050,
    majorTicks: ['950', '960', '970', '980', '990', '1000', '1010', '1020', '1030', '1040', '1050'],
    minorTicks: 10,
    valueBox: false,
    animation: false
})
gauge2.draw();
setInterval(function () { gauge2.value = pressure; }, 500);

var gauge3 = new RadialGauge({
    renderTo: 'Canvas3',
    title: "Humidity [%]",
    borders: false,
    width: 250, height: 250,
    // valueFormat: { int: 4, dec: 2 },
    // glow: true,
    minValue: 0,
    maxValue: 100,
    majorTicks: ['0', '10', '20', '30', '40', '50', '60', '70', '80', '90', '100'],
    minorTicks: 10,
    valueBox: false,
    animation: false
})
gauge3.draw();
setInterval(function () { gauge3.value = humidity; }, 500);

var gauge4 = new LinearGauge({
    renderTo: 'Canvas4',
    width: 600,
    height: 150,
    title: "Wind [km/h]",
    minValue: 0,
    maxValue: 200,
    majorTicks: ["0","10","20","30","40","50","60","70","80","90","100","110","120","130","140","150","160","170","180","190","200"],
    minorTicks: 10,
    strokeTicks: true,
    colorPlate: "#fff",
    colorTitle: "#111",
    colorUnits: "#111",
    fontUnitsSize: 30,
    fontUnitsWeight: 100,
    borderShadowWidth: 0,
    borders: false,
    barBeginCircle: false,
    tickSide: "left",
    numberSide: "left",
    needleSide: "left",
    needleType: "arrow",
    needleWidth: 3,
    colorNeedle: "#f22",
    colorNeedleEnd: "#422",
    animation: false,
    barWidth: 0,
    ticksWidth: 30,
    ticksWidthMinor: 15,
    valueBox: false,
    animation: false
})
gauge4.draw();
setInterval(function () { gauge4.value = wind; }, 500);

var gauge5 = new LinearGauge({
    renderTo: 'Canvas5',
    title: 'Illuminance [lx]',
    width: 600,
    height: 150,
    minValue: 0,
    maxValue: 60000,
    majorTicks: ['0', '10000', '20000', '30000', '40000', '50000', '60000'],
    minorTicks: 100,
    strokeTicks: true,
    colorPlate: "#fff",
    colorTitle: "#111",
    colorUnits: "#111",
    fontUnitsSize: 30,
    fontUnitsWeight: 100,
    borderShadowWidth: 0,
    borders: false,
    barBeginCircle: false,
    tickSide: "left",
    numberSide: "left",
    needleSide: "left",
    needleType: "arrow",
    needleWidth: 3,
    colorNeedle: "#f22",
    colorNeedleEnd: "#422",
    animation: false,
    barWidth: 0,
    ticksWidth: 30,
    ticksWidthMinor: 15,
    valueBox: false,
    animation: false
  })

gauge5.draw();
setInterval(function () { gauge5.value = illuminance ; }, 500);