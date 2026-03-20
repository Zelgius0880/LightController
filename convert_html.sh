#!/bin/bash

HTML_FILE="index_html.html"
HEADER_FILE="lib/WebServerHandler/index_html.h"

if [ ! -f "$HTML_FILE" ]; then
    echo "Error: $HTML_FILE not found!"
    exit 1
fi

HTML_CONTENT=$(cat "$HTML_FILE")

# Generate the header file content
cat <<EOF > "$HEADER_FILE"
#ifndef INDEX_HTML_H
#define INDEX_HTML_H

constexpr char index_html[] PROGMEM = R"rawliteral(
$HTML_CONTENT
)rawliteral";

#endif // INDEX_HTML_H
EOF

echo "Successfully converted $HTML_FILE to $HEADER_FILE"
