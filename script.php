<?php

// This script is intended to simulate a CGI script that reads data from stdin and echoes it back with a HTTP response header.
$data = file_get_contents("php://stdin");
echo "HTTP/1.1 200 OK\r\n";
echo "Content-Type: text/html\r\n\r\n";
echo("<html>\n<body>");
echo("<div style=\"width: 100%; font-size: 40px; font-weight: bold; text-align: center;\">");
echo("CGI Script Test Page");
echo "\nReceived data: $data";
echo("</div>");
echo("</body>\n</html>");

// Output HTTP response header