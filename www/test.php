<?php
// Output CGI-compliant headers
echo "Content-Type: text/html\n\n";

// HTML output
echo "<!DOCTYPE html>\n";
echo "<html>\n";
echo "<head><title>PHP CGI Test</title></head>\n";
echo "<body>\n";
echo "<h1>PHP CGI Test Script</h1>\n";

// Display query parameter (e.g., param1 from QUERY_STRING)
echo "<p>Query Parameter (param1): " . (isset($_GET['param1']) ? htmlspecialchars($_GET['param1']) : "Not set") . "</p>\n";

// Display some CGI environment variables
echo "<h2>Environment Variables</h2>\n";
echo "<ul>\n";
echo "<li>REQUEST_METHOD: " . (isset($_SERVER['REQUEST_METHOD']) ? htmlspecialchars($_SERVER['REQUEST_METHOD']) : "Not set") . "</li>\n";
echo "<li>QUERY_STRING: " . (isset($_SERVER['QUERY_STRING']) ? htmlspecialchars($_SERVER['QUERY_STRING']) : "Not set") . "</li>\n";
echo "<li>SCRIPT_FILENAME: " . (isset($_SERVER['SCRIPT_FILENAME']) ? htmlspecialchars($_SERVER['SCRIPT_FILENAME']) : "Not set") . "</li>\n";
echo "<li>REDIRECT_STATUS: " . (isset($_SERVER['REDIRECT_STATUS']) ? htmlspecialchars($_SERVER['REDIRECT_STATUS']) : "Not set") . "</li>\n";
echo "</ul>\n";

echo "</body>\n";
echo "</html>\n";
?>