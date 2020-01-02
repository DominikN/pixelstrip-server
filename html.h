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
    <title>ESP32 + Bootstrap + WebSocket + JSON + Husarnet</title>

    <script src="https://code.jquery.com/jquery-3.4.1.js"></script>
    <script type="text/javascript">
      var ws;

      var jsonObject;
      var mode;
      var currentTheme;
      var themeNum;
      var delay;
      var available;

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
          ws = new WebSocket(
            location.hostname.match(/\.husarnetusers\.com$/)
              ? "wss://" + location.hostname + "/__port_8001/"
              : "ws://" + location.hostname + ":8001/"
          );
          //ws = new WebSocket("ws://esp32strip:8001/");

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
            var i;
            for (i = 0; i < available; i++) {
              console.log("theme[" + i + "]: " + jsonObject.themes[i].name);
              //$( "#btns" ).append( "<button type=\"button\" class=\"btn btn-lg btn-block " + ((themeNum==i)?"btn-success":"btn-primary") + "\" onmousedown=\'triggerMode(\"" + jsonObject.themes[i].name + "\",50)\' ontouchstart=\'triggerMode(\"jsonObject.themes[i].name\",50)\'>" + jsonObject.themes[i].name + "</button>");
              $("#thms").append(
                "<option value='" +
                  i +
                  "'" +
                  (themeNum == i ? ' selected="selected">' : ">") +
                  jsonObject.themes[i].name +
                  "</option>"
              );
              //+ ((themeNum==i)?" selected>":">")
            }
          };

          ws.onclose = function() {
            // websocket is closed.
            alert("Connection is closed...");
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
        background: #4caf50;
        cursor: pointer;
      }

      .slider::-moz-range-thumb {
        width: 25px;
        height: 25px;
        background: #4caf50;
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

              <label class="mt-5">Set delay (in ms)</label>

              <div class="slidecontainer">
                <input
                  type="range"
                  min="20"
                  max="500"
                  value="50"
                  class="slider"
                  id="myRange"
                />
                <p>Value: <span id="demo"></span></p>
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
    </script>
  </body>
</html>

)rawText"
