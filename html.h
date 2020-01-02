R"rawText(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <meta http-equiv="X-UA-Compatible" content="ie=edge" />
    <link
      rel="stylesheet"
      href="https://use.fontawesome.com/releases/v5.0.13/css/all.css"
      integrity="sha384-DNOHZ68U8hZfKXOrtjWvjxusGo9WQnrNx2sqG0tfsghAvtVlRW3tvkXWZh58N9jp"
      crossorigin="anonymous"
    />
    <link
      rel="stylesheet"
      href="https://stackpath.bootstrapcdn.com/bootstrap/4.1.1/css/bootstrap.min.css"
      integrity="sha384-WskhaSGFgHYWDcbwN70/dfYBj47jz9qbsMId/iRN3ewGhXQFZCSftd1LZCfmhktB"
      crossorigin="anonymous"
    />
    <title>Pixel Strip Controller</title>

    <script src="https://code.jquery.com/jquery-3.4.1.js"></script>
    <script type="text/javascript">
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
          ws = new WebSocket("ws://esp32strip:8001/");

          ws.onopen = function() {
            // Web Socket is connected
            //alert("Connection is opened...");

            ws.send('{"getConfig":1}');
          };

          ws.onmessage = function(evt) {
            jsonObject = JSON.parse(evt.data);
            mode = jsonObject.config.mode;
            currentTheme = jsonObject.config.currentTheme;
            themeNum = jsonObject.config.themeNum;
            delay = jsonObject.config.delay;
            available = jsonObject.config.available;

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
                "ms" +
                "\r\navailable: " +
                available
            );
            var slider = document.getElementById("myRange");
            var output = document.getElementById("demo");
            slider.value = delay;
            output.innerHTML = delay;

            var i;
            for (i = 0; i < available; i++) {
              console.log("theme[" + i + "]: " + jsonObject.themes[i].name);
              $("#thms").append(
                "<option value='" +
                  i +
                  "'" +
                  (themeNum == i ? ' selected="selected">' : ">") +
                  jsonObject.themes[i].name +
                  "</option>"
              );
            }

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
            $("#timeStartH option[value=" + startH + "]").prop(
              "selected",
              true
            );
            $("#timeStartM option[value=" + startM + "]").prop(
              "selected",
              true
            );

            $("#timeStopH option[value=" + stopH + "]").prop("selected", true);
            $("#timeStopM option[value=" + stopM + "]").prop("selected", true);
          };

          ws.onclose = function() {
            // websocket is closed.
            alert("WebSocket is closed...");
          };
        } else {
          // The browser doesn't support WebSocket
          alert("WebSocket NOT supported by your Browser!");
        }
      }
    </script>
    <style>
      .slidecontainer {
        width: 100%;
      }

      .slider {
        -webkit-appearance: none;
        width: 100%;
        height: 25px;
        background: #d3d3d3;
        outline: none;
        opacity: 0.7;
        -webkit-transition: 0.2s;
        transition: opacity 0.2s;
      }

      .slider:hover {
        opacity: 1;
      }

      .slider::-webkit-slider-thumb {
        -webkit-appearance: none;
        appearance: none;
        width: 25px;
        height: 25px;
        background: #161616;
        cursor: pointer;
      }

      .slider::-moz-range-thumb {
        width: 25px;
        height: 25px;
        background: #161616;
        cursor: pointer;
      }
    </style>
  </head>

  <body onLoad="javascript:WebSocketBegin();">
    <header id="main-header" class="py-2 bg-light">
      <div class="container">
        <div class="row justify-content-md-center">
          <div class="col-md-6 text-center">
            <h1><i class="fas fa-cog"></i> Pixel Strip Controller</h1>
          </div>
        </div>
      </div>
    </header>

    <section class="py-5 bg-white">
      <div class="container">
        <div class="row">
          <div class="col" id="btns">
            <!--<div id="buttons"></div> -->
            <form>
              <div class="form-group">
                <label for="formctrl-themes">Select theme</label>
                <select class="form-control" id="thms"> </select>
              </div>

              <label class="mt-5">Set delay</label>
              <div class="slidecontainer">
                <input
                  type="range"
                  min="20"
                  max="500"
                  value="50"
                  class="slider"
                  id="myRange"
                />
                <p>Current delay: <span id="demo"></span> [ms]</p>
              </div>

              <div class="form-inline mt-5">
                <label for="formctrl-time">Select start time</label>
                <select class="form-control" id="timeStartH"> </select>
                <select class="form-control" id="timeStartM"> </select>
              </div>
              <div class="form-inline mt-5">
                <label for="formctrl-time">Select stop time</label>
                <select class="form-control" id="timeStopH"> </select>
                <select class="form-control" id="timeStopM"> </select>
              </div>
              <label class="mt-5" id="pingcnt">0</label>
            </form>
          </div>
        </div>
      </div>
    </section>

    <script>
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
    </script>

    <script>
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

    </script>
  </body>
</html>

)rawText"
