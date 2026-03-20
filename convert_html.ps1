$htmlFile = "index_html.html"
$headerFile = "lib\WebServerHandler\index_html.h"

if (!(Test-Path $htmlFile)) {
    Write-Error "Error: $htmlFile not found!"
    exit 1
}

$htmlContent = Get-Content -Raw $htmlFile

$headerContent = @"
#ifndef INDEX_HTML_H
#define INDEX_HTML_H

constexpr char index_html[] PROGMEM = R`"rawliteral(
$htmlContent
)rawliteral`";

#endif // INDEX_HTML_H
"@

Set-Content -Path $headerFile -Value $headerContent -NoNewline
Write-Host "Successfully converted $htmlFile to $headerFile"
