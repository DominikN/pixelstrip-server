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

<script
        src="https://code.jquery.com/jquery-3.4.1.min.js"
        integrity="sha256-CSXorXvZcTkaix6Yvo6HppcZGetbYMGWSFlBw8HfCJo="
        crossorigin="anonymous"></script>
    <script type="text/javascript">


      var ws;
      
      function triggerMode(theme, delay) {
  ws.send('{"trigger":{"mode":"infinite", "theme":"' + theme + '", "delay":' + delay + '}}')
      }

      function WebSocketBegin() {
        if ("WebSocket" in window) {
          //Let us open a web socket
  ws = new WebSocket(location.hostname.match(/\.husarnetusers\.com$/) ? "wss://" + location.hostname + "/__port_8001/" : "ws://" + location.hostname + ":8001/");
    //ws = new WebSocket("ws://esp32strip:8001/");

          ws.onopen = function() {
            // Web Socket is connected
            //alert("Connection is opened...");
          };

          ws.onmessage = function(evt) {

          };

          ws.onclose = function() {
            // websocket is closed.
            alert("Connection is closed...");
          };

    $('#buttons').load("/content.html");
        } else {
          // The browser doesn't support WebSocket
          alert("WebSocket NOT supported by your Browser!");
        }
      }
    </script>
  </head>

  <body onLoad="javascript:WebSocketBegin();">
    <header id="main-header" class="py-2 bg-success text-white">
      <div class="container">
        <div class="row justify-content-md-center">
          <div class="col-md-6 text-center">
            <h1><i class="fas fa-cog"></i> ESP32 control</h1>
          </div>
        </div>
      </div>
    </header>

    <section class="py-5 bg-white">

      <div class="container">
        <div class="row">
          <div class="col">
             <div id="buttons"></div> 
          </div>
        </div>
      </div>

    </section>
  </body>
</html>
)rawText"
