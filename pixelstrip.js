var ws;

var jsonObject;
var mode;
var currentTheme;
var themeNum;
var delay;
var available;

var startH = 0;
var startM = 0;
var stopH = 23;
var stopM = 59;

var cnt = 0;

function triggerMode(theme, delay) {
  ws.send(
    '{"trigger":{"mode":"infinite", "theme":"' +
      theme +
      '", "delay":' +
      delay +
      "}}"
  );
}

function sendValidTime() {
  ws.send(
    '{"validTimeSet": {"startH":' +
      startH +
      ', "startM":' +
      startM +
      ', "stopH":' +
      stopH +
      ', "stopM":' +
      stopM +
      "}}"
  );
}

const interval = setInterval(function() {
  if (ws.readyState == WebSocket.OPEN) {
    cnt = cnt + 1;
    ws.send('{"ping":' + cnt + "}");

    var pingcounter = document.getElementById("pingcnt");
    pingcounter.innerHTML = cnt;
  }
}, 10000);

function WebSocketBegin() {
  if ("WebSocket" in window) {
    //Let us open a web socket
    /*ws = new WebSocket(
            location.hostname.match(/\.husarnetusers\.com$/)
              ? "wss://" + location.hostname + "/__port_8001/"
              : "ws://" + location.hostname + ":8001/"
          );*/
    ws = new WebSocket("ws://esp32big:8001/");

    ws.onopen = function() {
      // Web Socket is connected
      //alert("Connection is opened...");

      ws.send('{"getConfig":1}');
    };

    ws.onmessage = function(evt) {
      jsonObject = JSON.parse(evt.data);
      console.log(evt.data);

      if (jsonObject.hasOwnProperty("config")) {
        mode = jsonObject.config.mode;
        currentTheme = jsonObject.config.currentTheme;
        themeNum = jsonObject.config.themeNum;
        delay = jsonObject.config.delay;

        startH = jsonObject.config.startH;
        startM = jsonObject.config.startM;
        stopH = jsonObject.config.stopH;
        stopM = jsonObject.config.stopM;

        console.log(
          "mode: " +
            mode +
            "\r\ncurrentTheme: " +
            currentTheme +
            "\r\ndelay: " +
            delay +
            "ms"
        );
        var slider = document.getElementById("myRange");
        var output = document.getElementById("demo");
        slider.value = delay;
        output.innerHTML = delay;

        var timeStartH = document.getElementById("timeStartH");
        var timeStartM = document.getElementById("timeStartM");
        var timeStopH = document.getElementById("timeStopH");
        var timeStopM = document.getElementById("timeStopM");

        for (i = 0; i < 24; i++) {
          var option = document.createElement("option");
          option.text = i;
          option.value = i;
          timeStartH.add(option);
        }
        for (i = 0; i < 60; i++) {
          var option = document.createElement("option");
          option.text = i;
          option.value = i;
          timeStartM.add(option);
        }
        for (i = 0; i < 24; i++) {
          var option = document.createElement("option");
          option.text = i;
          option.value = i;
          timeStopH.add(option);
        }
        for (i = 0; i < 60; i++) {
          var option = document.createElement("option");
          option.text = i;
          option.value = i;
          timeStopM.add(option);
        }
        $("#timeStartH option[value=" + startH + "]").prop("selected", true);
        $("#timeStartM option[value=" + startM + "]").prop("selected", true);

        $("#timeStopH option[value=" + stopH + "]").prop("selected", true);
        $("#timeStopM option[value=" + stopM + "]").prop("selected", true);
      }

      if (jsonObject.hasOwnProperty("themesConfig")) {
        var i;
        available = jsonObject.themesConfig.themes.length;
        
        console.log(
            "\r\navailable: " +
            available
        );
        
        for (i = 0; i < available; i++) {
          console.log(
            "theme[" + i + "]: " + jsonObject.themesConfig.themes[i].name
          );
          //$( "#btns" ).append( "<button type=\"button\" class=\"btn btn-lg btn-block " + ((themeNum==i)?"btn-success":"btn-primary") + "\" onmousedown=\'triggerMode(\"" + jsonObject.themes[i].name + "\",50)\' ontouchstart=\'triggerMode(\"jsonObject.themes[i].name\",50)\'>" + jsonObject.themes[i].name + "</button>");
          $("#thms").append(
            "<option value='" +
              i +
              "'" +
              (themeNum == i ? ' selected="selected">' : ">") +
              jsonObject.themesConfig.themes[i].name +
              "</option>"
          );
          //+ ((themeNum==i)?" selected>":">")
        }
      }
    };

    ws.onclose = function() {
      // websocket is closed.
      alert("WS connection is closed...");
      //ws = new WebSocket("ws://test756:8001/");
    };
  } else {
    // The browser doesn't support WebSocket
    alert("WebSocket NOT supported by your Browser!");
  }
}

var slider = document.getElementById("myRange");
var output = document.getElementById("demo");
output.innerHTML = slider.value;

slider.oninput = function() {
  output.innerHTML = this.value;
};

slider.ontouchend = function() {
  output.innerHTML = this.value;
  delay = this.value;
  triggerMode(currentTheme, delay);
};
slider.onmouseup = function() {
  output.innerHTML = this.value;
  delay = this.value;
  triggerMode(currentTheme, delay);
};

$("#thms").change(function() {
  console.log("change them");
  currentTheme = $("#thms option:selected").text();
  triggerMode(currentTheme, delay);
});

$("#timeStartH").change(function() {
  startH = $("#timeStartH option:selected").val();
  console.log("start time H\r\n" + startH);
  sendValidTime();
});

$("#timeStartM").change(function() {
  startM = $("#timeStartM option:selected").val();
  console.log("start time M\r\n" + startM);
  sendValidTime();
});

$("#timeStopH").change(function() {
  stopH = $("#timeStopH option:selected").val();
  console.log("stop time H\r\n" + stopH);
  sendValidTime();
});

$("#timeStopM").change(function() {
  stopM = $("#timeStopM option:selected").val();
  console.log("stop time M\r\n" + stopM);
  sendValidTime();
});
