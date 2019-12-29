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

    <script type="text/javascript">
function includeHTML() {
  var z, i, elmnt, file, xhttp;
  /*loop through a collection of all HTML elements:*/
  z = document.getElementsByTagName("*");

  for (i = 0; i < z.length; i++) {
    elmnt = z[i];
    /*search for elements with a certain atrribute:*/
    file = elmnt.getAttribute("w3-include-html");
    
    if (file) {
      /*make an HTTP request using the attribute value as the file name:*/
      xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
          if (this.status == 200) {elmnt.innerHTML = this.responseText;}
          if (this.status == 404) {elmnt.innerHTML = "Page not found.";}
          /*remove the attribute, and call this function once more:*/
          elmnt.removeAttribute("w3-include-html");
          includeHTML();
        }
      }      
      xhttp.open("GET", file, true);
      xhttp.send();
      /*exit the function:*/
      return;
    }
  }
};


      var ws;
      
      function triggerMode(theme, delay) {
  ws.send('{"trigger":{"mode":"infinite", "theme":' + theme + ', "delay":' + delay + '}}')
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
        } else {
          // The browser doesn't support WebSocket
          alert("WebSocket NOT supported by your Browser!");
        }
      }
    </script>
  </head>

  <body onLoad="javascript:WebSocketBegin(); javascript:includeHTML();">
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
             <div w3-include-html="content.html"></div> 
          </div>
        </div>
      </div>

    </section>
  </body>
</html>
)rawText"

//                <button
//                  type="button"
//                  class="btn btn-lg btn-warning btn-block"
//                  onmousedown='triggerMode("theme1", 50)'
//                  ontouchstart='triggerMode("theme1", 50)'
//                >
//                  click
//                </button>
