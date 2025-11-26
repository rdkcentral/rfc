/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

const https = require('node:https');
const path = require('node:path');
const fs = require('node:fs');
const url = require('node:url');
const { applyMtlsConfig } = require('./server-utils');

// HTTPS options with base configuration
const options = {
  key: fs.readFileSync(path.join('/etc/xconf/certs/mock-xconf-server-key.pem')),
  cert: fs.readFileSync(path.join('/etc/xconf/certs/mock-xconf-server-cert.pem')),
  port: 50053
};

// Apply mTLS settings if enabled using the centralized utility
applyMtlsConfig(options);

let save_request = false;
let savedrequest_json={};

/**
 * Function to read JSON file and return the data
 */
function readJsonFile(count) {
  
  var filePath = path.join('/etc/xconf', 'xconf-rfc-response.json');
  
  try {
    const fileData = fs.readFileSync(filePath, 'utf8');
    return JSON.parse(fileData);
  } catch (error) {
    console.error('Error reading or parsing JSON file:', error);
    return null;
  }
}

/*
* Function to handle RFC Data
*/
function handleRFCData(req, res, queryObject, file) {
  let data = '';
  req.on('data', function(chunk) {
    data += chunk;
  });
  req.on('end', function() {
    console.log('Data received: ' + data);
  });

  if (save_request) {
    savedrequest_json[new Date().toISOString()] = { ...queryObject };
  }
  res.writeHead(200, {'Content-Type': 'application/json', 'configSetHash': '1KM7h9ommUuUoyVm8oAvp2JCC19zyVJAsp'});
  res.end(JSON.stringify(readJsonFile(file)));
  return;
}

/**
 * Handles the incoming request and logs the data received
 * @param {http.IncomingMessage} req - The incoming request object
 * @param {http.ServerResponse} res - The server response object
 */
function requestHandler(req, res) {
  const queryObject = url.parse(req.url, true).query;
  console.log('Query Object: ' + JSON.stringify(queryObject));
  console.log('Request received: ' + req.url);
  console.log('json'+JSON.stringify(savedrequest_json));
  if (req.method === 'GET') {
    if (req.url.startsWith('/featureControl/getSettings')) {
      return handleRFCData(req, res, queryObject,0);
    }
    else if (req.url.startsWith('/adminSupportSet')) {
      handleAdminSet(req, res, queryObject);
    }
    else if (req.url.startsWith('/adminSupportGet')) {
      return handleAdminGet(req, res, queryObject);
    }
    else if (req.url.startsWith('/featureControl404/getSettings')) {
      res.writeHead(404);
      res.end("404 No Content");
      return;
    }
    else if (req.url.startsWith('/featureControl304/getSettings')) {
      res.writeHead(304);
      res.end("304 Not Modified");
      return;
    }
  }
  else if (req.method === 'POST') {
    /* Update Settings */
  }
  res.writeHead(200);
  res.end("Server is Up Please check the request....");
}

/* Mock Server for RFC */
const serverInstance = https.createServer(options, requestHandler);
serverInstance.listen(
  options.port,
  () => {
    console.log('RFC XConf Mock Server running at https://localhost:50053/');
  }
);
