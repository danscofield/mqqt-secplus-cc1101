#include <pgmspace.h>

#define SECRET

const char WIFI_SSID[] = "fill me in";
const char WIFI_PASSWORD[] = "fill me in";

// Amazon Root CA 1
static const char AWS_CERT_CA[] PROGMEM = R"EOF(fill me in)EOF";

// Device Certificate
static const char AWS_CERT_CRT[] PROGMEM = R"KEY(fill me in)KEY";

// Device Private Key
static const char AWS_CERT_PRIVATE[] PROGMEM = R"KEY(fill me in)KEY";