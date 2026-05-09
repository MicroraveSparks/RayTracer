#include "ServerHandler.h"

const char HEAD[] PROGMEM = R"rawliteral(
<title>Raytracer Control Panel</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="stylesheet" type="text/css" href="style.css">
<link rel="icon" href="stingrayx32.png">
)rawliteral";

String RayTracerWebServer::HeuristicToggle(String heuristicsName, String id, bool enabled) {
	String html = "<div class=\"form-check form-switch\"><input class=\"form-check-input\" type=\"checkbox\" id=\""+id+"\" onchange=\"toggleFeature(this)\"" + (enabled ? " checked" : "") + "><label class=\"form-check-label\" for=\""+id+"\">"+heuristicsName+"</label></div>";
	return html;
}

String RayTracerWebServer::alertJS(String message, String redirect, int timeout)
{
	String nextAction = "window.location.href = \""+redirect+"\";"; 
	if (redirect == "#")
	{
		nextAction = "location.reload();";
	}
	String script = "<script>alert(\""+message+"\"); setTimeout(() => {"+nextAction+"}, "+timeout+");</script>";
	return script;
}

RayTracerWebServer::RayTracerWebServer(Status &currentStatus, int port, HardwareSerial &serial, int outputPin) : AsyncWebServer(port), raytracerStatus(currentStatus), SerialMon(serial), output(outputPin) {
		// Initialize sessions to empty
		for (int i = 0; i < MAX_ACCOUNTS; i++) {
				sessions[i] = {nullptr, String(), {0}};
		}
}

void RayTracerWebServer::setup_server(const char* ssid, const char* password, User *users, String encryptionKey) {
	for (int i = 0; i < MAX_ACCOUNTS; i++) {
		this->users[i] = users[i];
	}
	
	this->SerialMon.println("Starting WiFi AP...");
	WiFi.mode(WIFI_AP);
	WiFi.softAP(ssid, password);
	this->SerialMon.println("AP Started");
	this->SerialMon.println("SSID:\t" + WiFi.softAPSSID() + "\tIP Address:\t" + WiFi.softAPIP().toString() + "\tPassword: " + password);
	

	this->SerialMon.println("Initializing mbedtls...");
	mbedtls_pk_init(&this->pk_context);

	File publicKey = SD.open("/certs/public.pem");
	File privateKey = SD.open("/certs/private.pem");

	if (!publicKey || !privateKey) {
		if (!this->generate_certs(encryptionKey)) {
			this->SerialMon.println("Failed to generate certificate files.");
			while (true) { delay(1000); } // Halt execution if certificate files cannot be generated
		}
	} else {
		mbedtls_pk_parse_keyfile(&this->pk_context, "/certs/private.pem", encryptionKey.c_str(), mbedtls_ctr_drbg_random, nullptr);
		mbedtls_pk_parse_public_keyfile(&this->pk_context, "/certs/public.pem");
		publicKey.close();
		privateKey.close();
	}
	
	this->SerialMon.println("Setting up authentication middleware...");
	this->authMiddleware = new AsyncMiddlewareFunction([this](AsyncWebServerRequest *request, ArMiddlewareNext next){
		Session current = this->check_auth(request);
		this->SerialMon.printf("Auth middleware: current user=%s, expiry=%s\n", current.user ? current.user->username : "null", ctime(&current.expiry));
		now = time(NULL);
		if (current.user != nullptr && current.expiry >= now) {
			return next();
		}
		
		String message = "Invalid Login. You will be redirected once this dialog is closed.";
		String redirect = "/login";
		return request->send(307, "text/html", this->alertJS(message, redirect, 0));
	});
	
	this->SerialMon.println("Setting up admin authorization middleware...");
	this->adminMiddleware = new AsyncMiddlewareFunction([this](AsyncWebServerRequest *request, ArMiddlewareNext next) {
		Session current = this->check_auth(request);
		now = time(NULL);
		if ((current.user && current.user->admin) && current.expiry >= now) {
			return next();
		}
		const AsyncWebHeader *ref = request->getHeader("Referrer");
		String message = "Admin privleges required. You will be redirected once this dialog is closed.";
		String prev = ref ? ref->value() : "/login";
		return request->send(307, "text/html", this->alertJS(message, prev, 0));
	});

	this->SerialMon.println("Setting up server routes...");
	this->setupRoutes();
}

