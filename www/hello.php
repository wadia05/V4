<?php
// Print HTTP headers
header("Content-Type: text/html");

// Get environment variables
$method = $_SERVER['REQUEST_METHOD'] ?? '';
$query_string = $_SERVER['QUERY_STRING'] ?? '';
$body_part = file_get_contents('php://input');

// Parse query string
$params = [];
if ($query_string) {
    parse_str($query_string, $params);
}

// Parse body part if it's a POST request
$body_params = [];
if ($method == 'POST' && $body_part) {
    parse_str($body_part, $body_params);
}

// Generate HTML response
echo "<html>";
echo "<head><title>CGI Test</title></head>";
echo "<body>";
echo "<h1>Hello from PHP CGI!</h1>";

echo "<h2>Environment Variables:</h2>";
echo "<ul>";
echo "<li>REQUEST_METHOD: " . htmlspecialchars($method) . "</li>";
echo "<li>QUERY_STRING: " . htmlspecialchars($query_string) . "</li>";
echo "</ul>";

if ($params) {
    echo "<h2>Query Parameters:</h2>";
    echo "<ul>";
    foreach ($params as $key => $value) {
        echo "<li>" . htmlspecialchars($key) . ": " . htmlspecialchars($value) . "</li>";
    }
    echo "</ul>";
}

if ($body_params) {
    echo "<h2>POST Data:</h2>";
    echo "<ul>";
    foreach ($body_params as $key => $value) {
        echo "<li>" . htmlspecialchars($key) . ": " . htmlspecialchars($value) . "</li>";
    }
    echo "</ul>";
}

echo "<h2>All Environment Variables:</h2>";
echo "<ul>";
foreach ($_SERVER as $key => $value) {
    echo "<li>" . htmlspecialchars($key) . ": " . htmlspecialchars($value) . "</li>";
}
echo "</ul>";

echo "</body>";
echo "</html>";
?>