void RayTracerWebServer::setModemInfo(JsonDocument info) {
	this->modemInfo = info;
}

bool RayTracerWebServer::generate_certs(String encryptionKey) {
	// Placeholder for certificate generation logic
	// In a real implementation, you would use mbedtls to generate a key pair and self-signed certificate
	this->SerialMon.println("Generating certificates...");
	mbedtls_pk_context rsa;
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	// Seed DRBG
	mbedtls_entropy_init(&entropy);
	mbedtls_ctr_drbg_init(&ctr_drbg);
	mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char*)"pers", 4);
	mbedtls_pk_setup(&this->pk_context, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
	// For simplicity, we will just create empty files here. Replace with actual certificate generation.
	File cert_dir = SD.open("/certs");
	if (!cert_dir) {
		SD.mkdir("/certs");
	}
	File privateFile = SD.open("/certs/private.pem", FILE_WRITE);
	File publicFile = SD.open("/certs/public.pem", FILE_WRITE);
	if (!privateFile || !publicFile) {
		return false;
	}
	mbedtls_pk_write_key_pem(&this->pk_context, (unsigned char*)privateFile.name(), privateFile.size());
	mbedtls_pk_write_pubkey_pem(&this->pk_context, (unsigned char*)publicFile.name(), publicFile.size());
	privateFile.close();
	publicFile.close();
	mbedtls_pk_parse_keyfile(&this->pk_context, "/certs/private.pem", encryptionKey.c_str(), mbedtls_ctr_drbg_random, nullptr);
	mbedtls_pk_parse_public_keyfile(&this->pk_context, "/certs/public.pem");
	return true;
}

IPAddress getIP() {
	return WiFi.softAPIP();
}

void RayTracerWebServer::setupRoutes() {
	this->serveStatic("/static", SD, "/www/static").setCacheControl("max-age=600");

	this->on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
			Session current = this->check_auth(request);
			User *user = current.user;
			request->send(SD, "/www/index.html", String(), false, [this, user](const String& var){
				if (var == "ADMINONLY") {
					String isAdmin = (user && user->admin) ? "" : "hidden";
					return isAdmin;
				}
				return this->processor(var);
			});
		}).addMiddleware(this->authMiddleware);

	this->on("/login", HTTP_GET, [this](AsyncWebServerRequest *request){
		request->send(SD, "/www/login.html", String(), false, [this](const String& var){
			this->SerialMon.println("Processing login page template...");
			return this->processor(var);
		});
	});

	this->on("/login", HTTP_POST, [this](AsyncWebServerRequest *request){
		now = time(NULL);
		this->SerialMon.printf("%s: Login attempt received\n", ctime(&now));
		this->SerialMon.printf("%s: Request params:\n", ctime(&now));
		for (int i = 0; i < request->params(); i++) {
			const AsyncWebParameter* param = request->getParam(i);
			this->SerialMon.printf("%s: %s\n", param->name().c_str(), param->value().c_str());
		}
		this->SerialMon.printf("%s: End of request data\n", ctime(&now));
		String username, password;
		if (request->hasParam("username", true) && request->hasParam("password", true)) {
			username = request->getParam("username", true)->value();
			password = request->getParam("password", true)->value();
			for (int i = 0; i < MAX_ACCOUNTS; i++) {
				this->SerialMon.printf("%s: Checking credentials against user %d: username=%s, password_hash=%u\n", ctime(&now), i, this->users[i].username, this->users[i].password_hash);
				if (strcmp(username.c_str(), this->users[i].username) == 0 && this->users[i].password_hash == hash_password(password.c_str())) {
					this->SerialMon.printf("%s: Successful login for user '%s'\n", ctime(&now), username.c_str());
					String token = this->generate_token(i);
					this->sessions[i].token = token;
					this->sessions[i].user = &this->users[i];
					this->sessions[i].expiry = now + SESSION_TIMEOUT;
					String cookieStr = "token=" + token + "; HttpOnly; Max-Age=" + String(SESSION_TIMEOUT) + ";";
					AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "OK");
					response->addHeader("Set-Cookie", cookieStr);
					response->addHeader("Location", "/");
					return request->send(response);
				}
			}
		} else {
			this->SerialMon.printf("%s: Missing username or password in the request\n", ctime(&now));
			String message = "Missing username or password. You will be redirected once this dialog is closed.";
			String redirect = "#";
			return request->send(400, "text/html", this->alertJS(message, redirect, 0));
		}
	}, [this](AsyncWebServerRequest *request, const String& filename, unsigned int, unsigned char*, unsigned int, bool){
			this->SerialMon.printf("Received file: %s\n", filename.c_str());
		// This body handler is required to process the POST data and populate request parameters
		// The actual login logic will be handled in the main handler above after the body is processed
		}, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
			String body = String((char*)data).substring(0, len);
			this->SerialMon.printf("Received login data: %s\n", body.c_str());
			return request->send(200, "text/plain", "OK");
	});
		
	this->on("/logout", HTTP_GET, [this](AsyncWebServerRequest *request){
		Session current = this->check_auth(request);
		if (current.user) {
			for (int i = 0; i < MAX_ACCOUNTS; i++) {
				if (sessions[i].user == current.user) {
					sessions[i] = {nullptr, String(), {0}};
					this->SerialMon.printf("User '%s' logged out successfully\n", current.user->username);
					return request->redirect("/logged-out");
					break;
				}
			}
		}
		return request->redirect("/logged-out");
	}).addMiddleware(this->authMiddleware);

	this->on("/logged-out", HTTP_GET, [this](AsyncWebServerRequest *request){
		request->send(SD, "/www/logged-out.html", String(), false, [this](const String& var){
			return this->processor(var);
		});
	});

	this->on("/start", HTTP_GET, [this](AsyncWebServerRequest *request) {
		String inputMessage;
		String inputParam;
		inputParam = "state";
		if (request->hasParam("state")) {
			inputMessage = request->getParam("state")->value();
			this->raytracerStatus.setRunning(inputMessage == "1" ? true : false);
			digitalWrite(output, this->raytracerStatus.isRunning() ? HIGH : LOW);
			this->SerialMon.println("Raytracer " + String(this->raytracerStatus.isRunning() ? "started!" : "stopped!"));
		}
		else {
			inputMessage = "No message sent";
			inputParam = "none";
		}
		request->send(200, "text/plain", "OK");
	}).addMiddleware(this->authMiddleware);

	this->on("/modem-info", HTTP_GET, [this](AsyncWebServerRequest *request) {
		this->SerialMon.println("Serving modem info page...");
		this->SerialMon.println("Current modem info:");
		for (JsonPair pair : this->modemInfo.as<JsonObject>()) {
			this->SerialMon.printf("%s: %s\n", pair.key().c_str(), pair.value().as<String>().c_str());
		}
		request->send(SD, "/www/modem-info.html", String(), false, [this](const String& var){
			if (var == "MANUFACTURER") {
				return this->modemInfo["manufacturer"].as<String>();
			}
			if (var == "MODEM_MODEL") {
				return this->modemInfo["model"].as<String>();
			}
			if (var == "HARDWARE_VERSION") {
				return this->modemInfo["hardware_version"].as<String>();
			}
			if (var == "FIRMWARE_VERSION") {
				return this->modemInfo["firmware_version"].as<String>();
			}
			if (var == "IMEI") {
				return this->modemInfo["imei"].as<String>();
			}
			if (var == "IMSI") {
				return this->modemInfo["imsi"].as<String>();
			}
			return this->processor(var);
		});
	}).addMiddlewares({this->authMiddleware, this->adminMiddleware});

	this->on("/safety-status", HTTP_GET, [this](AsyncWebServerRequest *request) {
		switch (this->raytracerStatus.getSafetyStatus()){
			case SafetyStatus::Safe:
				return request->send(200, "application/json", "{\"status\":\"safe\"}");
			case SafetyStatus::Unsafe:
				return request->send(200, "application/json", "{\"status\":\"unsafe\"}");
			case SafetyStatus::Paused:
				return request->send(200, "application/json", "{\"status\":\"paused\"}");
			case SafetyStatus::Error:
				return request->send(500, "application/json", "{\"status\":\"error\"}");
			default:
				this->SerialMon.println("Unknown safety status, returning error");
				return request->send(500, "application/json", "{\"status\":\"error\"}");
		}
	}).addMiddleware(this->authMiddleware);

	JsonDocument Heurtistics = this->fetchHeurtistics();
	this->SerialMon.println("Setting up heuristic routes for heuristics:");
	this->SerialMon.println(Heurtistics.as<String>());
	for (JsonPair pair : Heurtistics.as<JsonObject>()) {
		String heuristicString = pair.key().c_str();
		this->on("/" + String(pair.key().c_str()), HTTP_GET, [this, heuristicString](AsyncWebServerRequest *request){
			JsonDocument Heurtistics = this->fetchHeurtistics();
			JsonObject heuristic = Heurtistics[heuristicString].as<JsonObject>();
			JsonObject newHeuristics = Heurtistics.as<JsonObject>();
			String heuristicName = heuristic["name"].as<String>();
			this->SerialMon.printf("Setting up route for heuristic '%s'\n", heuristicName);
			String inputMessage;
			String inputParam;
			inputParam = "state";
			if (request->hasParam("state")) {
				inputMessage = request->getParam("state")->value();
				bool enabled = inputMessage == "1" ? true : false;
				this->SerialMon.printf("Heuristic '%s' %s\n", heuristicName, enabled ? "enabled" : "disabled");
				newHeuristics[heuristicString]["enabled"] = enabled;
				
				File heuristicsFile = SD.open("/settings/heuristics.json", FILE_WRITE);
				if (heuristicsFile) {
					serializeJson(newHeuristics, heuristicsFile);
					heuristicsFile.close();
					return request->send(200, "text/plain", "OK");
				} else {
					this->SerialMon.println("Failed to open heuristics file for writing");
					String message = "Failed to update heuristics settings. Please try again. Please reboot the device if the problem persists.";
					return request->send(500, "text/html", alertJS(message, "#", 0));
				}
			}
			else {
				inputMessage = "No message sent";
				inputParam = "none";
				String message = "Invalid request. No state parameter provided.";
				return request->send(400, "text/html", alertJS(message, "#", 0));
			}
			String message = "An unexpected error occurred. Please try again.";
			return request->send(500, "text/html", alertJS(message, "#", 0));
		}).addMiddleware(this->authMiddleware);
	}

	this->on("/logs", HTTP_GET, [this](AsyncWebServerRequest *request) {
		this->SerialMon.println("Serving logs page...");
		request->send(SD, "/www/logs.html", String(), false, [this](const String& var){
			if (var == "LOGS") {
				String html = "";
				JsonDocument *logsDoc = logList();
				JsonArray logsArray = logsDoc->as<JsonArray>();
				if (logsArray.isNull()) {
					this->SerialMon.println("Failed to list logs or no logs found");
					html += "<p>No logs available.</p>";
					return html;
				}
				for (JsonObject log : logsArray) {
					html += "<tr>";
					html += "<td>" + log["id"].as<String>() + "</td>";
					html += "<td>" + log["size"].as<String>() + "</td>";
					html += "<td>" + log["timestamp"].as<String>() + "</td>";
					html += "<td><a href=\"/download-log?logid=" + log["id"].as<String>() + "\" class=\"btn btn-primary\" download=\"" + log["timestamp"].as<String>() + ".pcap\">View Log</a></td>";
					html += "</tr>";
				}
				return html;
			}
			return this->processor(var);
		});
	}).addMiddlewares({this->authMiddleware, this->adminMiddleware});
	this->on("/download-log", HTTP_GET, [this](AsyncWebServerRequest *request) {
		if (request->hasParam("logid")) {
			String logfile = request->getParam("logid")->value();
			this->SerialMon.printf("Log file requested: %s\n", logfile.c_str());
			if (SD.exists("/logs/" + logfile + ".pcap")) {
				return request->send(SD, ("/logs/" + logfile + ".pcap"), "application/vnd.tcpdump.pcap");
			} else {
				this->SerialMon.println("Requested log file does not exist");
				String message = "Requested log file does not exist. Please select a valid log file.";
				return request->send(404);
			}
		} else {
			this->SerialMon.println("No logfile parameter provided in the request");
			String message = "Invalid request. No logfile parameter provided.";
			return request->send(400, "text/html", alertJS(message, "#", 0));
		}
	}).addMiddlewares({this->authMiddleware, this->adminMiddleware});

	this->onNotFound([](AsyncWebServerRequest *request){
		request->send(SD, "/www/404.html", String(), false);
	});
}

JsonDocument RayTracerWebServer::fetchHeurtistics() {
	File heuristicsFile = SD.open("/settings/heuristics.json");
	JsonDocument doc;
	if (heuristicsFile) {
		deserializeJson(doc, heuristicsFile);
		heuristicsFile.close();
		this->SerialMon.println("Heuristics settings loaded successfully:");
		this->SerialMon.println(doc.as<String>());
	} else {
		this->SerialMon.println("Failed to open heuristics file, using default settings");
		doc["test_heuristic"]["name"] = "Test Heuristic";
		doc["test_heuristic"]["enabled"] = false;
	}
	return doc;
}

String RayTracerWebServer::generate_token(int userIndex) {
	// Generate a token
	char tokenStr[256] = {'\0'};
	sprintf(tokenStr, "{username:%s,admin:%s}", users[userIndex].username, (users[userIndex].admin ? "true" : "false"));
	this->SerialMon.printf("Generated token string: %s\n", tokenStr);
	unsigned char encoded[256];
	size_t olen = 0;
	mbedtls_base64_encode(encoded, sizeof(encoded), &olen, (unsigned char*)tokenStr, strlen(tokenStr));
	this->SerialMon.printf("Generated token: %s\n", encoded);
	return String((char*)encoded);
}

RayTracerWebServer::Session RayTracerWebServer::check_auth(AsyncWebServerRequest *request) {
	if (!request->hasHeader("Cookie")) {
		request->send(401, "text/plain", "Unauthorized");
		return Session(nullptr, String(), {0});
	}
	if (request->getHeader("Cookie")->value().startsWith("token=")) {
		String token = request->getHeader("Cookie")->value().substring(6); // Extract token value from "token=..."
		for (int i = 0; i < MAX_ACCOUNTS; i++) {
			this->SerialMon.printf("Checking session %d: token=%s\n", i, sessions[i].token ? sessions[i].token : "null");
			if (this->sessions[i].token == token) {
				return this->sessions[i];
			}
		}
	}
	request->send(401, "text/plain", "Unauthorized");
	return Session(nullptr, String(), {0});
}

String RayTracerWebServer::processor(const String& var) {
	if (var == "STATUS") {
		switch (this->raytracerStatus.getSafetyStatus()) {
			case SafetyStatus::Safe:
				return "Safe";
			case SafetyStatus::Unsafe:
				return "Unsafe";
			case SafetyStatus::Paused:
				return "Paused";
			case SafetyStatus::Error:
				return "Error";
			default:
				this->SerialMon.println("Unknown safety status, returning error");
				return "Error";
		}
	}
	if (var == "HEAD") {
		return HEAD;
	}
	if (var == "ISRUNNING") {
		return this->raytracerStatus.isRunning() ? "checked" : "";
	}
	if (var == "HEURISTICS") {
		String html = "";
		JsonDocument Heurtistics = this->fetchHeurtistics();
		for (JsonPair pair : Heurtistics.as<JsonObject>()) {
			JsonObject heuristic = pair.value().as<JsonObject>();
			html += HeuristicToggle(heuristic["name"].as<String>(), pair.key().c_str(), heuristic["enabled"].as<bool>());
		}
		return html;
	}
	if (var == "STATE"){
		if(this->raytracerStatus.isRunning()) {
			return "Running";
		}
		else {
			return "Paused";
		}
	}
	return String();
}
